#include "syscall_wrapper.h"
#include <stdio.h>
#include <stdlib.h>

#define NUM_ALLOCS 1000
#define MAX_ALLOC 6273
#define TEST 431

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        fprintf(stderr, "Error: could not open file '%s'\n", argv[1]);
        return EXIT_FAILURE;
    }

    int count = 0, size;
    void *ptrs[NUM_ALLOCS];

    printf("Allocated %ld\n", get_total_alloc_mem());
    printf("Freeing %ld\n", get_total_free_mem());
    printf("------------------------------------------------------------------------\n");
    printf("Fulfilling the array\n");

    for (int i = 0; i < NUM_ALLOCS; i++) {
        if (count == 0 || count == TEST) {
            printf("Allocated %ld\n", get_total_alloc_mem());
            printf("Freeing %ld\n", get_total_free_mem());
        }

        if (fscanf(file, "%d", &size) == EOF) {
            printf("EOF\n");
            return EXIT_FAILURE;
        }

        ptrs[i] = allocate(size);
        count++;
    }

    printf("The array is full and now changing\n");

    for (int i = 0; i < MAX_ALLOC; i++) {
        if (count == 0 || count == TEST) {
            printf("Allocated %ld\n", get_total_alloc_mem());
            printf("Freeing %ld\n", get_total_free_mem());
        }

        printf("Freeing %p\n", ptrs[i % NUM_ALLOCS]);
        deallocate(ptrs[i % NUM_ALLOCS]);

        if (fscanf(file, "%d", &size) == EOF) {
            printf("EOF\n");
            return EXIT_FAILURE;
        }

        ptrs[i % NUM_ALLOCS] = allocate(size);
        count++;
    }

    printf("Emptying the array....\n");
    for (int i = 0; i < NUM_ALLOCS; i++) {
        if (count == 0 || count == TEST) {
            printf("Allocated %ld\n", get_total_alloc_mem());
            printf("Freeing %ld\n", get_total_free_mem());
        }

        deallocate(ptrs[i]);
        count++;
    }

    printf("The array is empty\n");
    printf("------------------------------------------------------------------------\n");
    printf("Allocated %ld\n", get_total_alloc_mem());
    printf("Freeing %ld\n", get_total_free_mem());

    fclose(file);
    return EXIT_SUCCESS;
}
