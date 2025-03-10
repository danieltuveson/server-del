#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <del.h>
#include "globals.h"
#include "delfuncs.h"

static union DelForeignValue write_char(union DelForeignValue *arguments, void *context)
{
    char byte = arguments[0].byte;
    struct DelBuffer *dbuffer = (struct DelBuffer *) context;
    // Don't want to overflow if too many chars written
    if (dbuffer->length < dbuffer->capacity - 1) {
        dbuffer->buffer[dbuffer->length] = byte;
        dbuffer->length++;
    }
    DEL_NORETURN();
}

static union DelForeignValue get_charbuffer_size(union DelForeignValue *arguments, void *context)
{
    IGNORE(arguments);
    IGNORE(context);
    union DelForeignValue del_buffer_size;
    del_buffer_size.integer = DEL_BUFFER_SIZE;
    return del_buffer_size;
}

// Signals to program to write contents of output buffer and clear it for subsequent use
// Will add a flag here or something soon
static union DelForeignValue flush_chars(union DelForeignValue *arguments, void *context)
{
    IGNORE(arguments);
    IGNORE(context);
    DEL_NORETURN();
}

// static union DelForeignValue get_read_char_count(union DelForeignValue *arguments, void *context)
// {
//     IGNORE(arguments);
//     union DelForeignValue del_buffer_size;
//     del_buffer_size.integer = DEL_BUFFER_SIZE;
//     return del_buffer_size;
// }
// 
// static union DelForeignValue read_char(union DelForeignValue *arguments, void *context)
// {
//     IGNORE(arguments);
//     struct DelBuffer *dbuffer = (struct DelBuffer *) context;
//     union DelForeignValue del_char;
//     if (dbuffer->length < dbuffer->capacity - 1) {
//         del_char.byte = dbuffer->buffer[dbuffer->length];
//     }
//     return del_char;
// }

struct DelBuffer *dbuffer_new(void)
{
    char *buffer = calloc(DEL_BUFFER_SIZE, 1);
    struct DelBuffer *dbuffer = malloc(sizeof(*dbuffer));
    dbuffer->capacity = DEL_BUFFER_SIZE;
    dbuffer->length = 0;
    dbuffer->buffer = buffer;
    return dbuffer;
}

void dbuffer_clear(struct DelBuffer *dbuffer)
{
    memset(dbuffer->buffer, 0, DEL_BUFFER_SIZE);
    dbuffer->length = 0;
}

void dbuffer_free(struct DelBuffer *dbuffer)
{
    free(dbuffer->buffer);
    free(dbuffer);
}

struct DelBufferedReader *dbuffered_reader_new(void)
{
    struct DelBufferedReader *reader = malloc(sizeof(*reader));
    reader->dbuffer = dbuffer_new();
    reader->count = 0;
    return reader;
}

void dbuffered_reader_clear(struct DelBufferedReader *reader)
{
    dbuffer_clear(reader->dbuffer);
    reader->count = 0;
}

void dbuffered_reader_free(struct DelBufferedReader *reader)
{
    dbuffer_free(reader->dbuffer);
    free(reader);
}

void init_with_funcs(DelCompiler *compiler, struct DelBufferedReader *reader,
        struct DelBuffer *output_buffer)
{
    IGNORE(reader);
    del_compiler_init(compiler, stderr);
    // Write functions
    del_register_function(*compiler, NULL, get_charbuffer_size, DEL_INT);
    del_register_function(*compiler, output_buffer, write_char, DEL_UNDEFINED, DEL_BYTE);
    del_register_yielding_function(*compiler, output_buffer, flush_chars, DEL_UNDEFINED);
    // Read functions
    // del_register_function(*compiler, reader, read_byte, DEL_BYTE);
}

