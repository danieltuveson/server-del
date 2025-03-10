#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include "sql.h"
#include "globals.h"

bool globals_init(struct Globals *globals)
{
    const char *db_name = "data/database.db";
    struct Database *db = db_new(db_name);
    if (!db) {
        return false;
    }
    globals->db = db;
    return true;
}

void globals_cleanup(struct Globals *globals)
{
    db_free(globals->db);
}

