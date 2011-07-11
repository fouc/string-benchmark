#ifndef REDIRECT_MALLOC_HPP
#define REDIRECT_MALLOC_HPP
/**
 * Code by Gianluca Insolvibile.
 *    http://www.linuxjournal.com/article/6679
 *
 * Using the malloc hooks to substitute GC functions
 * to existing malloc/free.
 * TODO: Similar wrapper functions can be written
 * to redirect calloc() and realloc()
 */

#include <malloc.h>
#include <gc.h>

static void gc_wrapper_init(void);
static void *gc_wrapper_malloc(size_t,const void *);
static void gc_wrapper_free(void*, const void *);

__malloc_ptr_t (*old_malloc_hook)
__MALLOC_PMT((size_t __size,
              __const __malloc_ptr_t));
void (*old_free_hook)
__MALLOC_PMT ((__malloc_ptr_t __ptr,
               __const __malloc_ptr_t));


/* Override initializing hook from the C library. */
void (*__malloc_initialize_hook)(void) =
    gc_wrapper_init;

static void gc_wrapper_init()
{
    old_malloc_hook = __malloc_hook;
    old_free_hook = __free_hook;
    __malloc_hook = gc_wrapper_malloc;
    __free_hook = gc_wrapper_free;
}

void *
gc_wrapper_malloc(size_t size, const void *ptr)
{
    void *result;
    /* Restore all old hooks */
    __malloc_hook = old_malloc_hook;
    __free_hook = old_free_hook;

    /* Call the Boehm malloc */
    result = GC_malloc(size);


    /* Save underlying hooks */
    old_malloc_hook = __malloc_hook;
    old_free_hook = __free_hook;

    /* Restore our own hooks */
    __malloc_hook = gc_wrapper_malloc;
    __free_hook = gc_wrapper_free;

    return result;
}

static void
gc_wrapper_free(void *ptr, const void *caller)
{
    /* Nothing done! */
}
#endif // REDIRECT_MALLOC_HPP
