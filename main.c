#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <stdbool.h>
#include <event2/buffer.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/keyvalq_struct.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include <del.h>
#include "html.h"
#include "globals.h"
#include "delfuncs.h"
#include "sql.h"

#define VM_COUNT 100

// libevent helpers
static void generic_request_handler(struct evhttp_request *req, void *ctx)
{
    IGNORE(ctx);
    struct evbuffer *reply = evbuffer_new();
    evbuffer_add_printf(reply, html_404);
    evhttp_send_reply(req, HTTP_NOTFOUND, NULL, reply);
    evbuffer_free(reply);
}

static void homepage(struct evhttp_request *req, void *ctx)
{
    IGNORE(ctx);
    // struct DelContext *dc = (struct DelContext *) ctx;
    // enum DelVirtualMachineStatus status = del_vm_status(dc->vm);
    // if (status != DEL_VM_STATUS_COMPLETED && status != DEL_VM_STATUS_ERROR) {
    //     del_vm_execute(dc->vm);
    // }
    struct evbuffer *reply = evbuffer_new();
    // status = del_vm_status(dc->vm);
    // if (status == DEL_VM_STATUS_YIELD) {
    //     evbuffer_add_printf(reply, dc->output_buffer->buffer);
    //     dbuffer_clear(dc->output_buffer);
    // } else {
    //     // evbuffer_add_printf(reply, html_500);
    // }
    evbuffer_add_printf(reply, html_homepage);
    evhttp_send_reply(req, HTTP_OK, NULL, reply);
    evbuffer_free(reply);
}

#define MAX_FORM_QUERY_STRING 1000
char body[MAX_FORM_QUERY_STRING];

static struct evkeyvalq *setup_post(struct evhttp_request *req)
{
    // Get body
    struct evbuffer *buf = evhttp_request_get_input_buffer(req);
    memset(body, 0, MAX_FORM_QUERY_STRING-1);
    ev_ssize_t readlen = evbuffer_copyout(buf, body, MAX_FORM_QUERY_STRING-1);
    fprintf(stderr, "POST: readlen %d\n", (int) readlen);
    fprintf(stderr, "POST: MAX-1 %d\n", MAX_FORM_QUERY_STRING-1);
    if (readlen == -1 || readlen == MAX_FORM_QUERY_STRING-1) {
        fprintf(stderr, "POST failure: could not read query\n");
        return NULL;
    }
    fprintf(stderr, "body: %s\n", body);

    // Read parameters from body
    struct evkeyvalq *params = malloc(sizeof(*params));
    if (evhttp_parse_query_str(body, params) == -1) {
        fprintf(stderr, "POST failure: could not parse query parameters\n");
        free(params);
        return NULL;
    }
    return params;
}


static void route_new_questionnaire(struct evhttp_request *req, void *ctx)
{
    struct Globals *globals = ctx;
    struct evbuffer *reply = evbuffer_new();
    enum evhttp_cmd_type req_type = evhttp_request_get_command(req);
    if (req_type == EVHTTP_REQ_POST) {
        fprintf(stderr, "POST: new_questionnaire\n");
        struct evkeyvalq *params = setup_post(req);
        if (!params) {
            evbuffer_add_printf(reply, html_404);
            evhttp_send_reply(req, HTTP_NOTFOUND, NULL, reply);
            goto exit;
        }
        const char *q_name = evhttp_find_header(params, "q-name");
        if (!q_name) {
            fprintf(stderr, "POST failure: could not find header parameter 'q-name'");
            evbuffer_add_printf(reply, html_404);
            evhttp_send_reply(req, HTTP_NOTFOUND, NULL, reply);
            goto free_params;
        }
        fprintf(stderr, "POST: q_name: %s\n", q_name);

        if (!db_new_questionnaire(globals->db, q_name)) {
            evbuffer_add_printf(reply, html_500);
            evhttp_send_reply(req, HTTP_INTERNAL, NULL, reply);
            goto free_params;
        }

        evbuffer_add_printf(reply, html_new_questionnaire_response, q_name, "TODO: remove this");
        evhttp_send_reply(req, HTTP_OK, NULL, reply);
free_params:
        evhttp_clear_headers(params);
        free(params);
        goto exit;
    } else if (req_type == EVHTTP_REQ_GET) { // Default to GET request
        fprintf(stderr, "GET new_questionnaire\n");
        evbuffer_add_printf(reply, html_new_questionnaire);
        evhttp_send_reply(req, HTTP_OK, NULL, reply);
        goto exit;
    }
    evhttp_send_reply(req, HTTP_BADREQUEST, NULL, reply);
exit:
    evbuffer_free(reply);
}

static void signal_cb(evutil_socket_t fd, short event, void *arg)
{
    IGNORE(event);
    fprintf(stderr, "%s signal received\n", strsignal(fd));
    event_base_loopbreak(arg);
}

// Most libevent http functions return 0 on success
// Can use this for setup functions that we expect to succeed before continuing
#define SETUP_SUCCESS(ev_setup_call) assert(ev_setup_call == 0)

// int main(int argc, char **argv)
int main(void)
{
    struct Globals globals;
    assert(globals_init(&globals));

    // ***************************************************
    // ******************** del setup ********************
    // ***************************************************

    struct DelBufferedReader *reader = dbuffered_reader_new();
    struct DelBuffer *buffer = dbuffer_new();

    // Compile
    DelCompiler compiler;
    init_with_funcs(&compiler, reader, buffer);

    DelProgram program = del_compile_file(compiler, "example.del");
    if (!program) {
        del_compiler_free(compiler);
        dbuffered_reader_free(reader);
        dbuffer_free(buffer);
        fprintf(stderr, "An error occurred during compilation\n");
        return EXIT_FAILURE;
    }
    del_compiler_free(compiler);

    // Init VMs
    DelVM vm;
    del_vm_init(&vm, stdout, stderr, program);

    struct DelContext dc = { reader, buffer, vm };

    // ***************************************************
    // ***************** libevent setup ******************
    // ***************************************************

    ev_uint16_t http_port = 8080;
    char *http_addr = "0.0.0.0";
    struct event_base *base;
    struct evhttp *http_server;
    struct event *sig_int;

    base = event_base_new();

    // Setup
    http_server = evhttp_new(base);
    evhttp_set_allowed_methods(http_server, EVHTTP_REQ_GET | EVHTTP_REQ_POST);
    SETUP_SUCCESS(evhttp_bind_socket(http_server, http_addr, http_port));

    // Routes
    SETUP_SUCCESS(evhttp_set_cb(
                http_server,
                "/",
                homepage,
                &dc));
    SETUP_SUCCESS(evhttp_set_cb(
                http_server,
                "/new-questionnaire",
                route_new_questionnaire,
                &globals));
    // SETUP_SUCCESS(evhttp_set_cb(http_server, "/form", form, NULL));
    evhttp_set_gencb(http_server, generic_request_handler, NULL);

    sig_int = evsignal_new(base, SIGINT, signal_cb, base);
    event_add(sig_int, NULL);

    printf("Listening requests on http://%s:%d\n", http_addr, http_port);
    event_base_dispatch(base);

    // ***************************************************
    // ********************* Cleanup *********************
    // ***************************************************

    // dbuffer frees
    dbuffered_reader_free(reader);
    dbuffer_free(buffer);

    // del frees
    del_program_free(program);
    del_vm_free(vm);

    globals_cleanup(&globals);

    // libevent frees
    evhttp_free(http_server);
    event_free(sig_int);
    event_base_free(base);
}

