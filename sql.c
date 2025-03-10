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
    // PREP_NEW_QUEST:
    "INSERT INTO surveys VALUES(NULL, ?, ?, NULL, NULL);",
    // PREP_EXAMPLE_2:
    "SELECT * FROM surveys;",
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
        fprintf(stderr, "SQL error: %s\n", err_msg);
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

bool db_new_questionnaire(struct Database *db, const char *q_name)
{
    sqlite3 *sql_db = db->sql_db;

    fprintf(stderr, "SQL: sql_new_questionnaire\n");
    assert(sql_db != NULL);
    assert(q_name != NULL);
    int len = strlen(q_name);
    if (len > MAX_QUESTIONNAIRE_NAME_LENGTH) {
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
    char uuid_str[37] = {0};
    uuid_unparse(uuid, uuid_str);
    fprintf(stderr, "SQL: uuid: %s\n", uuid_str);

    // Prepare sql
    // TODO: Check if row exists before trying to create it
    sqlite3_stmt *pp_stmt = db->stmts[PREP_NEW_QUEST]->pp_stmt;

    // Bind uuid
    int rc = sqlite3_bind_text(pp_stmt, 1, uuid_str, -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error %d: failed to bind uuid to statement\n", rc);
        goto fail;
    }
    // Bind questionnaire name
    rc = sqlite3_bind_text(pp_stmt, 2, q_name, -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error %d: failed to bind q_name to statement\n", rc);
        goto fail;
    }

    // Execute
    rc = sqlite3_step(pp_stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "SQL error %d: error executing SQL statement\n", rc);
        goto fail;
    }

    // Cleanup 
    sqlite3_reset(pp_stmt);
    return true;
fail:
    sqlite3_reset(pp_stmt);
    return false;
}

