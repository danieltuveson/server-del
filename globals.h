#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdbool.h>

#define IGNORE(ignored) (void)ignored
// Max survey string length (without trailing null byte)
#define MAX_SURVEY_NAME_LENGTH 31
#define MAX_SURVEY_NAME_LENGTH_STR "31"
// Max uuid string length (without trailing null byte)

struct Globals {
    // Used for saving intermediate paths for dynamic routes
    char *path;
    char *filename;
    // Used for requests that require database
    struct Database *db;
};

bool globals_init(struct Globals *globals);
void globals_cleanup(struct Globals *globals);

#endif

