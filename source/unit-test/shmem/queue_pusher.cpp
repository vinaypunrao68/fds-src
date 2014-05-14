/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <stdio.h>
#include <random>
#include <chrono>
#include <thread>
#include <platform/fds-shm-queue.h>

int main() {

    
    fds::FdsShmQueue<int> queue ("shm_1");
    queue.shmq_alloc(65535);

    // For some number of iterations
    int i = 0;
    while(true) {
        // Push i and wait
        queue.shmq_enqueue(i);
        ++i;
        // // Generate a random wait period between [0, 5] seconds
        // std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());
        // std::uniform_int_distribution<int> distribution(0,3);
        // int rnd_roll = distribution(generator);  // generates number in the range 1..6 

        // // Generate a random wait period
        // std::chrono::seconds span(rnd_roll);
        // // Sleep a bit 
        // std::this_thread::sleep_for(span);

        // printf("%d\n", i);
    }

    return 0;
}
