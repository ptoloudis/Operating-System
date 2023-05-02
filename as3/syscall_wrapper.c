#include <sys/syscall.h>
#include <unistd.h>
#include "syscall_wrapper.h"

long allocate(size_t size) {
    return syscall(__NR_my_kmalloc,size);
}

long deallocate(void* ptr) {
    return syscall(__NR_my_kfree,ptr);
}

long get_total_alloc_mem(void) {
    return syscall(__NR_slob_get_total_alloc_mem);
}

long get_total_free_mem(void) {
    return syscall(__NR_slob_get_total_free_mem);
}
