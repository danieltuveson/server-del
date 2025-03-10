#ifndef HTML_H
#define HTML_H

#include "globals.h"

/* 
 * A cursed header for defining HTML template macros
 *
 * Pray that you don't make any typos when using these
 *
 */

// Attribute macros
#define attr(name, content) " " name "=\"" content "\""
#define css(property, value) property ": " value ";"

// Tag macros
#define tag_self_close(tagname, attrs) "<" tagname attrs ">"
#define tag(tagname, attrs, body) tag_self_close(tagname, attrs) body "</" tagname ">"

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
#define a(attrs, body)        tag("a", attrs, body)
#define nav(attrs, body)      tag("nav", attrs, body)
#define h1(attrs, body)       tag("h1", attrs, body)
#define h2(attrs, body)       tag("h2", attrs, body)
#define h3(attrs, body)       tag("h3", attrs, body)
#define h4(attrs, body)       tag("h4", attrs, body)
#define style(attrs, body)    tag("style", attrs, body)
#define link(attrs, body)     tag("link", attrs, body)
#define script(attrs, body)   tag("script", attrs, body)

#define navigation \
    div("", \
        nav(\
            attr("style",\
                css("position","sticky")\
                css("top", "0")\
                css("padding-right",)\
                css("background-color", "#b1c0fc"))\
            attr("aria-label", "Main menu"),\
            a(attr("href", "/") attr("style", css("margin-left", "20px")), "Home")\
            a(attr("href", "/new-questionnaire") attr("style", css("margin-left", "20px")),\
                "Form")\
        )\
    )

#define html_boilerplate(content) "<!DOCTYPE html>"\
    html(attr("lang", "en"),\
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
            title("", "Questionnaire Engine")\
        )\
        body("", \
            navigation\
            h1("", "Questionnaire Engine")\
            content\
        )\
    );

const char html_template[] = html_boilerplate("%s");

const char html_homepage[] = html_boilerplate(h2("", "Welcome!"));

const char html_form[] = html_boilerplate(
    h2("", "HTML Form")
    form(attr("action", "/form") attr("method", "POST"),
        label(attr("for", "fname"), "First name:")
        input(attr("type", "text") attr("id", "fname") attr("name", "fname") attr("value", "John"))
        br
        label(attr("for", "lname"), "Last name:")
        input(attr("type", "text") attr("id", "lname") attr("name", "lname") attr("value", "Doe"))
        br
        fieldset("",
            div(attr("id", "radio-1"),
                input(attr("type", "radio") attr("id", "yes")
                      attr("name", "yesNo") attr("value", "1"))
                label(attr("for", "yes"), "Yes")
            )
            br
            div(attr("id", "radio-2"),
                input(attr("type", "radio") attr("id", "no")
                      attr("name", "yesNo") attr("value", "2"))
                label(attr("for", "no"), "No")
            )
        )
        br br
        input(attr("type", "submit") attr("value", "Submit"))
    )
    p("",
        "If you click the \"Submit\" button, the form-data will be sent to a page called '/'."
    )
);

const char html_form_response[] = html_boilerplate(
    h2("", "HTML Form Response")
    div(attr("id", "fname"), "First name: %s")
    div(attr("id", "lname"), "Last name: %s")
    div(attr("id", "yes-no"), "Yes / No: %s")
);

const char html_new_questionnaire[] = html_boilerplate(
    h2("", "New Questionnaire")
    form(attr("action", "/new-questionnaire") attr("method", "POST"),
        label(attr("for", "q-name"), "Questionnaire name: ")
        input(
            attr("type", "text")
            attr("id", "q-name")
            attr("name", "q-name")
            attr("minlength", "1")
            attr("maxlength", MAX_QUESTIONNAIRE_NAME_LENGTH_STR)
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

const char html_new_questionnaire_response[] = html_boilerplate(
    h2("", "New questionnaire created")
    div(attr("id", "q-name"), "Questionnaire name: %s")
    div(attr("id", "q-id"), "Questionnaire ID: %s")
);


const char html_404[] = html_boilerplate(h2("", "404 - Page Not Found"));
const char html_500[] = html_boilerplate(h2("", "500 - Internal Server Error"));

// undef tags
#undef tag_self_close
#undef tag
#undef html
#undef head
#undef title
#undef meta
#undef body
#undef div
#undef span
#undef p
#undef form
#undef fieldset
#undef input
#undef br 
#undef label
#undef a
#undef nav
#undef h1
#undef h2
#undef h3
#undef h4
#undef style
#undef link
#undef script

// undef attributes
#undef attr
#undef css

// undef rest
#undef html_boilerplate
#undef navigation

#endif

