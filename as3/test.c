#include "syscall_wrapper.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_ALLOCS 1000
#define MAX_ALLOC 6273
#define TEST 431
#define MAX_SIZE 255 // 1023  4095

int main()
{
    srand(251);

    int count = 0, size;
    void* ptrs[NUM_ALLOCS];

    printf("Allocated %ld\n", get_total_alloc_mem());
    printf("Freeing %ld\n", get_total_free_mem());
    printf("------------------------------------------------------------------------\n");
    printf("Fulfilling the array\n");
    
    for (int i = 0; i < NUM_ALLOCS; i++) {
        if(!count){
            printf("Allocated %ld\n", get_total_alloc_mem());
            printf("Freeing %ld\n", get_total_free_mem());
        }
        else if(count == TEST){
            count = 0;
        }

        size = rand() % MAX_SIZE + 1;
        ptrs[i] = (void*)allocate(size);
        count++;
    }

    printf("The array is full and now change\n");

    for (int i = 0; i < MAX_ALLOC; i++) {
        if(!count){
            printf("Allocated %ld\n", get_total_alloc_mem());
            printf("Freeing %ld\n", get_total_free_mem());
        }
        else if(count == TEST){
            count = 0;
        }

        printf("Freeing %p\n", ptrs[i % NUM_ALLOCS]);
        deallocate(ptrs[i % NUM_ALLOCS]);
        size = rand() % MAX_SIZE + 1;
        ptrs[i % NUM_ALLOCS] = (void*)allocate(size);
        count++;
    }

    printf("Emptying the array....\n");
    for (int i = 0; i < NUM_ALLOCS; i++) {
        if(!count){
            printf("Allocated %ld\n", get_total_alloc_mem());
            printf("Freeing %ld\n", get_total_free_mem());
        }
        else if(count == TEST){
            count = 0;
        }

        deallocate(ptrs[i]);
        count++;
    }
    
    printf("The array is empty\n");
    printf("------------------------------------------------------------------------\n");
    printf("Allocated %ld\n", get_total_alloc_mem());
    printf("Freeing %ld\n", get_total_free_mem());

    return 0;
}
