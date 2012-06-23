#include <ruby.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "mustache.h"
#include "stash.h"



extern void log_template_execution_time(mustache_context_t* m_ctx, double time);
extern double get_time();

VALUE cTemplate = Qnil;

static VALUE cTemplate_init(VALUE self, VALUE rb_template_string)
{
    mustache_node_t* c_template;
    Data_Get_Struct(self, mustache_node_t, c_template);

    char* c_template_string;
    long c_template_string_length;
    mustache_error_t* error = NULL;

    c_template_string = strdup(rb_str2cstr(rb_template_string, &c_template_string_length));
    mustache_build_template(c_template_string, c_template, &error);

    if (error) {
        rb_raise(rb_eSyntaxError, error->error_text);
    }

    return self;
}

static VALUE cTemplate_render(int argc, VALUE* argv, VALUE self)
{
    VALUE data = Qnil;
    VALUE partials = Qnil;
    VALUE debug = Qnil;

    if (argc > 0) {
        VALUE type = TYPE(argv[0]);
        if (type != T_OBJECT && type != T_HASH) {
            rb_raise(rb_eArgError, "Expected an object or hash as argument 1");
            return Qnil;
        }
        data = argv[0];
    }

    if (argc > 1) {
        VALUE type = TYPE(argv[1]);
        if (type != T_HASH) {
            rb_raise(rb_eArgError, "Expected a hash as argument 2");
            return Qnil;
        }
        partials = argv[1];
    }

    if (argc > 2) {
        if (strcmp(rb_obj_classname(argv[2]), "Template::Debug") != 0) {
            rb_raise(rb_eArgError, "Expected an instance of Template::Debug as argument 3");
            return Qnil;
        }
        debug = argv[2];
    }

    // Get the template
    mustache_node_t* c_template;
    Data_Get_Struct(self, mustache_node_t, c_template);

    // Create the context
    mustache_context_t* m_ctx = create_mustache_context(NULL);
    mustache_ruby_context_t* r_ctx = (mustache_ruby_context_t*) m_ctx->custom;
    m_ctx->data = (void*) data;
    m_ctx->partials = (void*) partials;
    m_ctx->debug_mode = debug != Qnil;

    double start = 0;
    if (m_ctx->debug_mode) {
        start = get_time();
        r_ctx->debugObject = debug;
    }

    // Execute the template
    mustache_execute_template(c_template, m_ctx);

    if (m_ctx->debug_mode) {
        log_template_execution_time(m_ctx, get_time() - start);
    }

    // Return the buffer
    VALUE result = rb_str_resize(r_ctx->buffer, r_ctx->buffer_length);
    m_ctx->destroy(m_ctx);
    return result;
}

static void template_mark(mustache_node_t* node)
{
}

static void template_free(mustache_node_t* node)
{
}

static VALUE template_allocate(VALUE klass)
{
    mustache_node_t* c_template = mustache_create_node();
    return Data_Wrap_Struct(cTemplate, template_mark, template_free, c_template);
}

void Init_stash()
{
    // Define the Template class
    cTemplate = rb_define_class("Template", rb_cObject);
    rb_define_alloc_func(cTemplate, template_allocate);
    rb_define_method(cTemplate, "initialize", cTemplate_init, 1);
    rb_define_method(cTemplate, "render", cTemplate_render, -1);
}

bool mustache_write_to_buffer(mustache_context_t* m_ctx, char* data, size_t data_length)
{
    if (data_length == 0) {
        return true;
    }

    mustache_ruby_context_t* r_ctx = (mustache_ruby_context_t*) m_ctx->custom;
    rb_str_buf_cat(r_ctx->buffer, data, data_length);
    r_ctx->buffer_length += data_length;
    return true;
}

void* mustache_malloc(long size)
{
    return ruby_xmalloc(size);
}

void mustache_free(void* ptr)
{
    ruby_xfree(ptr);
}

mustache_node_t* get_partial(char* key, void* partials)
{
    VALUE rb_partials = (VALUE) partials;
    Check_Type(rb_partials, T_HASH);

    VALUE rb_value;
    if (NIL_P(rb_value = rb_hash_aref(rb_partials, ID2SYM(rb_intern(key))))) {
        return NULL;
    }

    if (rb_class_of(rb_value) != cTemplate) {
        rb_raise(rb_eTypeError, "Expected Template");
    }

    mustache_node_t* template = NULL;
    Data_Get_Struct(rb_value, mustache_node_t, template);
    return template;
}

double get_time()
{
    struct timeval t;
    struct timezone tzp;
    gettimeofday(&t, &tzp);
    return t.tv_sec + t.tv_usec*1e-6;
}

void log_template_execution_time(mustache_context_t* m_ctx, double time)
{
    mustache_ruby_context_t* r_ctx = (mustache_ruby_context_t*) m_ctx->custom;
    rb_ivar_set(r_ctx->debugObject, rb_intern("@total_time"), rb_float_new(time));
}

void log_ruby_callback_execution_time(mustache_context_t* m_ctx, ID callback_name, double time)
{
    mustache_ruby_context_t* r_ctx = (mustache_ruby_context_t*) m_ctx->custom;
    rb_funcall(r_ctx->debugObject, rb_intern("log_callback"), 2, ID2SYM(callback_name), rb_float_new(time));
}
