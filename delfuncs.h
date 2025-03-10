#ifndef DELFUNCS_H
#define DELFUNCS_H

struct DelBuffer {
    int capacity;
    int length;
    char *buffer;
};

struct DelBufferedReader {
    int count;
    struct DelBuffer *dbuffer;
};

struct DelContext {
    struct DelBufferedReader *reader;
    struct DelBuffer *output_buffer;
    DelVM vm;
};

// DelBuffer
struct DelBuffer *dbuffer_new(void);
void dbuffer_clear(struct DelBuffer *dbuffer);
void dbuffer_free(struct DelBuffer *dbuffer);

// DelBufferedReader
struct DelBufferedReader *dbuffered_reader_new(void);
void dbuffered_reader_clear(struct DelBufferedReader *reader);
void dbuffered_reader_free(struct DelBufferedReader *reader);

// Misc
void init_with_funcs(DelCompiler *compiler, struct DelBufferedReader *reader,
        struct DelBuffer *output_buffer);

#define DEL_BUFFER_SIZE 4096
#endif

