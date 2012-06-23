typedef struct mustache_ruby_context {
    VALUE buffer;
    long buffer_length;
    VALUE debugObject;
} mustache_ruby_context_t;

mustache_context_t* create_mustache_context(mustache_context_t* parent);

mustache_value_t* convert_type(
  char* key,
  void* context,
  mustache_context_t* m_lookup_context,
  mustache_context_t* m_exection_context);

mustache_node_t* get_partial(char* key, void* partials);
void log_ruby_callback_execution_time(mustache_context_t* m_ctx, ID callback_name, double time);
double get_time();
