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
    
    fds::FdsShmQueue<int> queue ("shm_1");
    queue.shmq_alloc(1024);

    assert(queue.empty());
    queue.shmq_enqueue(12);
    printf("Passed first enqueue\n");
    assert(!queue.empty());
    queue.shmq_enqueue(22);
    printf("Passed second enqueue\n");
    queue.shmq_enqueue(32);
    printf("Passed third enqueue\n");
    queue.shmq_enqueue(42);
    printf("Passed fourth enqueue\n");

    fds::FdsShmQueue<int> queue_alt ("shm_1");
    queue_alt.shmq_connect();
    assert(!queue_alt.empty());
    
    for (int i = 0; i < 4; ++i) {
        int ret (0);
        if (i % 2 == 0) {
            queue.shmq_dequeue(ret);
        } else {
            queue_alt.shmq_dequeue(ret);
        }
        assert(ret == (i * 10) + 12);
    }

    assert(queue.empty());

    printf("Passed first dequeues\n");
    
    for (int i = 0; i < 50; ++i) {
        queue.shmq_enqueue(i);
    }
    for (int i = 0; i < 50; ++i) {
        int ret;
        queue.shmq_dequeue(ret);
    }
    assert(queue.empty());

    printf("shm test: PASSED!\n");

}
