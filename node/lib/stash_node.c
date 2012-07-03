#include <stdlib.h>
#include "mustache.h"

void* mustache_malloc(long size)
{
    return malloc(size);
}

void mustache_free(void* ptr)
{
    free(ptr);
}

bool mustache_write_to_buffer(mustache_context_t* m_ctx, char* data, size_t data_length)
{
    return true;
}
