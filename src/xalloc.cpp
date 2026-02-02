#include <errno.h>
#include <stdarg.h>
#include <cstdio>
#include <cstring>

#include "utility.h"
#include "xalloc.h"

void *xmalloc(size_t size)
{
    void *ptr;

    if (size == 0) {
        return nullptr;
    }
    ptr = malloc(size);
    if (ptr == nullptr) {
        fprintf(stderr, "malloc(%zu): %s\n",
                size, strerror(errno));
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void *xcalloc(size_t nmemb, size_t size)
{
    void *ptr;

    if (nmemb == 0 || size == 0) {
        return nullptr;
    }
    ptr = calloc(nmemb, size);
    if (ptr == nullptr) {
        fprintf(stderr, "calloc(%zu, %zu): %s\n",
                nmemb, size, strerror(errno));
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void *xrealloc(void *ptr, size_t size)
{
    if (size == 0) {
        free(ptr);
        return nullptr;
    }
    ptr = realloc(ptr, size);
    if (ptr == nullptr) {
        fprintf(stderr, "realloc(%p, %zu): %s\n",
                ptr, size, strerror(errno));
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void *xreallocarray(void *ptr, size_t nmemb, size_t size)
{
    size_t n_bytes;

    if (nmemb == 0 || size == 0) {
        free(ptr);
        return nullptr;
    }
    if (__builtin_mul_overflow(nmemb, size, &n_bytes)) {
        fprintf(stderr, "reallocarray(%p, %zu, %zu): integer overflow\n",
                ptr, nmemb, size);
        exit(EXIT_FAILURE);
    }
    ptr = realloc(ptr, n_bytes);
    if (ptr == nullptr) {
        fprintf(stderr, "reallocarray(%p, %zu, %zu): %s\n",
                ptr, nmemb, size, strerror(errno));
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void *xmemdup(const void *ptr, size_t size)
{
    char *p_dup;

    if (size == 0) {
        return nullptr;
    }

    p_dup = malloc(size);
    if (p_dup == nullptr) {
        fprintf(stderr, "xmemdup(%p, %zu): %s\n",
                ptr, size, strerror(errno));
        exit(EXIT_FAILURE);
    }
    memcpy(p_dup, ptr, size);
    return p_dup;
}

char *xstrdup(const char *string)
{
    size_t length;
    char *result;

    /* `+ 1` for the null terminator */
    length = strlen(string) + 1;
    result = xmalloc(length);
    memcpy(result, string, length);
    return result;
}

char *xstrndup(const char *string, size_t length)
{
    char *result;

    length = strnlen(string, length);
    /* `+ 1` for the null terminator */
    result = xmalloc(length + 1);
    result[length] = '\0';
    memcpy(result, string, length);
    return result;
}

char *xasprintf(const char *format, ...)
{
    va_list list;
    int total_size;
    char *result;

    /* get the length of the expanded format */
    va_start(list, format);
    total_size = vsnprintf(nullptr, 0, format, list);
    va_end(list);

    if (total_size < 0) {
        fprintf(stderr, "snprintf(%s): %s\n",
                format, strerror(errno));
        exit(EXIT_FAILURE);
    }

    va_start(list, format);
    /* `+ 1` for the null terminator */
    result = xmalloc(total_size + 1);
    (void) vsprintf(result, format, list);
    va_end(list);
    return result;
}
