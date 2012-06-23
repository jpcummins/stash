#include "intern.h"

/* forward declare helper functions */
static bool contains_node_type(mustache_node_t* root, enum mustache_node_type type);
static void remove_node(mustache_node_t* node);
static bool is_posfix_standalone(mustache_node_t* node);
bool is_whitespace(mustache_node_t* text_node);

bool optimize_block(mustache_node_t* node)
{
    mustache_node_t* child = node->first_child;
    while (child) {
        if (!child->optimize(child)) {
            return false;
        }
        child = child->next_sibling;
    }
    return true;
}

bool optimize_text(mustache_node_t* node)
{
    // Remove indentation from starting standalone tags
    if ((!node->prv_sibling || node->prv_sibling->type == NODE_NEWLINE) &&
        node->next_sibling &&
        (node->next_sibling->type == NODE_SECTION || node->next_sibling->type == NODE_INVERTED_SECTION) &&
        contains_node_type(node->next_sibling->first_child, NODE_NEWLINE) &&
        is_whitespace(node)) {
        remove_node(node);
    }

    // Remove indentation from ending standalone tags if beginning tag was intented
    if (!node->next_sibling &&
        node->prv_sibling &&
        node->prv_sibling->type == NODE_NEWLINE &&
        node->parent &&
        node->parent->parent &&
        (node->parent->parent->type == NODE_SECTION || node->parent->parent->type == NODE_INVERTED_SECTION) &&
        (!node->parent->parent->next_sibling || (node->parent->parent->prv_sibling && is_whitespace(node->parent->parent->prv_sibling))) &&
        is_whitespace(node)) {
        remove_node(node);
    }

    return true;
}

bool optimize_newline(mustache_node_t* node)
{
    // Remove newlines after starting standalone section tags
    if (!node->prv_sibling &&
        node->parent &&
        node->parent->parent &&
        (node->parent->parent->type == NODE_SECTION || node->parent->parent->type == NODE_INVERTED_SECTION)&&
        (!node->parent->parent->prv_sibling || is_whitespace(node->parent->parent->prv_sibling))) {
        remove_node(node);
    }

    // Remove newlines after ending standalone section tags
    if (node->prv_sibling &&
        (node->prv_sibling->type == NODE_SECTION || node->prv_sibling->type == NODE_INVERTED_SECTION) &&
        (!node->prv_sibling->first_child->first_child || contains_node_type(node->prv_sibling->first_child, NODE_NEWLINE))) {
        remove_node(node);
    }
    return true;
}

bool optimize_variable(mustache_node_t* node)
{
    return true;
}

bool optimize_section(mustache_node_t* node)
{
    return node->first_child->optimize(node->first_child);
}

bool optimize_comment(mustache_node_t* node)
{
    // Remove whitespace from standalone comments.
    // Comments that satisfy any of the following conditions are considered standalone:
    //   1. "{{!comment}}"
    //   2. "{{!comment}}  "
    //   3. "{{!comment}}\n"
    //   4. "{{!comment}}  \n"
    //   5. "  {{!comment}}"
    //   6. "  {{!comment}}  "
    //   7. "  {{!comment}}\n"
    //   8. "  {{!comment}}  \n"
    //   9. "\n{{!comment}}"
    //  10. "\n{{!comment}}  "
    //  11. "\n{{!comment}}\n"
    //  12. "\n{{!comment}}  \n"
    //  13. "\n  {{!comment}}"
    //  14. "\n  {{!comment}}  "
    //  15. "\n  {{!comment}}\n"
    //  16. "\n  {{!comment}}  \n"

    bool is_standalone = false;
    mustache_node_t* tmp = NULL;
    mustache_node_t* prv = node->prv_sibling;
    mustache_node_t* next = node->next_sibling;
    if (!prv) {
        is_standalone = is_posfix_standalone(node);
    } else if (is_whitespace(prv)) {
        if (!prv->prv_sibling &&
            (!node->parent || !node->parent->parent || !(node->parent->parent->type == NODE_SECTION || node->parent->parent->type == NODE_INVERTED_SECTION))) {
            is_standalone = is_posfix_standalone(node);
        } else if (prv->type == NODE_NEWLINE) {
            is_standalone = is_posfix_standalone(node);
        } else if (prv->prv_sibling && prv->prv_sibling->type == NODE_NEWLINE) {
            is_standalone = is_posfix_standalone(node);
        }
    }

    // if the comment is standalone, remove the unnessesary nodes.
    if (is_standalone) {
        if (prv && prv->type == NODE_TEXT && node->type == NODE_COMMENT) {
            remove_node(prv);
        }
        if (next && next->type == NODE_TEXT) {
            tmp = next->next_sibling;
            remove_node(next);
            next = tmp;
        }
        while (next && next->type == NODE_NEWLINE) {
            tmp = next->next_sibling;
            remove_node(next);
            next = tmp;
        }
    }
    return true;
}

bool optimize_partial(mustache_node_t* node)
{
    return optimize_comment(node);
}


/* helper functions */

static bool contains_node_type(mustache_node_t* root, enum mustache_node_type type)
{
    if (root->type == type) {
        return true;
    }

    if ((root->first_child && contains_node_type(root->first_child, type)) ||
        (root->next_sibling && contains_node_type(root->next_sibling, type))) {
        return true;
    }

    return false;
}

bool is_whitespace(mustache_node_t* text_node)
{
    if (text_node->type == NODE_NEWLINE) {
        return true;
    }

    if (text_node->type != NODE_TEXT) {
        return false;
    }

    char* text = text_node->token->text;
    while (text[0]) {
        if (!isspace(text[0])) {
            return false;
        }
        text++;
    }
    return true;
}

static void remove_node(mustache_node_t* node)
{
    if (!node) {
        return;
    }

    mustache_node_t* prv = node->prv_sibling;
    mustache_node_t* next = node->next_sibling;

    if (prv) {
        prv->next_sibling = next;
    } else {
        node->parent->first_child = next;
    }

    if (next) {
        next->prv_sibling = prv;
    }

    // TODO: free node
    // TODO: free node's children
}

static bool is_posfix_standalone(mustache_node_t* node)
{
    mustache_node_t* next = node->next_sibling;
    if (!next || next->type == NODE_NEWLINE) {
        return true;
    } else if (next->type == NODE_TEXT && is_whitespace(next)) {
        return (!next->next_sibling || next->next_sibling->type == NODE_NEWLINE);
    }
    return false;
}
