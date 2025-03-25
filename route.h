#ifndef ROUTE_H
#define ROUTE_H

#include <event2/http.h>

#define MAX_FORM_QUERY_STRING 1024

typedef void (*Route)(struct evhttp_request *, void *);
// typedef struct Value *(*ExprParserFunction)(struct Globals *);

// struct Routing {
//     const char *path;
//     void(*cb)(struct evhttp_request *, void *);
// };

void route_404_notfound(struct evhttp_request *req, void *ctx);
void route_500_internal(struct evhttp_request *req, void *ctx);
void route_index(struct evhttp_request *req, void *ctx);
void route_new_survey(struct evhttp_request *req, void *ctx);
void route_css_style(struct evhttp_request *req, void *ctx);
void route_dynamic(struct evhttp_request *req, void *ctx);

#endif

