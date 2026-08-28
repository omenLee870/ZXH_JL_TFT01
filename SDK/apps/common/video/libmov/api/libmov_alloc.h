#ifndef _libmov_alloc_h_
#define _libmov_alloc_h_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void *libmov_malloc(size_t size);
void *libmov_calloc(size_t n, size_t size);
void *libmov_realloc(void *ptr, size_t size);
void libmov_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* _libmov_alloc_h_ */

