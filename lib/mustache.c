#include "intern.h"

mustache_node_t* mustache_create_node()
{
    mustache_node_t* node = mustache_malloc(sizeof(mustache_node_t));
    node->type = NODE_UNKNOWN;
    node->parent = NULL;
    node->first_child = NULL;
    node->prv_sibling = NULL;
    node->next_sibling = NULL;
    node->token = NULL;
    node->execute = NULL;
    node->optimize = NULL;
    return node;
}

void mustache_destroy_node(mustache_node_t* node)
{
    if (node->first_child) {
        mustache_destroy_node(node->first_child);
    }

    if (node->next_sibling) {
        mustache_destroy_node(node->next_sibling);
    }

    free_token(node->token);
    mustache_free(node);
}

bool mustache_build_template(
    char* template,
    mustache_node_t* root,
    mustache_error_t** error)
{
    mustache_token_t* tokens = NULL;
    get_tokens(template, &tokens);
    build_template(tokens, root, error);
    root->optimize(root);
    return true;
}

bool mustache_execute_template(mustache_node_t* node, mustache_context_t* ctx)
{
    ctx->prefix = "";
    bool result = node->execute(node, ctx);
    return result;
}
