#ifndef GLOBALS_H
#define GLOBALS_H

#define IGNORE(ignored) (void)ignored
#define MAX_QUESTIONNAIRE_NAME_LENGTH 31
#define MAX_QUESTIONNAIRE_NAME_LENGTH_STR "31"

struct Globals {
    struct Database *db;
};

bool globals_init(struct Globals *globals);
void globals_cleanup(struct Globals *globals);

#endif

