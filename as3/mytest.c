#include "syscall_wrapper.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stddef.h>

int main()
{
    void* ptr;
    printf("Allocated %ld\n", get_total_alloc_mem());
    printf("Freeing %ld\n", get_total_free_mem());
    ptr = (void*)allocate(127);
    printf("Allocated %ld\n", get_total_alloc_mem());
    printf("Freeing %ld\n", get_total_free_mem());
    deallocate(ptr);
    printf("Allocated %ld\n", get_total_alloc_mem());
    printf("Freeing %ld\n", get_total_free_mem());

    ptr = (void*)allocate(63);
    printf("Allocated %ld\n", get_total_alloc_mem());
    printf("Freeing %ld\n", get_total_free_mem());
    deallocate(ptr);
    printf("Allocated %ld\n", get_total_alloc_mem());
    printf("Freeing %ld\n", get_total_free_mem());


    return 0;
}
