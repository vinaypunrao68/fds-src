/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <stdio.h>
#include <platform/fds-shmem.h>

int main() {

    // shm name
    char *n = "abc\0";

    // Create a new shared mem segment
    fds::FdsShmem a{n};
    void *first = a.shm_alloc(1024);
    assert(first != nullptr);
    
    printf("Value of first[0] = %d\n", ((int*)first)[0]);
    // Write to it
    *(int *)first = 42;
    printf("Value of first[0] = %d\n", ((int*)first)[0]);
    assert(((int*)first)[0] == 42);
    
    // Create a second and attach
    fds::FdsShmem b{n};
    void *second = b.shm_attach();
    assert(second != nullptr);
    
    
    // verify that the contents are the same
    assert( ((int*)first)[0] == ((int*)second)[0] );

    assert(a.shm_remove() == true);

    return 0;
}
