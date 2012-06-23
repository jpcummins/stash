#include <ruby.h>
#include "mustache.h"
#include "stash.h"

const size_t INITIAL_BUFFER_SIZE = 1024;

mustache_ruby_context_t* create_mustache_ruby_context()
{
    mustache_ruby_context_t* r_ctx = mustache_malloc(sizeof(mustache_ruby_context_t));
    r_ctx->buffer = rb_str_buf_new(INITIAL_BUFFER_SIZE);
    r_ctx->buffer_length = 0;
    return r_ctx;
}

void destroy_mustache_context(mustache_context_t* m_ctx)
{
    mustache_free(m_ctx);
}

mustache_context_t* create_mustache_context(mustache_context_t* parent)
{
    mustache_context_t* m_ctx = mustache_malloc(sizeof(mustache_context_t));
    m_ctx->get_data = convert_type;
    m_ctx->get_partial = get_partial;
    m_ctx->parent = parent;
    m_ctx->data = NULL;
    m_ctx->partials = parent ? parent->partials : NULL;
    m_ctx->prefix = parent ? parent->prefix : NULL;
    m_ctx->custom = parent ? parent->custom : create_mustache_ruby_context();
    m_ctx->debug_mode = parent ? parent->debug_mode : false;
    m_ctx->destroy = destroy_mustache_context;
    return m_ctx;
}

static void destroy_mustache_object(mustache_value_t* value)
{
    mustache_value_t* m_value = (mustache_value_t*) value;
    m_value->data.object->destroy(m_value->data.object);
    mustache_free(m_value);
}

static mustache_value_t* create_mustache_object(
    mustache_context_t* m_ctx,
    VALUE object)
{
    mustache_value_t* val = mustache_malloc(sizeof(mustache_value_t));
    val->type = VALUE_OBJECT;
    val->length = 0;
    val->data.object = create_mustache_context(m_ctx);
    val->data.object->data = (void*) object;
    val->destroy = destroy_mustache_object;
    return val;
}

static void destroy_mustache_list(mustache_value_t* value)
{
    mustache_context_t** m_list_ctxs = value->data.list;
    for (size_t i = 0; i < value->length; i++) {
        m_list_ctxs[i]->destroy(m_list_ctxs[i]);
    }
    mustache_free(value);
}

static mustache_value_t* create_mustache_list(
    mustache_context_t* m_ctx,
    VALUE rb_array)
{
    size_t length = RARRAY_LEN(rb_array);
    mustache_context_t** m_list_ctxs = mustache_malloc(sizeof(mustache_context_t*) * length);

    for (long i = 0; i < length; i++) {
        m_list_ctxs[i] = create_mustache_context(m_ctx);
        m_list_ctxs[i]->data = (void*) rb_ary_entry(rb_array, i);
    }

    mustache_value_t* val = mustache_malloc(sizeof(mustache_value_t));
    val->type = VALUE_LIST;
    val->length = length;
    val->data.list = m_list_ctxs;
    val->destroy = destroy_mustache_list;
    return val;
}

static void destroy_mustache_value(mustache_value_t* value)
{
    mustache_free(value);
}

static mustache_value_t* create_mustache_value(
    mustache_context_t* m_ctx,
    VALUE rb_string)
{
    mustache_value_t* val = mustache_malloc(sizeof(mustache_value_t));
    val->type = VALUE_VALUE;
    val->data.value = rb_string == Qnil ? NULL : RSTRING_PTR(rb_string);
    val->length = rb_string == Qnil ? 0 : RSTRING_LEN(rb_string);
    val->destroy = destroy_mustache_value;
    return val;
}

static mustache_value_t* convert_value(
    VALUE rb_value,
    mustache_context_t* m_ctx)
{
    switch (TYPE(rb_value)) {
        case T_OBJECT: return create_mustache_object(m_ctx, rb_value);
        case T_STRUCT: return create_mustache_object(m_ctx, rb_value);
        case T_HASH:   return create_mustache_object(m_ctx, rb_value);
        case T_TRUE:   return create_mustache_value(m_ctx, rb_str_new2("true"));
        case T_FALSE:  return create_mustache_value(m_ctx, Qnil);
        case T_NONE:   return create_mustache_value(m_ctx, Qnil);
        case T_NIL:    return create_mustache_value(m_ctx, Qnil);
        case T_FLOAT:  return create_mustache_value(m_ctx, rb_funcall(rb_value, rb_intern("to_s"), 0, 0));
        case T_STRING: return create_mustache_value(m_ctx, rb_value);
        case T_FIXNUM: return create_mustache_value(m_ctx, rb_fix2str(rb_value, 10));
        case T_BIGNUM: return create_mustache_value(m_ctx, rb_big2str(rb_value, 10));
        case T_ARRAY:  return create_mustache_list(m_ctx, rb_value);
        default:
            if (rb_class_of(rb_value) == rb_cProc) {
                return convert_value(rb_funcall(rb_value, rb_intern("call"), 0, 0), m_ctx);
            }
            return create_mustache_value(m_ctx, rb_any_to_s(rb_value));
    }
}

static mustache_value_t* convert_hash(
  VALUE hash,
  char* key,
  mustache_context_t* m_lookup_context,
  mustache_context_t* m_exection_context)
{
    VALUE rb_value;
    if (NIL_P(rb_value = rb_hash_aref(hash, ID2SYM(rb_intern(key))))) {
        return NULL;
    }
    return convert_value(rb_value, m_lookup_context);
}

static mustache_value_t* convert_object(
  VALUE rb_object,
  char* key,
  mustache_context_t* m_lookup_context,
  mustache_context_t* m_exection_context)
{
    ID id = rb_intern(key);
    if (rb_respond_to(rb_object, id)) {
        rb_ivar_set(rb_object, rb_intern("@context"), (VALUE) m_exection_context->data);

        double start = 0;
        if (m_lookup_context->debug_mode) {
            start = get_time();
        }

        VALUE result = rb_funcall(rb_object, id, 0, 0);

        if (m_lookup_context->debug_mode) {
            log_ruby_callback_execution_time(m_lookup_context, id, get_time() - start);
        }

        return convert_value(result, m_lookup_context);
    }
    return NULL;
}

mustache_value_t* convert_type(
  char* key,
  void* context,
  mustache_context_t* m_lookup_context,
  mustache_context_t* m_exection_context)
{
    if (strcmp(key, ".") == 0) {
        return convert_value((VALUE)context, m_lookup_context);
    }

    VALUE rb_context = (VALUE) context;
    switch (TYPE(rb_context)) {
        case T_OBJECT: return convert_object(rb_context, key, m_lookup_context, m_exection_context);
        case T_HASH:   return convert_hash(rb_context, key, m_lookup_context, m_exection_context);
        default:       return NULL;
    }
}
