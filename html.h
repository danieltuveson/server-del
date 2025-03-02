#ifndef HTML_H
#define HTML_H

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
                css("background-color", "#b1c0fc"))\
            attr("aria-label", "Main menu"),\
            a(attr("href", "/"), "Home")\
            a(attr("href", "/form"), "Form")\
        )\
    )

#define html_boilerplate(content) "<!DOCTYPE html>"\
    "<html>"\
    "<body>"\
    navigation\
    "<h1>Story Engine</h1>"\
    content\
    "</body>"\
    "</html>"

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


const char html_404[] = html_boilerplate(h2("", "404 - Page Not Found"));
const char html_500[] = html_boilerplate(h2("", "500 - Internal Server Error"));

// undef tags
#undef tag_self_close
#undef tag
#undef html
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

