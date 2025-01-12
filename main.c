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

// Example del program
int run_scripts(int argc, char *argv[])
{
    // if (argc != 2) {
    if (argc < 2) {
        printf("Error: expecting input file\nExample usage:\n"
                "./server hello_world.del\n");
        return EXIT_FAILURE;
    }
    // DelInstructions instructions_array[VM_COUNT];
    // DelVM vm_array[VM_COUNT];

    for (int i = 1; i < argc; i++) {
        // Compile
        DelAllocator allocator = del_allocator_new();
        DelInstructions instructions = del_read_and_compile(allocator, argv[i]);
        if (!instructions) {
            del_allocator_freeall(allocator);
            printf("An error occurred during compilation\n");
            return EXIT_FAILURE;
        }
        del_allocator_freeall(allocator);
        // instruction_array[i] = instructions;

        // Init VMs
        DelVM vm;
        del_vm_init(&vm, instructions);

        // Run
        del_vm_execute(vm);
        if (del_vm_status(vm) == VM_STATUS_ERROR) {
            del_vm_free(vm);
            del_instructions_free(instructions);
            return EXIT_FAILURE;
        }
        del_vm_free(vm);
        del_instructions_free(instructions);
    }

    return EXIT_SUCCESS;
}

// libevent helpers
#define IGNORE(ctx) (void)ctx

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
    struct evbuffer *reply = evbuffer_new();
    evbuffer_add_printf(reply, html_homepage);
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

int main(int argc, char *argv[])
{
    if (run_scripts(argc, argv) == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }
    ev_uint16_t http_port = 8080;
    char *http_addr = "0.0.0.0";
    struct event_base *base;
    struct evhttp *http_server;
    struct event *sig_int;

    base = event_base_new();

    http_server = evhttp_new(base);
    evhttp_set_allowed_methods(http_server, EVHTTP_REQ_GET|EVHTTP_REQ_POST);
    SETUP_SUCCESS(evhttp_bind_socket(http_server, http_addr, http_port));
    SETUP_SUCCESS(evhttp_set_cb(http_server, "/", homepage, NULL));
    SETUP_SUCCESS(evhttp_set_cb(http_server, "/form", form, NULL));
    evhttp_set_gencb(http_server, generic_request_handler, NULL);

    sig_int = evsignal_new(base, SIGINT, signal_cb, base);
    event_add(sig_int, NULL);

    printf("Listening requests on http://%s:%d\n", http_addr, http_port);

    event_base_dispatch(base);

    evhttp_free(http_server);
    event_free(sig_int);
    event_base_free(base);
}

