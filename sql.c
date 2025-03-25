#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <uuid/uuid.h>
#include <sqlite3.h>
#include "globals.h"
#include "sql.h"

// ***************************************************
// *************** Prepared Statements ***************
// ***************************************************

// NOTE: Order here must *exactly* match order of enum PrepStmtTypes
const char *queries[] = {
    // PREP_CREATE_SURVEY:
    "INSERT INTO surveys VALUES(NULL, ?, ?, NULL, NULL);",
    // PREP_GET_SURVEY_NAME:
    "SELECT survey_uuid, name FROM surveys;",
    // PREP_GET_SURVEY_PROGRAM:
    "SELECT name, code FROM surveys where name = ?;"
};

static void free_prepared_stmts(struct PrepStmt **stmts, int count)
{
    for (int i = 0; i < count; i++) {
        fprintf(stderr, "SQL: finalizing statement %d: `%s`\n", i, queries[i]);
        sqlite3_finalize(stmts[i]->pp_stmt);
        free(stmts[i]);
    }
    free(stmts);
}

static struct PrepStmt **initialize_prepared_stmts(sqlite3 *sql_db)
{
    fprintf(stderr, "SQL: initializing prepared statements\n");
    struct PrepStmt **stmts = calloc(PREP_TOTAL_PREPPED_STMTS, sizeof(*stmts));

    for (int i = 0; i < PREP_TOTAL_PREPPED_STMTS; i++) {
        fprintf(stderr, "SQL: preparing statement %d: `%s`\n", i, queries[i]);
        sqlite3_stmt *pp_stmt = NULL;
        int rc = sqlite3_prepare_v2(sql_db, queries[i], -1, &pp_stmt, NULL);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL error (%d): failed to prepare statement %d\n", rc, i);
            free_prepared_stmts(stmts, i);
            return NULL;
        }
        stmts[i] = malloc(sizeof(**stmts));
        stmts[i]->type = i;
        stmts[i]->pp_stmt = pp_stmt;
    }
    return stmts;
}

// ***************************************************
// *************** Database methods ******************
// ***************************************************

struct Database *db_new(const char *db_name)
{
    // Open / create sqlite database
    struct Database *db = malloc(sizeof(*db));
    sqlite3 *sql_db;
    int rc = sqlite3_open(db_name, &sql_db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: can't open database: %s\n", sqlite3_errmsg(sql_db));
        goto err;
    }

    // Initialize surveys table
    fprintf(stderr, "SQL: creating 'surveys' table\n");
    assert(sql_db != NULL);
    const char *query_str =
        "CREATE TABLE IF NOT EXISTS surveys ("
            "survey_id INTEGER PRIMARY KEY,"
            "survey_uuid text,"
            "name text UNIQUE,"
            "program blob,"
            "code blob"
        ");";
    char *err_msg = NULL;
    rc = sqlite3_exec(sql_db, query_str, NULL, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error (%d): %s\n", rc, err_msg);
        sqlite3_free(err_msg);
        goto err;
    }

    // Initialize prepared statements
    struct PrepStmt **stmts = initialize_prepared_stmts(sql_db);
    if (!stmts) {
        goto err;
    }
    
    db->stmts = stmts;
    db->sql_db = sql_db;
    return db;
err:
    free(db);
    sqlite3_close(sql_db);
    return NULL;
}

void db_free(struct Database *db)
{
    free_prepared_stmts(db->stmts, PREP_TOTAL_PREPPED_STMTS);
    sqlite3_close(db->sql_db);
    free(db);
}

// ***************************************************
// ************* Route-specific queries **************
// ***************************************************

bool db_new_survey(struct Database *db, const char *s_name)
{
    sqlite3 *sql_db = db->sql_db;

    fprintf(stderr, "SQL: sql_new_survey\n");
    assert(sql_db != NULL);
    assert(s_name != NULL);
    int len = strlen(s_name);
    if (len > MAX_SURVEY_NAME_LENGTH) {
        fprintf(stderr, "SQL: variable name too long\n");
        return false;
    } else if (len == 0) {
        fprintf(stderr, "SQL: variable name is empty\n");
        return false;
    }

    // Create uuid
    uuid_t uuid;
    uuid_generate(uuid);

    // uuid string
    char survey_uuid[UUID_STR_LEN] = {0};
    uuid_unparse(uuid, survey_uuid);
    fprintf(stderr, "SQL: uuid: %s\n", survey_uuid);

    // Prepare sql
    // TODO: Check if row exists before trying to create it
    sqlite3_stmt *pp_stmt = db->stmts[PREP_CREATE_SURVEY]->pp_stmt;

    // Bind uuid
    int rc = sqlite3_bind_text(pp_stmt, 1, survey_uuid, -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error (%d): failed to bind uuid to statement\n", rc);
        goto fail;
    }
    // Bind survey name
    rc = sqlite3_bind_text(pp_stmt, 2, s_name, -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error (%d): failed to bind s_name to statement\n", rc);
        goto fail;
    }

    // Execute
    rc = sqlite3_step(pp_stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "SQL error (%d): error executing SQL statement\n", rc);
        goto fail;
    }

    // Cleanup 
    sqlite3_reset(pp_stmt);
    return true;
fail:
    sqlite3_reset(pp_stmt);
    return false;
}

sqlite3_stmt *db_prep_get_survey_name(struct Database *db)
{
    fprintf(stderr, "SQL: db_prep_get_survey_name\n");
    assert(db != NULL);

    sqlite3_stmt *pp_stmt = db->stmts[PREP_GET_SURVEY_NAME]->pp_stmt;
    assert(pp_stmt != NULL);

    return pp_stmt;
}

void db_finish_get_survey_name(struct Database *db)
{
    sqlite3_stmt *pp_stmt = db->stmts[PREP_GET_SURVEY_NAME]->pp_stmt;
    sqlite3_reset(pp_stmt);
}

enum DatabaseStatus db_get_survey_name(sqlite3_stmt *pp_stmt, struct SurveyRecord *sr)
{
    fprintf(stderr, "SQL: db_get_survey_name\n");
    assert(pp_stmt != NULL);

    int rc = sqlite3_step(pp_stmt);
    if (rc == SQLITE_DONE) {
        return DB_FINISHED;
    } else if (rc != SQLITE_ROW) {
        return DB_ERROR;
    }

    int length = 0;
    const char *survey_uuid = (const char *)sqlite3_column_text(pp_stmt, 0);
    assert(survey_uuid != NULL);
    length = strlen(survey_uuid);
    fprintf(stderr, "SQL: uuid (%d): '%s'\n", length, survey_uuid);
    assert(length == UUID_STR_LEN - 1); 
    strcpy(sr->survey_uuid, survey_uuid);

    const char *s_name = (const char *)sqlite3_column_text(pp_stmt, 1);
    assert(s_name != NULL);
    length = strlen(s_name);
    fprintf(stderr, "SQL: s_name (%d): '%s'\n", length, s_name);
    assert(length <= MAX_SURVEY_NAME_LENGTH);
    strcpy(sr->name, s_name);

    return DB_IN_PROGRESS;
}

sqlite3_stmt *db_prep_get_survey_program(struct Database *db)
{
    fprintf(stderr, "SQL: db_prep_get_survey_program\n");
    assert(db != NULL);

    sqlite3_stmt *pp_stmt = db->stmts[PREP_GET_SURVEY_PROGRAM]->pp_stmt;
    assert(pp_stmt != NULL);

    return pp_stmt;
}

void db_finish_get_survey_program(struct Database *db)
{
    sqlite3_stmt *pp_stmt = db->stmts[PREP_GET_SURVEY_PROGRAM]->pp_stmt;
    sqlite3_reset(pp_stmt);
}

enum DatabaseStatus db_get_survey_program(sqlite3_stmt *pp_stmt, struct SurveyRecord *sr,
        char *s_name)
{
    fprintf(stderr, "SQL: db_get_survey_program\n");
    assert(pp_stmt != NULL);

    // Bind survey name
    int rc = sqlite3_bind_text(pp_stmt, 1, s_name, -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error (%d): failed to bind s_name to statement\n", rc);
        goto error;
    }

    rc = sqlite3_step(pp_stmt);
    if (rc != SQLITE_ROW) {
        fprintf(stderr, "SQL error (%d): could not find survey with `name = '%s'`\n", rc, s_name);
        goto error;
    }

    const char *program = (const char *)sqlite3_column_text(pp_stmt, 1);
    assert(program != NULL);
    int length = strlen(program);
    if (length > 20) {
        fprintf(stderr, "SQL: program for %s: \n%.20s...\n", s_name, program);
    } else {
        fprintf(stderr, "SQL: program for %s: \n%s\n", s_name, program);
    }
    strcpy(sr->program, program);
    strcpy(sr->name, s_name);

    assert(sqlite3_step(pp_stmt) == DB_FINISHED);

    sqlite3_reset(pp_stmt);
    return DB_FINISHED;
error:
    sqlite3_reset(pp_stmt);
    return DB_ERROR;
}

