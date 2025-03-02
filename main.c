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
#include <del.h>
#include "html.h"

#define VM_COUNT 100

struct DelBuffer {
    int max_length;
    int length;
    char *buffer;
    char *output;
};

struct DelContext {
    struct DelBuffer dbuffer;
    DelVM vm;
};

#define DEL_BUFFER_SIZE 1000
#define IGNORE(ignored) (void)ignored

union DelForeignValue write_char(union DelForeignValue *arguments, void *context)
{
    char byte = arguments[0].byte;
    struct DelBuffer *dbuffer = (struct DelBuffer *) context;
    // Don't want to overflow if too many chars written
    if (dbuffer->length < dbuffer->max_length - 1) {
        dbuffer->buffer[dbuffer->length] = byte;
        dbuffer->length++;
    }
    DEL_NORETURN();
}

union DelForeignValue get_charbuffer_size(union DelForeignValue *arguments, void *context)
{
    IGNORE(arguments);
    IGNORE(context);
    union DelForeignValue del_buffer_size;
    del_buffer_size.integer = DEL_BUFFER_SIZE;
    return del_buffer_size;
}

union DelForeignValue flush_chars(union DelForeignValue *arguments, void *context)
{
    IGNORE(arguments);
    struct DelBuffer *dbuffer = (struct DelBuffer *) context;
    memset(dbuffer->output, 0, DEL_BUFFER_SIZE);
    memcpy(dbuffer->output, dbuffer->buffer, DEL_BUFFER_SIZE);
    memset(dbuffer->buffer, 0, DEL_BUFFER_SIZE);
    dbuffer->length = 0;
    DEL_NORETURN();
}

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
    // IGNORE(ctx);
    struct DelContext *dc = (struct DelContext *) ctx;
    enum DelVirtualMachineStatus status = del_vm_status(dc->vm);
    if (status != DEL_VM_STATUS_COMPLETED && status != DEL_VM_STATUS_ERROR) {
        del_vm_execute(dc->vm);
    }
    struct evbuffer *reply = evbuffer_new();
    status = del_vm_status(dc->vm);
    if (status == DEL_VM_STATUS_YIELD) {
        evbuffer_add_printf(reply, dc->dbuffer.output);
    } else {
        evbuffer_add_printf(reply, html_500);
        // evbuffer_add_printf(reply, html_homepage);
    }
    evhttp_send_reply(req, HTTP_OK, NULL, reply);
    evbuffer_free(reply);
}

#define MAX_FORM_QUERY_STRING 1000
char body[MAX_FORM_QUERY_STRING];

static void form(struct evhttp_request *req, void *ctx)
{
    IGNORE(ctx);
    struct evbuffer *reply = evbuffer_new();
    enum evhttp_cmd_type req_type = evhttp_request_get_command(req);
    if (req_type == EVHTTP_REQ_POST) {
        // Get body
        struct evbuffer *buf = evhttp_request_get_input_buffer(req);
        memset(body, 0, MAX_FORM_QUERY_STRING-1);
        ev_ssize_t readlen = evbuffer_copyout(buf, body, MAX_FORM_QUERY_STRING-1);
        printf("readlen %d\n", (int) readlen);
        printf("MAX-1 %d\n", MAX_FORM_QUERY_STRING-1);
        if (readlen == -1 || readlen == MAX_FORM_QUERY_STRING-1) {
            evhttp_send_reply(req, HTTP_INTERNAL, NULL, reply);
            goto exit;
        }
        printf("body: %s\n", body);

        // Read parameters from body
        struct evkeyvalq params;
        if (evhttp_parse_query_str(body, &params) == -1) {
            goto exit;
        }
        const char *firstname = evhttp_find_header(&params, "fname");
        const char *lastname = evhttp_find_header(&params, "lname");
        const char *yesNo = evhttp_find_header(&params, "yesNo");

        // Respond
        evbuffer_add_printf(reply, html_form_response, firstname, lastname, yesNo);
        evhttp_send_reply(req, HTTP_OK, NULL, reply);
        evhttp_clear_headers(&params);
        goto exit;
    } else if (req_type == EVHTTP_REQ_GET) { // Default to GET request
        evbuffer_add_printf(reply, html_form);
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
    printf("%s signal received\n", strsignal(fd));
    event_base_loopbreak(arg);
}

// Most libevent http functions return 0 on success
// Can use this for setup functions that we expect to succeed before continuing
#define SETUP_SUCCESS(ev_setup_call) assert(ev_setup_call == 0)

// int main(int argc, char *argv[])
int main(void)
{
    char buffer[DEL_BUFFER_SIZE];
    memset(buffer, 0, DEL_BUFFER_SIZE);

    char output[DEL_BUFFER_SIZE];
    memset(output, 0, DEL_BUFFER_SIZE);

    struct DelBuffer dbuffer;
    dbuffer.max_length = DEL_BUFFER_SIZE;
    dbuffer.length = 0;
    dbuffer.buffer = buffer;
    dbuffer.output = output;

    // Compile
    DelCompiler compiler;
    del_compiler_init(&compiler, stderr);
    del_register_function(compiler, NULL, get_charbuffer_size, DEL_INT);
    del_register_function(compiler, &dbuffer, write_char, DEL_UNDEFINED, DEL_BYTE);
    del_register_yielding_function(compiler, &dbuffer, flush_chars, DEL_UNDEFINED);

    // flush_chars
    DelProgram program = del_compile_file(compiler, "example.del");
    if (!program) {
        del_compiler_free(compiler);
        printf("An error occurred during compilation\n");
        return EXIT_FAILURE;
    }
    del_compiler_free(compiler);
    // instruction_array[i] = instructions;

    // Init VMs
    DelVM vm;
    del_vm_init(&vm, stdout, stderr, program);

    struct DelContext dc = { dbuffer, vm };

    // if (run_scripts(argc, argv) == EXIT_FAILURE) {
    //     return EXIT_FAILURE;
    // }
    ev_uint16_t http_port = 8080;
    char *http_addr = "0.0.0.0";
    struct event_base *base;
    struct evhttp *http_server;
    struct event *sig_int;

    base = event_base_new();

    http_server = evhttp_new(base);
    evhttp_set_allowed_methods(http_server, EVHTTP_REQ_GET|EVHTTP_REQ_POST);
    SETUP_SUCCESS(evhttp_bind_socket(http_server, http_addr, http_port));
    SETUP_SUCCESS(evhttp_set_cb(http_server, "/", homepage, &dc));
    SETUP_SUCCESS(evhttp_set_cb(http_server, "/form", form, NULL));
    evhttp_set_gencb(http_server, generic_request_handler, NULL);

    sig_int = evsignal_new(base, SIGINT, signal_cb, base);
    event_add(sig_int, NULL);

    printf("Listening requests on http://%s:%d\n", http_addr, http_port);

    event_base_dispatch(base);

    del_program_free(program);
    del_vm_free(vm);
    evhttp_free(http_server);
    event_free(sig_int);
    event_base_free(base);
}

