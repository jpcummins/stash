#include "intern.h"

/* forward declare helper functions */
static void append_node(mustache_node_t* parent, mustache_node_t* src);
static mustache_error_t* create_error(mustache_token_t* last_token, enum mustache_token_type expected_token, char* error_text);

/*

template         := block
block            := statement | block
statement        := [text] | [newline] | command
command          := variable | section | inverse_section | partial | comment
variable         := [raw] [identifier] | [identifier]
section          := [section_start] [identifier] block [section_end]
inverse_section  := [inverse_section_start] [identifier] block [section_end]
partial          := [partial] [identifier]

*/

void build_template(mustache_token_t* tokens, mustache_node_t* root, mustache_error_t** error)
{
    root->type = NODE_BLOCK;
    root->execute = execute_block;
    root->optimize = optimize_block;
    append_node(root, build_block(&tokens, error));

    if (tokens) {
        *error = create_error(tokens, TOKEN_ERROR, tokens->text);
    }
}

mustache_node_t* build_block(mustache_token_t** tokens, mustache_error_t** error)
{
    mustache_node_t* block;
    mustache_node_t* next;

    block = mustache_create_node();
    block->type = NODE_BLOCK;
    block->execute = execute_block;
    block->optimize = optimize_block;

    while ((next = build_statement(tokens, error))) {
        append_node(block, next);
    }

    return block;
}

mustache_node_t* build_statement(mustache_token_t** tokens, mustache_error_t** error)
{
    mustache_node_t* next = build_text(tokens, error);

    if (!next) {
        next = build_newline(tokens, error);
    }

    if (!next) {
        next = build_command(tokens, error);
    }

    return next;
}

mustache_node_t* build_text(mustache_token_t** tokens, mustache_error_t** error)
{
    mustache_node_t* next;

    if (!is_token(*tokens, TOKEN_TEXT)) {
        return NULL;
    }

    next = mustache_create_node();
    next->type = NODE_TEXT;
    next->token = pop_token(tokens);
    next->execute = execute_text;
    next->optimize = optimize_text;
    return next;
}

mustache_node_t* build_newline(mustache_token_t** tokens, mustache_error_t** error)
{
    mustache_node_t* next;

    if (!is_token(*tokens, TOKEN_NEWLINE)) {
        return NULL;
    }

    next = mustache_create_node();
    next->type = NODE_NEWLINE;
    next->token = pop_token(tokens);
    next->execute = execute_newline;
    next->optimize = optimize_newline;
    return next;
}

mustache_node_t* build_command(mustache_token_t** tokens, mustache_error_t** error)
{
    mustache_node_t* next = build_raw_variable(tokens, error);

    if (!next) {
        next = build_variable(tokens, error);
    }

    if (!next) {
        next = build_section(tokens, error);
    }

    if (!next) {
        next = build_partial(tokens, error);
    }

    if (!next) {
        next = build_comment(tokens, error);
    }

    return next;
}

mustache_node_t* build_raw_variable(mustache_token_t** tokens, mustache_error_t** error)
{
    mustache_node_t* next;

    if (!is_token(*tokens, TOKEN_RAW)) {
        return NULL;
    }

    free_token(pop_token(tokens));

    if (!is_token(*tokens, TOKEN_IDENTIFIER)) {
        *error = create_error(*tokens, TOKEN_IDENTIFIER, "Expected identifier");
    }

    next = mustache_create_node();
    next->type = NODE_RAW_VARIABLE;
    next->token = pop_token(tokens);
    next->execute = execute_raw_variable;
    next->optimize = optimize_variable;
    return next;
}

mustache_node_t* build_variable(mustache_token_t** tokens, mustache_error_t** error)
{
    mustache_node_t* next;

    if (!is_token(*tokens, TOKEN_IDENTIFIER)) {
        return NULL;
    }

    next = mustache_create_node();
    next->type = NODE_VARIABLE;
    next->token = pop_token(tokens);
    next->execute = execute_variable;
    next->optimize = optimize_variable;
    return next;
}

mustache_node_t* build_section(mustache_token_t** tokens, mustache_error_t** error)
{
    mustache_node_t* section;
    mustache_token_t* token;
    bool is_inverted;

    if (!is_token(*tokens, TOKEN_SECTION_START) &&
        !is_token(*tokens, TOKEN_ISECTION_START)) {
        return NULL;
    }

    token = pop_token(tokens);
    is_inverted = token->type == TOKEN_ISECTION_START;
    free_token(token);

    if (!is_token(*tokens, TOKEN_IDENTIFIER)) {
        *error = create_error(*tokens, TOKEN_IDENTIFIER, "Expected identifier");
        return NULL;
    }

    section = mustache_create_node();
    section->type = is_inverted ? NODE_INVERTED_SECTION : NODE_SECTION;
    section->token = pop_token(tokens);
    section->execute = execute_section;
    section->optimize = optimize_section;
    append_node(section, build_block(tokens, error));

    if (!is_token(*tokens, TOKEN_SECTION_END)) {
        *error = create_error(*tokens, TOKEN_SECTION_END, "Expected section end");
        return NULL;
    }

    free_token(pop_token(tokens));

    if (!is_token(*tokens, TOKEN_IDENTIFIER)) {
        *error = create_error(*tokens, TOKEN_IDENTIFIER, "Expected identifier");
        return NULL;
    }

    free_token(pop_token(tokens));

    return section;
}

mustache_node_t* build_partial(mustache_token_t** tokens, mustache_error_t** error)
{
    mustache_node_t* node;

    if (!is_token(*tokens, TOKEN_PARTIAL)) {
        return NULL;
    }

    free_token(pop_token(tokens));

    if (!is_token(*tokens, TOKEN_IDENTIFIER)) {
        *error = create_error(*tokens, TOKEN_IDENTIFIER, "Expected identifier");
        return NULL;
    }

    node = mustache_create_node();
    node->type = NODE_PARTIAL;
    node->token = pop_token(tokens);
    node->execute = execute_partial;
    node->optimize = optimize_partial;
    return node;
}

mustache_node_t* build_comment(mustache_token_t** tokens, mustache_error_t** error)
{
    mustache_node_t* node;

    if (!is_token(*tokens, TOKEN_COMMENT)) {
        return NULL;
    }

    node = mustache_create_node();
    node->type = NODE_COMMENT;
    node->token = pop_token(tokens);
    node->execute = execute_comment;
    node->optimize = optimize_comment;
    return node;
}


/* Helper functions */

static void append_node(mustache_node_t* parent, mustache_node_t* src)
{
    mustache_node_t* child = parent->first_child;

    if (!child) {
        parent->first_child = src;
    }
    else {
        while (child->next_sibling) {
            child = child->next_sibling;
        }
        child->next_sibling = src;
        src->prv_sibling = child;
    }

    src->parent = parent;
}

static mustache_error_t* create_error(mustache_token_t* last_token, enum mustache_token_type expected_token, char* error_text)
{
    mustache_error_t* error = mustache_malloc(sizeof(mustache_error_t));
    error->last_token = last_token;
    error->error_text = error_text;
    return error;
}