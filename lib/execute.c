#include "intern.h"

/* forward declare helper functions */
static bool html_encode(char* non_terminated_input, size_t length, mustache_context_t* ctx);
static mustache_value_t* get_variable_value(char* key, mustache_context_t* ctx);
static mustache_value_t* get_value(char* key, mustache_context_t* ctx);
bool is_whitespace(mustache_node_t* text_node);

bool execute_block(mustache_node_t* node, mustache_context_t* ctx)
{
    mustache_node_t* child = node->first_child;

    while (child) {
        if (!child->execute(child, ctx)) {
            return false;
        }
        child = child->next_sibling;
    }
    return true;
}

bool execute_text(mustache_node_t* node, mustache_context_t* ctx)
{
    return mustache_write_to_buffer(ctx, node->token->text, strlen(node->token->text));
}

bool execute_newline(mustache_node_t* node, mustache_context_t* ctx)
{
    if (!mustache_write_to_buffer(ctx, node->token->text, strlen(node->token->text))) {
        return false;
    }

    if (*ctx->prefix && node->next_sibling) {
        if (!mustache_write_to_buffer(ctx, ctx->prefix, strlen(ctx->prefix))) {
            return false;
        }
    }

    return true;
}

bool execute_raw_variable(mustache_node_t* node, mustache_context_t* ctx)
{
    mustache_value_t* value = get_variable_value(node->token->text, ctx);
    if (value) {
        if (value->type == VALUE_VALUE) {
            if (!mustache_write_to_buffer(ctx, value->data.value, value->length)) {
                value->destroy(value);
                return false;
            }
        }
        value->destroy(value);
    }

    return true;
}

bool execute_variable(mustache_node_t* node, mustache_context_t* ctx)
{
    mustache_value_t* value = get_variable_value(node->token->text, ctx);
    if (value) {
        if (value->type == VALUE_VALUE) {
            if (!html_encode(value->data.value, value->length, ctx)) {
                printf("html_encode failed\n");
                value->destroy(value);
                return false;
            }
        }
        value->destroy(value);
    }

    return true;
}
size_t foo = 0;

bool execute_section(mustache_node_t* node, mustache_context_t* ctx)
{
    mustache_value_t* value = get_variable_value(node->token->text, ctx);
    mustache_context_t** list_contexts = NULL;
    mustache_context_t* list_context = NULL;
    bool response = true;

    bool is_true;

    // If the variable doesn't exist, don't execute the template
    if (!value && node->type == NODE_SECTION) {
        return true;
    } else if (!value && node->type == NODE_INVERTED_SECTION) {
        return node->first_child->execute(node->first_child, ctx);
    }

    switch (value->type) {
        case VALUE_LIST:
            list_contexts = value->data.list;

            if (node->type == NODE_INVERTED_SECTION) {
                if (value->length) {
                    return true;
                } else {
                    return node->first_child->execute(node->first_child, ctx);
                }
            }

            for (size_t i = 0; i < value->length; i++) {
                list_context = list_contexts[i];
                list_context->parent = ctx;
                list_context->partials = ctx->partials;
                list_context->get_partial = ctx->get_partial;
                response = node->first_child->execute(node->first_child, list_context);

                if (!response) {
                    break;
                }
            }

            break;
        case VALUE_OBJECT:
            is_true = value->data.object != NULL;
            if (is_true == (node->type == NODE_SECTION)) {
                value->data.object->parent = ctx;
                response = node->first_child->execute(node->first_child, value->data.object);
                value->data.object->parent = NULL;
            }
            break;
        case VALUE_VALUE:
            is_true = value->length > 0;
            if (is_true == (node->type == NODE_SECTION)) {
                response = node->first_child->execute(node->first_child, ctx);
            } else {
                response = true;
            }
            break;
    }

    value->destroy(value);

    if (!response) {
        // TODO: truncate
    }
    return true;
}

bool execute_partial(mustache_node_t* node, mustache_context_t* ctx)
{
    mustache_node_t* partial = ctx->get_partial(node->token->text, ctx->partials);

    // add indentation
    char* indentation = NULL;

    if (node->prv_sibling &&
        node->prv_sibling->type == NODE_TEXT &&
        is_whitespace(node->prv_sibling) &&
        (!node->prv_sibling->prv_sibling || node->prv_sibling->prv_sibling->type == NODE_NEWLINE)) {

        // Save existing indentation, it will be replaced after partial execution
        indentation = ctx->prefix;
        size_t current_length = *ctx->prefix ? strlen(ctx->prefix) : 0;
        size_t sibling_length = strlen(node->prv_sibling->token->text);

        ctx->prefix = malloc(sizeof(char) * (current_length + sibling_length + 1));

        if (current_length) {
            strncpy(ctx->prefix, ctx->prefix, current_length);
        }
        strncpy(ctx->prefix + current_length, node->prv_sibling->token->text, sibling_length);
    }

    bool ret_val = true;
    if (partial) {
        ret_val = partial->execute(partial, ctx);
    }

    // restore indentation
    if (indentation) {
        free(ctx->prefix);
        ctx->prefix = indentation;
    }

    return ret_val;
}

bool execute_comment(mustache_node_t* node, mustache_context_t* ctx)
{
    return true;
}

/* helper functions */

static bool html_encode(
    char* non_terminated_input,
    size_t length,
    mustache_context_t* ctx)
{
    char current;
    char* replacement;
    size_t replacement_length;

    for (size_t i = 0; i < length; i++) {
        current = non_terminated_input[i];

        switch (current) {
            case '&': replacement = "&amp;";  replacement_length = 5; break;
            case '"': replacement = "&quot;"; replacement_length = 6; break;
            case '<': replacement = "&lt;";   replacement_length = 4; break;
            case '>': replacement = "&gt;";   replacement_length = 4; break;
            default:
                replacement = &current;
                replacement_length = 1;
                break;
        }

        if (!mustache_write_to_buffer(ctx, replacement, replacement_length)) {
            return false;
        }
    }

    return true;
}

static mustache_value_t* get_variable_value(char* key, mustache_context_t* ctx)
{
    // special case for implicit variables
    if (key[0] == '.') {
        return ctx->get_data(key, ctx->data, ctx, ctx);
    }

    char* context_identifier = strtok(key, ".");
    mustache_value_t* result = NULL;
    while (context_identifier) {
        result = get_value(context_identifier, ctx);
        if (result && result->type == VALUE_OBJECT) {
            ctx = result->data.object;
        }
        context_identifier = strtok(NULL, ".");
    }
    return result;
}

static mustache_value_t* get_value(char* key, mustache_context_t* ctx)
{
    mustache_value_t* value = ctx->get_data(key, ctx->data, ctx, ctx);
    mustache_context_t* current_context = ctx;
    while (!value && current_context->parent) {
        current_context = current_context->parent;
        value = current_context->get_data(key, current_context->data, current_context, ctx);
    }
    return value;
}
