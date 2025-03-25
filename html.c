#include <stdio.h>
#include <event2/buffer.h>
#include "html.h"

/* 
 * Cursed HTML template macros
 *
 * Pray that you don't make any typos when using these
 *
 */

// Attribute macros
#define attr(name, content) " " name "=\"" content "\""
#define css(property, value) property ": " value ";"

// Generic tag macros
#define tag_self_close(tagname, attrs) "<" tagname attrs ">"
#define tag(tagname, attrs, body) tag_self_close(tagname, attrs) body "</" tagname ">"

// Specific tags
#define html(attrs, body)     tag("html", attrs, body)
#define head(attrs, body)     tag("head", attrs, body)
#define title(attrs, body)    tag("title", attrs, body)
#define meta(attrs)           tag_self_close("meta", attrs)
#define body(attrs, body)     tag("body", attrs, body)
#define div(attrs, body)      tag("div", attrs, body)
#define span(attrs, body)     tag("span", attrs, body)
#define p(attrs, body)        tag("p", attrs, body)
#define form(attrs, body)     tag("form", attrs, body)
#define fieldset(attrs, body) tag("fieldset", attrs, body)
#define input(attrs)          tag_self_close("input", attrs)
#define br                    tag_self_close("br", "")
#define label(attrs, body)    tag("label", attrs, body)
#define ul(attrs, body)       tag("ul", attrs, body)
#define ol(attrs, body)       tag("ol", attrs, body)
#define li(attrs, body)       tag("li", attrs, body)
#define a(attrs, body)        tag("a", attrs, body)
#define nav(attrs, body)      tag("nav", attrs, body)
#define h1(attrs, body)       tag("h1", attrs, body)
#define h2(attrs, body)       tag("h2", attrs, body)
#define h3(attrs, body)       tag("h3", attrs, body)
#define h4(attrs, body)       tag("h4", attrs, body)
#define style(attrs, body)    tag("style", attrs, body)
#define link(attrs)           tag_self_close("link", attrs)
#define script(attrs, body)   tag("script", attrs, body)

// **********************************************
// ********** String constant templates *********
// **********************************************

#define navigation \
    div("", \
        nav(\
            attr("style",\
                css("position","sticky")\
                css("top", "0")\
                css("padding-right",)\
                css("background-color", "#b1c0fc"))\
            attr("aria-label", "Main menu"),\
            a(attr("href", "/") attr("style", css("margin-left", "20px")), "Projects")\
            a(attr("href", "/new-survey") attr("style", css("margin-left", "20px")),\
                "New Project")\
        )\
    )

#define default_head\
        head("", \
            meta(attr("charset", "UTF-8"))\
            meta(\
                attr("name", "viewport")\
                attr("content", "width=device-width, initial-scale=1.0"))\
            meta(\
                attr("http-equiv", "X-UA-Compatible")\
                attr("content", "ie=edge"))\
            meta(\
                attr("name", "google")\
                /* Google thinks "questionnaire" == french */\
                attr("content", "notranslate"))\
            title("", "Survey Engine")\
            link(attr("rel", "stylesheet") attr("href", "static/css/style.css"))\
        )

#define html_boilerplate(content) "<!DOCTYPE html>"\
    html(attr("lang", "en"),\
        default_head \
        body("", \
            navigation\
            h1("", "Survey Engine")\
            content\
        )\
    );

const char html_form_response[] = html_boilerplate(
    h2("", "HTML Form Response")
    div(attr("id", "fname"), "First name: %s")
    div(attr("id", "lname"), "Last name: %s")
    div(attr("id", "yes-no"), "Yes / No: %s")
);

const char template_new_survey[] = html_boilerplate(
    h2("", "New Survey")
    form(attr("action", "/new-survey") attr("method", "POST"),
        label(attr("for", "survey-name"), "Survey name: ")
        input(
            attr("type", "text")
            attr("id", "survey-name")
            attr("name", "survey-name")
            attr("minlength", "1")
            attr("maxlength", MAX_SURVEY_NAME_LENGTH_STR)
            attr("pattern", "[A-Za-z]+\\w*")
            attr("title", "input must begin with a letter and may only contain "
                          "letters, numbers, and underscores")
            attr("required", "")
        )
        br
        br
        input(attr("type", "submit") attr("value", "Create"))
    )
);

// **********************************************
// ************** Actual Functions **************
// **********************************************

const char *fmt_index_start = 
    "<!DOCTYPE html>"
    "<html lang=\"en\">"
    default_head
    "<body>"
    navigation
    h1("", "Survey Engine")
    "<ul>";

void html_index_start(struct evbuffer *buff)
{
    evbuffer_add_printf(buff, fmt_index_start);
}

const char *fmt_index_end = "</ul></body></html>";

void html_index_end(struct evbuffer *buff, bool finished_early)
{
    if (finished_early) {
        evbuffer_add_printf(buff, li("", "..."));
    }
    evbuffer_add_printf(buff, fmt_index_end);
}

const char *fmt_index_row =
    li(attr("id", "%s"), a(attr("href", "/edit-survey/%s"), "%s"));

void html_index_row(struct evbuffer *buff, char *uuid_str, char *s_name)
{
    fprintf(stderr, "HTML: uuid_str: %s, name: %s\n", uuid_str, s_name);
    evbuffer_add_printf(buff, fmt_index_row, uuid_str, s_name, s_name);
}

void html_new_survey(struct evbuffer *buff)
{
    evbuffer_add_printf(buff, template_new_survey);
}

const char template_404[] = html_boilerplate(h2("", "404 - Page Not Found"));

void html_404_notfound(struct evbuffer *buff)
{
    evbuffer_add_printf(buff, template_404);
}

const char template_500[] = html_boilerplate(h2("", "500 - Internal Server Error"));

void html_500_internal(struct evbuffer *buff)
{
    evbuffer_add_printf(buff, template_500);
}

