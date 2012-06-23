#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "mustache.h"
#include "lex.yy.h"

/* build.c */
void build_template(mustache_token_t* tokens, mustache_node_t* root, mustache_error_t** error);
mustache_node_t* build_block(mustache_token_t** tokens, mustache_error_t** error);
mustache_node_t* build_statement(mustache_token_t** tokens, mustache_error_t** error);
mustache_node_t* build_text(mustache_token_t** tokens, mustache_error_t** error);
mustache_node_t* build_newline(mustache_token_t** tokens, mustache_error_t** error);
mustache_node_t* build_command(mustache_token_t** tokens, mustache_error_t** error);
mustache_node_t* build_raw_variable(mustache_token_t** tokens, mustache_error_t** error);
mustache_node_t* build_variable(mustache_token_t** tokens, mustache_error_t** error);
mustache_node_t* build_section(mustache_token_t** tokens, mustache_error_t** error);
mustache_node_t* build_partial(mustache_token_t** tokens, mustache_error_t** error);
mustache_node_t* build_comment(mustache_token_t** tokens, mustache_error_t** error);

/* execute.h */
bool execute_block(mustache_node_t* node, mustache_context_t* ctx);
bool execute_statement(mustache_node_t* node, mustache_context_t* ctx);
bool execute_text(mustache_node_t* node, mustache_context_t* ctx);
bool execute_newline(mustache_node_t* node, mustache_context_t* ctx);
bool execute_command(mustache_node_t* node, mustache_context_t* ctx);
bool execute_raw_variable(mustache_node_t* node, mustache_context_t* ctx);
bool execute_variable(mustache_node_t* node, mustache_context_t* ctx);
bool execute_section(mustache_node_t* node, mustache_context_t* ctx);
bool execute_partial(mustache_node_t* node, mustache_context_t* ctx);
bool execute_comment(mustache_node_t* node, mustache_context_t* ctx);

/* optimize.h */
bool optimize_block(mustache_node_t* node);
bool optimize_text(mustache_node_t* node);
bool optimize_newline(mustache_node_t* node);
bool optimize_command(mustache_node_t* node);
bool optimize_raw_variable(mustache_node_t* node);
bool optimize_variable(mustache_node_t* node);
bool optimize_section(mustache_node_t* node);
bool optimize_partial(mustache_node_t* node);
bool optimize_comment(mustache_node_t* node);

/* tockens.c */
enum scanner_state {
    TEXT,
    COMMENT,
    STATEMENT
};

typedef struct scanner_data {
  char* buffer;
  int buffer_size;
  int buffer_index;
  mustache_token_t* head_token;

  char* statement_start;
  char* statement_end;
  char* next_statement_end;
} scanner_data_t;


int get_tokens(char* input, mustache_token_t** tokens);
mustache_token_t* pop_token(mustache_token_t** head);
bool is_token(mustache_token_t* head, enum mustache_token_type type);
bool is_string_in_buffer(char* string, int string_len, char* buffer, int buffer_len);
void free_token(mustache_token_t* token);
mustache_token_t* create_token(enum mustache_token_type type, char* text);
void push_token(enum mustache_token_type type, char* text, yyscan_t yyscanner);
void push_text(yyscan_t yyscanner);
void push_comment(yyscan_t yyscanner);
void push_error(yyscan_t yyscanner);
void set_start_delimiter(char* delimiter, scanner_data_t* data);
void use_next_end_delimiter(scanner_data_t* data);
void set_next_end_delimiter(char* delimiter, scanner_data_t* data);