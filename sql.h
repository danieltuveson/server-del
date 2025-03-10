#ifndef SQL_H
#define SQL_H

#include <stdbool.h>
#include <sqlite3.h>

// Do not change the order of these values
// Order must match order in which they are declared at `const char *queries[]`
enum PrepStmtType {
    PREP_NEW_QUEST,
    PREP_EXAMPLE_2,
    // Not a real statement - just to keep track of how many exist
    // This must always be the *last* statement in this list
    PREP_TOTAL_PREPPED_STMTS
};

struct PrepStmt {
    enum PrepStmtType type;
    sqlite3_stmt *pp_stmt;
};

struct Database {
    sqlite3 *sql_db;
    struct PrepStmt **stmts;
};

struct Database *db_new(const char *db_name);
void db_free(struct Database *db);
bool db_new_questionnaire(struct Database *db, const char *q_name);

#endif

