#ifndef HTML_H
#define HTML_H

/* 
 * A cursed header for defining HTML template macros
 *
 * Pray that you don't make any typos when using these
 *
 */

// Attribute macros
#define attr(name, content) " " name "=\"" content "\" "
#define css(property, value) property ": " value ";"

// Tag macros
#define tag_self_close(tagname, attrs) "<" tagname " " attrs " >"
#define tag(tagname, attrs, body) tag_self_close(tagname, attrs) body "</" tagname ">"

#define html(attrs, body) tag("html", attrs, body)
#define body(attrs, body) tag("body", attrs, body)
#define div(attrs, body) tag("div", attrs, body)
#define span(attrs, body) tag("span", attrs, body)
#define p(attrs, body) tag("p", attrs, body)
#define form(attrs, body) tag("form", attrs, body)
#define input(attrs) tag_self_close("input", attrs)
#define br "<br>"
#define label(attrs, body) tag("label", attrs, body)
#define a(attrs, body) tag("a", attrs, body)
#define nav(attrs, body) tag("nav", attrs, body)
#define h1(attrs, body) tag("h1", attrs, body)
#define h2(attrs, body) tag("h2", attrs, body)
#define h3(attrs, body) tag("h3", attrs, body)
#define h4(attrs, body) tag("h4", attrs, body)

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

#define boilerplate(content) "<!DOCTYPE html>"\
    "<html>"\
    "<body>"\
    navigation\
    "<h1>Story Engine</h1>"\
    content\
    "</body>"\
    "</html>"

const char html_homepage[] = boilerplate(h2("", "Welcome!"));

const char html_form[] =
    boilerplate(
        h2("", "HTML Forms")
        form(attr("action", "/form"), 
            label(attr("for", "fname"), "First name:")
            input(attr("type", "text") attr("id", "fname") attr("value", "John"))
            br
            label(attr("for", "lname"), "Last name:")
            input(attr("type", "text") attr("id", "lname") attr("value", "Doe"))
            br br
            input(attr("type", "submit") attr("value", "Submit"))
        )
        p("",
            "If you click the \"Submit\" button, the form-data will be sent to a page called '/'."
        )
    );

const char html_404[] = boilerplate("<h1>404 - Page Not Found");

// undef tags
#undef tag_self_close
#undef tag
#undef html
#undef body
#undef div
#undef span
#undef p
#undef form
#undef input
#undef br 
#undef label
#undef a
#undef nav
#undef h1
#undef h2
#undef h3
#undef h4

// undef attributes
#undef attr
#undef css
#endif

// undef rest
#undef boilerplate
#undef navigation
