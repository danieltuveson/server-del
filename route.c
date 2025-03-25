// C Standard Library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>
// UNIX + libc extras
#include <alloca.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
// Deps
#include <uuid/uuid.h>
#include <event2/buffer.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/keyvalq_struct.h>
// Internal
#include "html.h"
#include "globals.h"
#include "sql.h"
#include "route.h"

// ******************************************************
// ****************** Helper functions ******************
// ******************************************************

static void route_log(const char *fmt, ...)
{
    fprintf(stderr, "ROUTE: ");
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}

static int str_prefix(const char *prefix, const char *path)
{
    int i;
    for (i = 0; prefix[i] != '\0'; i++) {
        if (prefix[i] != path[i]) {
            return 0;
        }
    }
    return i;
}

// ******************************************************
// ******************* Static routes ********************
// ******************************************************

char post_body[MAX_FORM_QUERY_STRING];

// libevent helpers
void route_404_notfound(struct evhttp_request *req, void *ctx)
{
    fprintf(stderr, "ROUTE: 404 - not found\n");
    IGNORE(ctx);
    struct evbuffer *reply = evbuffer_new();
    html_404_notfound(reply);
    evhttp_send_reply(req, HTTP_NOTFOUND, NULL, reply);
    evbuffer_free(reply);
}

void route_500_internal(struct evhttp_request *req, void *ctx)
{
    fprintf(stderr, "ROUTE: 500 - internal server error\n");
    IGNORE(ctx);
    struct evbuffer *reply = evbuffer_new();
    html_500_internal(reply);
    evhttp_send_reply(req, HTTP_INTERNAL, NULL, reply);
    evbuffer_free(reply);
}

// TODO:
// - Put upper limit on number of surveys to iterate through
// - If limit is exceeded, allow user to load another page of results
// - Add ability to search?
#define SURVEY_LIST_LIMIT 30
static void route_index_nocheck(struct evhttp_request *req, void *ctx)
{
    fprintf(stderr, "ROUTE: index\n");
    struct Globals *globals = ctx;
    struct evbuffer *reply = evbuffer_new();
    html_index_start(reply);

    sqlite3_stmt *stmt = db_prep_get_survey_name(globals->db);
    enum DatabaseStatus db_status = DB_IN_PROGRESS;

    struct SurveyRecord sr;
    memset(&sr, 0, sizeof(sr));
    sr.survey_uuid = alloca(UUID_STR_LEN);
    sr.name = alloca(MAX_SURVEY_NAME_LENGTH + 1);
    bool finished_early = true;
    int i = 0;
    for (; i < SURVEY_LIST_LIMIT; i++) {
        db_status = db_get_survey_name(stmt, &sr);
        if (db_status == DB_FINISHED) {
            finished_early = false;
            break;
        } else if (db_status != DB_IN_PROGRESS) {
            evbuffer_free(reply);
            route_500_internal(req, ctx);
            return;
        }
        html_index_row(reply, sr.survey_uuid, sr.name);
        memset(sr.survey_uuid, 0, UUID_STR_LEN);
        memset(sr.name, 0, MAX_SURVEY_NAME_LENGTH + 1);
    }
    if (finished_early && i == SURVEY_LIST_LIMIT) {
        db_status = db_get_survey_name(stmt, &sr);
        if (db_status == DB_FINISHED) {
            finished_early = false;
        }
    }
    html_index_end(reply, finished_early);
    evhttp_send_reply(req, HTTP_OK, NULL, reply);
    evbuffer_free(reply);
}
#undef SURVEY_LIST_LIMIT

void route_index(struct evhttp_request *req, void *ctx)
{
    enum evhttp_cmd_type req_type = evhttp_request_get_command(req);
    if (req_type != EVHTTP_REQ_GET) {
        fprintf(stderr, "ROUTE: index - bad request type\n");
        route_404_notfound(req, ctx);
        return;
    }
    route_index_nocheck(req, ctx);
}

static struct evkeyvalq *setup_post(struct evhttp_request *req)
{
    // Get body
    struct evbuffer *buf = evhttp_request_get_input_buffer(req);
    memset(post_body, 0, MAX_FORM_QUERY_STRING-1);
    ev_ssize_t readlen = evbuffer_copyout(buf, post_body, MAX_FORM_QUERY_STRING-1);
    fprintf(stderr, "POST: readlen %d\n", (int) readlen);
    fprintf(stderr, "POST: MAX-1 %d\n", MAX_FORM_QUERY_STRING-1);
    if (readlen == -1 || readlen == MAX_FORM_QUERY_STRING-1) {
        fprintf(stderr, "POST failure: could not read query\n");
        return NULL;
    }
    fprintf(stderr, "post_body: %s\n", post_body);

    // Read parameters from post_body
    struct evkeyvalq *params = malloc(sizeof(*params));
    if (evhttp_parse_query_str(post_body, params) == -1) {
        fprintf(stderr, "POST failure: could not parse query parameters\n");
        free(params);
        return NULL;
    }
    return params;
}

void route_new_survey(struct evhttp_request *req, void *ctx)
{
    struct Globals *globals = ctx;
    struct evbuffer *reply = evbuffer_new();
    enum evhttp_cmd_type req_type = evhttp_request_get_command(req);
    if (req_type == EVHTTP_REQ_POST) {
        fprintf(stderr, "POST: new_survey\n");
        struct evkeyvalq *params = setup_post(req);
        if (!params) {
            route_404_notfound(req, ctx);
            goto exit;
        }
        const char *s_name = evhttp_find_header(params, "survey-name");
        if (!s_name) {
            fprintf(stderr, "POST failure: could not find header parameter 'survey-name'");
            route_404_notfound(req, ctx);
            goto free_params;
        }
        fprintf(stderr, "POST: s_name: %s\n", s_name);

        if (!db_new_survey(globals->db, s_name)) {
            route_500_internal(req, ctx);
            goto free_params;
        }

        struct evkeyvalq *headers = evhttp_request_get_output_headers(req);
        evhttp_add_header(headers, "Location", "/");
        evhttp_send_reply(req, HTTP_MOVETEMP, NULL, NULL);
        evhttp_clear_headers(headers);
free_params:
        evhttp_clear_headers(params);
        free(params);
        goto exit;
    } else if (req_type == EVHTTP_REQ_GET) { // Default to GET request
        fprintf(stderr, "GET new_survey\n");
        html_new_survey(reply);
        evhttp_send_reply(req, HTTP_OK, NULL, reply);
        goto exit;
    }
    evhttp_send_reply(req, HTTP_BADREQUEST,NULL, reply);
exit:
    evbuffer_free(reply);
}

// TODO
// - Return routing code and metadata code as two textareas of GET
// - Has "save" button for submitting 
// - If post, try to recompile: 
//   - If there are errors, return them in a div
//   - If there are no errors, return to same screen as GET
void route_edit_survey(struct evhttp_request *req, void *ctx)
{
    struct Globals *globals = ctx;

    sqlite3_stmt *stmt = db_prep_get_survey_program(globals->db);
    enum DatabaseStatus db_status = DB_IN_PROGRESS;

    struct SurveyRecord sr;
    memset(&sr, 0, sizeof(sr));
    sr.survey_uuid = alloca(UUID_STR_LEN);
    sr.name = alloca(MAX_SURVEY_NAME_LENGTH + 1);
    enum DatabaseStatus db_status = db_get_survey_program(stmt, &sr, s_name);
    assert(false);
}

void route_take_survey(struct evhttp_request *req, void *ctx)
{
    IGNORE(req);
    IGNORE(ctx);
    // TODO
    // Should take a survey uuid as parameter and initialize a new VM
    // - Lookup survey, including program
    // - Create new VM from source, if it does not yet exist
    assert(false);
}

// void route_survey_name(struct evhttp_request *req, void *ctx)
// {
//     // TODO
// }

#define SERVER_ROOT "/home/dtuveson/server-del"

static void route_static(struct evhttp_request *req, const char *path, void *ctx)
{
    route_log("static: %s", path);

    // Validate request type
    enum evhttp_cmd_type req_type = evhttp_request_get_command(req);
    if (req_type != EVHTTP_REQ_GET) {
        route_log("static: bad request type");
        route_404_notfound(req, ctx);
        return;
    }
    
    // Try to get file info
    struct stat st;
    int status = stat(path, &st);
    if (status != 0) {
        route_log("static: could not stat '%s'", path);
        route_404_notfound(req, ctx);
        return;
    }

    // Try to open file
    int fd = -1;
    assert(st.st_size != 0);
    fd = open(path, O_RDONLY);
    if (fd == -1) {
        route_log("css - could not open file '%s'", path);
        route_404_notfound(req, ctx);
        return;
    }

    // Send content
    struct evbuffer *evbuff = evbuffer_new();
    status = evbuffer_add_file(evbuff, fd, 0, st.st_size);
    if (status != 0) {
        route_log("css: error adding file to evbuffer");
        evhttp_send_error(req, HTTP_INTERNAL, NULL);
        goto cleanup;
    }
    char buf[128];
    route_log("css: Content-Length: %lu", st.st_size);
    snprintf(buf, sizeof(buf), "%lu", st.st_size);
    struct evkeyvalq *headers = evhttp_request_get_output_headers(req);
    evhttp_add_header(headers, "Content-Type", "text/css");
    evhttp_add_header(headers, "Content-Length", buf);
    evhttp_send_reply(req, HTTP_OK, "OK", evbuff);
cleanup:
    close(fd);
    evbuffer_free(evbuff);
}

int validate_filename(const char *filename)
{
    assert(filename != NULL);
    if (filename[0] != '/') {
        return -1;
    }
    int i = 1;
    for (char c = filename[i]; c != '\0'; i++) {
        if (!((isalnum(c) || c == '-' || c == '.') && (i < FILENAME_MAX))) {
            return -1;
        }
        c = filename[i + 1];
    }
    return i;
}

static void route_css(struct evhttp_request *req, void *ctx)
{
    route_log("css");
    struct Globals *globals = ctx;
    const char *path = globals->path;
    const char *filename = globals->filename;
    int length = validate_filename(filename);
    if (length == -1) {
        route_log("malformed filename requested: '%s'", filename);
        route_404_notfound(req, ctx);
        return;
    }
    int server_root_len = sizeof(SERVER_ROOT) - 1;
    int path_len = strlen(path);
    int full_path_len = path_len + server_root_len;
    route_log("full path len: %d", path_len);
    char *full_path = calloc(full_path_len + 1, 1);
    for (int i = 0; i < full_path_len; i++) {
        full_path[i] = i < server_root_len
            ? SERVER_ROOT[i]
            : path[i - server_root_len];
    }
    route_log("full_path: %s", full_path);
    route_static(req, full_path, ctx);
    free(full_path);
}

static void route_javascript(struct evhttp_request *req, void *ctx)
{
    // TODO
    assert(false);
    route_log("javascript");
    struct Globals *globals = ctx;
    const char *path = globals->path;
    const char *filename = globals->filename;
    int length = validate_filename(filename);
    if (length == -1) {
        route_log("malformed filename requested: '%s'", filename);
        route_404_notfound(req, ctx);
        return;
    }
    route_static(req, path, ctx);
}

// ******************************************************
// ******************* Dynamic routes *******************
// ******************************************************

struct RoutingPair {
    const char *route;
    void(*callback)(struct evhttp_request *, void *);
};

const struct RoutingPair top_level_routing_table[] = {
    { "/edit-survey",       route_edit_survey },
    { "/take-survey",       route_take_survey },
    { "/static/javascript", route_javascript },
    { "/static/css",        route_css },
};

void route_dynamic(struct evhttp_request *req, void *ctx)
{
    struct Globals *globals = ctx;
    route_log("route_dynamic");

    struct evhttp_uri *decoded = evhttp_uri_parse(evhttp_request_get_uri(req));
    if (!decoded) {
        route_log("error parsing uri");
        route_500_internal(req, ctx);
        return;
    }

    const char *path = evhttp_uri_get_path(decoded);
    if (!path) {
        path = "/";
    }

    char *decoded_path = evhttp_uridecode(path, 0, NULL);
    if (decoded_path == NULL) {
        route_log("error decoding uri");
        route_500_internal(req, ctx);
        goto exit;
    }

    route_log("decoded path: %s", decoded_path);
    int table_len = sizeof(top_level_routing_table) / sizeof(struct RoutingPair);
    for (int i = 0; i < table_len; i++) {
        const char *prefix = top_level_routing_table[i].route;
        int num_chars = str_prefix(prefix, decoded_path);
        if (num_chars != 0) {
            route_log("found route: %s", prefix);
            globals->path = decoded_path;
            globals->filename = decoded_path + num_chars;
            top_level_routing_table[i].callback(req, ctx);
            route_log("remaining path: %s", decoded_path + num_chars);
            goto exit;
        }
    }
    route_404_notfound(req, ctx);
exit:
    evhttp_uri_free(decoded);
    if (decoded_path) {
        free(decoded_path);
    }
}

