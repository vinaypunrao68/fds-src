/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <stdio.h>
#include <platform/fds-shmem.h>
#include <sys/mman.h>

int main() {

    // shm name
    const char * n = "/test\0";

    // Be sure that we're already unlinked
    shm_unlink(n);

    // Create a new shared mem segment
    fds::FdsShmem a{n, 1024};
    void *first = a.shm_alloc();

    printf("Value of first[0] = %d\n", ((int*)first)[0]);
    // Write to it
    *(int *)first = 1;
    printf("Value of first[0] = %d\n", ((int*)first)[0]);

    // Create a second and attach
    fds::FdsShmem b{n, 1024};
    void *second = b.shm_attach();
    
    // verify that the contents are the same
    assert( ((int*)first)[0] == ((int*)second)[0] );

    // Try to write via second
    //*(int *)second = 2;
    
    // Unmap and remap
    a.shm_detach();
    b.shm_detach();
     
    assert(a.shm_remove() == 0);

    return 0;
}
