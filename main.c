#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <event2/buffer.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/keyvalq_struct.h>
#include <event2/buffer.h>
#include <uuid/uuid.h>
#include <del.h>
#include "html.h"
#include "globals.h"
#include "delfuncs.h"
#include "sql.h"
#include "route.h"

#define VM_COUNT 100


static void signal_cb(evutil_socket_t fd, short event, void *arg)
{
    IGNORE(event);
    fprintf(stderr, "%s signal received\n", strsignal(fd));
    event_base_loopbreak(arg);
}

// Most libevent http functions return 0 on success
// Can use this for setup functions that we expect to succeed before continuing
#define SETUP_SUCCESS(ev_setup_call) assert(ev_setup_call == 0)

// Reduce boilerplate
#define TOP_LEVEL_ROUTE(str, func) evhttp_set_cb(http_server, str, func, &globals)

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

    // Top level routes
    TOP_LEVEL_ROUTE("/", route_index);
    TOP_LEVEL_ROUTE("/new-survey", route_new_survey);
    // TOP_LEVEL_ROUTE("/static/css/style.css", route_css_style);
    // ROUTE("/static", send_file_to_user);
    // evhttp_set_gencb(http_server, send_file_to_user, NULL);
    evhttp_set_gencb(http_server, route_dynamic, &globals);

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

