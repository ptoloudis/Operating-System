#include <stddef.h>
long allocate(size_t size);
long deallocate(void* ptr);
long get_total_alloc_mem(void);
long get_total_free_mem(void);
