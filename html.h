#ifndef HTML_H
#define HTML_H

#include "globals.h"

// Functions for printing HTML to libevent buffers

void html_index_start(struct evbuffer *buff);
void html_index_end(struct evbuffer *buff, bool finished_early);
void html_index_row(struct evbuffer *buff, char *uuid_str, char *s_name);
void html_new_survey(struct evbuffer *buff);

// Error pages
void html_404_notfound(struct evbuffer *buff);
void html_500_internal(struct evbuffer *buff);

#endif

