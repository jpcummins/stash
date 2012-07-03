#include <stddef.h>
#include <stdbool.h>

typedef struct mustache_context mustache_context_t;
typedef struct mustache_value mustache_value_t;
typedef struct mustache_node mustache_node_t;
typedef struct mustache_token mustache_token_t;
typedef struct mustache_error mustache_error_t;

enum mustache_token_type {
    TOKEN_RAW,
    TOKEN_SECTION_START,
    TOKEN_ISECTION_START,
    TOKEN_SECTION_END,
    TOKEN_COMMENT,
    TOKEN_TEXT,
    TOKEN_NEWLINE,
    TOKEN_PARTIAL,
    TOKEN_IDENTIFIER,
    TOKEN_ERROR
};

enum mustache_node_type {
    NODE_UNKNOWN,
    NODE_BLOCK,
    NODE_COMMENT,
    NODE_TEXT,
    NODE_NEWLINE,
    NODE_RAW_VARIABLE,
    NODE_VARIABLE,
    NODE_SECTION,
    NODE_INVERTED_SECTION,
    NODE_PARTIAL
};

enum mustache_value_type {
    VALUE_LIST,
    VALUE_OBJECT,
    VALUE_VALUE
};

union mustache_value_data {
    mustache_context_t** list;
    mustache_context_t* object;
    char* value;
};

struct mustache_token {
    mustache_token_t* next;
    mustache_token_t* prv;
    enum mustache_token_type type;
    char* text;
};

struct mustache_value {
    enum mustache_value_type type;
    union mustache_value_data data;
    size_t length;
    void (*destroy) (mustache_value_t* value);
};

struct mustache_error {
    mustache_token_t* last_token;
    enum mustache_token_type expected_token;
    char* error_text;
};

struct mustache_context {

    mustache_value_t* (*get_data) (
        char* key,
        void* data,
        mustache_context_t* lookup_context,
        mustache_context_t* execution_context);

    mustache_node_t* (*get_partial) (
        char* key,
        void* data);

    void (*destroy) (
        mustache_context_t* ctx);

    mustache_context_t* parent;
    void* data;
    void* partials;
    char* prefix;
    void* custom;
    bool debug_mode;
};

struct mustache_node {
    enum mustache_node_type type;
    mustache_node_t* parent;
    mustache_node_t* first_child;
    mustache_node_t* prv_sibling;
    mustache_node_t* next_sibling;
    mustache_token_t* token;

    bool (*execute)(
        mustache_node_t* node,
        mustache_context_t* ctx);

    bool (*optimize)(
        mustache_node_t* node);
};

mustache_node_t* mustache_create_node();

void mustache_destroy_node(mustache_node_t* node);
bool mustache_build_template(char* tmp, mustache_node_t* root, mustache_error_t** error);
bool mustache_execute_template(mustache_node_t* node, mustache_context_t* ctx);

/* Functions to be defined by each implementation */

void* mustache_malloc(long size);
void mustache_free(void* ptr);
bool mustache_write_to_buffer(mustache_context_t* m_ctx, char* data, size_t data_length);
