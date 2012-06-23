#include "intern.h"

void set_start_delimiter(char* delimiter, scanner_data_t* data)
{
    data->statement_start = strdup(delimiter);
}

void use_next_end_delimiter(scanner_data_t* data)
{
    data->statement_end = data->next_statement_end;
    data->next_statement_end = "";
}

void set_next_end_delimiter(char* delimiter, scanner_data_t* data)
{
    data->statement_end = strdup(delimiter);
}

mustache_token_t* pop_token(mustache_token_t** head)
{
    mustache_token_t* token = *head;
    *head = token->next;
    return token;
}

void reverse(mustache_token_t** head)
{
    mustache_token_t* prv = NULL;
    mustache_token_t* current = *head;

    while (current) {
        mustache_token_t* next = current->next;
        current->next = prv;
        prv = current;
        current = next;
    }

    *head = prv;
}

bool is_token(mustache_token_t* head, enum mustache_token_type type)
{
    if (!head) {
        return false;
    }

    return head->type == type;
}

void free_token(mustache_token_t* token)
{
    if (!token) {
        return;
    }

    if (*token->text) {
        free(token->text);
    }

    free(token);
}

bool is_string_in_buffer(char* string, int string_len, char* buffer, int buffer_len)
{
    if (buffer_len < string_len) {
        return false;
    }

    return strncmp(&buffer[buffer_len - string_len], string, string_len) == 0;
}

mustache_token_t* create_token(enum mustache_token_type type, char* text)
{
    mustache_token_t* token = malloc(sizeof(mustache_token_t));
    char* copied_text = strdup(text);

    if (token) {
        token->type = type;
        token->text = copied_text;
        token->next = NULL;
    }

    return token;
}

void push_token(enum mustache_token_type type, char* text, yyscan_t yyscanner)
{
    scanner_data_t* data = (scanner_data_t*) yyget_extra(yyscanner);
    mustache_token_t* token = create_token(type, text);

    if (data->head_token) {
        token->next = data->head_token;
    }

    data->head_token = token;
}

void push_text(yyscan_t yyscanner)
{
    // Get the accumulated text
    scanner_data_t* data = (scanner_data_t*) yyget_extra(yyscanner);

    // If the buffer contains text, push the token and reset buffer
    if (data->buffer[0]) {
        push_token(TOKEN_TEXT, data->buffer, yyscanner);
        memset(data->buffer, '\0', data->buffer_size);
        data->buffer_index = 0;
    }
}

void push_comment(yyscan_t yyscanner)
{
    // Get the accumulated text
    scanner_data_t* data = (scanner_data_t*) yyget_extra(yyscanner);

    // If the buffer contains text, push the token and reset buffer
    if (data->buffer[0]) {
        push_token(TOKEN_COMMENT, data->buffer, yyscanner);
        memset(data->buffer, '\0', data->buffer_size);
        data->buffer_index = 0;
    }
}

void push_error(yyscan_t yyscanner)
{
    return;
}

int get_tokens(char* input, mustache_token_t** tokens)
{
    yyscan_t scanner;
    scanner_data_t data;
    data.head_token = NULL;
    data.buffer_size = sizeof(char) * strlen(input) + 1;
    data.buffer = malloc(data.buffer_size);
    memset(data.buffer, '\0', data.buffer_size);
    data.buffer_index = 0;
    data.statement_start = "{{";
    data.statement_end = "}}";
    data.next_statement_end = "";

    yylex_init_extra(&data, &scanner);
    yy_scan_string(input, scanner);
    yylex(scanner);
    yylex_destroy(scanner);
    *tokens = data.head_token;
    free(data.buffer);
    reverse(tokens);

    return true;
}
