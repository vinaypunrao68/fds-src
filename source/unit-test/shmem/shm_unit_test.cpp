/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <stdio.h>
#include <platform/fds-shmem.h>
#include <platform/fds-shm-queue.h>

int main() {

    
    // shm name
    char *n = "abc\0";

    // Create a new shared mem segment
    fds::FdsShmem a{n};
    void *first = a.shm_alloc(1024);
    assert(first != nullptr);

    // Write to it
    *(int *)first = 42;
    assert(((int*)first)[0] == 42);
    
    // Create a second and attach
    fds::FdsShmem b{n};
    void *second = b.shm_attach();
    assert(second != nullptr);
    
    // verify that the contents are the same
    assert( ((int*)first)[0] == ((int*)second)[0] );

    assert(a.shm_remove() == true);
    
    fds::FdsShmQueue<int> queue ("shm_queue", 4096);

    assert(queue.empty());
    queue.shmq_enqueue(12);
    assert(!queue.empty());
    queue.shmq_enqueue(22);
    queue.shmq_enqueue(32);
    queue.shmq_enqueue(42);

    for (int i = 0; i < 4; ++i) {
        int ret = queue.shmq_dequeue();
        assert(ret == (i * 10) + 12);
    }
    return 0;

    for (int i = 0; i < 100; ++i) {
        queue.shmq_enqueue(i);
    }
    for (int i = 0; i < 100; ++i) {
        queue.shmq_dequeue();
    }
    assert(queue.empty());

}
