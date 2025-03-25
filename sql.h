#ifndef SQL_H
#define SQL_H

#include <stdbool.h>
#include <sqlite3.h>
#include <dsalloc.h>

// ************************************************
// ********** Global Database Structures **********
// ************************************************

// Do not change the order of these values
// Order must match order in which they are declared at `const char *queries[]`
enum PrepStmtType {
    PREP_CREATE_SURVEY,
    PREP_GET_SURVEY_NAME,
    PREP_GET_SURVEY_PROGRAM,
    // Not a real statement - just to keep track of how many exist
    // This must always be the *last* statement in this list
    PREP_TOTAL_PREPPED_STMTS
};

struct PrepStmt {
    enum PrepStmtType type;
    sqlite3_stmt *pp_stmt;
};

enum DatabaseStatus {
    DB_ERROR,
    DB_IN_PROGRESS,
    DB_FINISHED
};

struct Database {
    sqlite3 *sql_db;
    struct PrepStmt **stmts;
};

struct Database *db_new(const char *db_name);
void db_free(struct Database *db);

// ************************************************
// ******************** Models ********************
// ************************************************

struct SurveyRecord {
    int survey_id;
    char *survey_uuid;
    char *name;
    char *program; // Source
    char *code; // Bytecode
};

// Creates record in 'surveys' table
bool db_new_survey(struct Database *db, const char *s_name);

// Gets list of name-uuid pairs
sqlite3_stmt *db_prep_get_survey_name(struct Database *db);
enum DatabaseStatus db_get_survey_name(sqlite3_stmt *pp_stmt, struct SurveyRecord *sn);
void db_finish_get_survey_name(struct Database *db);

// Get program for given survey name
sqlite3_stmt *db_prep_get_survey_program(struct Database *db);
enum DatabaseStatus db_get_survey_program(sqlite3_stmt *pp_stmt, struct SurveyRecord *sr,
        char *s_name);
void db_finish_get_survey_program(struct Database *db);


#endif

