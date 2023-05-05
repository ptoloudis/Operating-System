#include "syscall_wrapper.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define NUM_ALLOCS 1200 // allocation array size
#define MAX_ALLOC 20000 // extra allocations

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

    srand(1425);

    int j, size;
    void *ptrs[NUM_ALLOCS];
    long int alloc, free;

    alloc = get_total_alloc_mem();
    free = get_total_free_mem();
    printf("Allocated %ld, Freeing %ld, ratio %lf\n",alloc,free, (double) free/alloc );
    printf("------------------------------------------------------------------------\n");
    printf("Fulfilling the array\n");

    for (int i = 0; i < NUM_ALLOCS; i++) {
        if (fscanf(file, "%d", &size) == EOF) {
            printf("EOF\n");
            return EXIT_FAILURE;
        }

        ptrs[i] =(void*) allocate(size);

        alloc = get_total_alloc_mem();
	    free = get_total_free_mem();
	    printf("Allocated %ld, Freeing %ld, ratio %lf\n",alloc,free, (double) free/alloc );
    }

    printf("The array is full and now changing\n");

    for (int i = 0; i < MAX_ALLOC; i++) {
	    
        j = rand() % NUM_ALLOCS;
        deallocate(ptrs[j]);

        if (fscanf(file, "%d", &size) == EOF) {
            printf("EOF\n");
            return EXIT_FAILURE;
        }

        ptrs[j] = (void*) allocate(size);
        
    }

    printf("Emptying the array....\n");
    for (int i = 0; i < NUM_ALLOCS; i++) {

	    deallocate(ptrs[i]);
        alloc = get_total_alloc_mem();
	    free = get_total_free_mem();
	    printf("Allocated %ld, Freeing %ld, ratio %lf\n",alloc,free, (double) free/alloc );
    }

    printf("The array is empty\n");
    printf("------------------------------------------------------------------------\n");
    alloc = get_total_alloc_mem();
    free = get_total_free_mem();
    printf("Allocated %ld, Freeing %ld, ratio %lf\n",alloc,free, (double) free/alloc );
    
    fclose(file);
    return EXIT_SUCCESS;
}
