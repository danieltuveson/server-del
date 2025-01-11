#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <event2/buffer.h>
#include <event2/event.h>
#include <event2/http.h>
#include <del.h>

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

static void generic_request_handler(struct evhttp_request *req, void *ctx)
{
    struct evbuffer *reply = evbuffer_new();

    char str[] = "<h1>404 - Not Found</h1>";
    int i = evbuffer_add_printf(reply, str);
    evhttp_send_reply(req, HTTP_OK, NULL, reply);
    evbuffer_free(reply);
}

static void homepage(struct evhttp_request *req, void *ctx)
{
    struct evbuffer *reply = evbuffer_new();
    char str[] = "<h1>hello!</h1>";
    int i = evbuffer_add_printf(reply, str);
    evhttp_send_reply(req, HTTP_OK, NULL, reply);
    evbuffer_free(reply);
}

static void signal_cb(evutil_socket_t fd, short event, void *arg)
{
    printf("%s signal received\n", strsignal(fd));
    event_base_loopbreak(arg);
}

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
    evhttp_bind_socket(http_server, http_addr, http_port);
    evhttp_set_cb(http_server, "/", homepage, NULL);
    evhttp_set_gencb(http_server, generic_request_handler, NULL);

    sig_int = evsignal_new(base, SIGINT, signal_cb, base);
    event_add(sig_int, NULL);

    printf("Listening requests on http://%s:%d\n", http_addr, http_port);

    event_base_dispatch(base);

    evhttp_free(http_server);
    event_free(sig_int);
    event_base_free(base);
}

