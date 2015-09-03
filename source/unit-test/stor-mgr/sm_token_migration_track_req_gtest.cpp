/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <string>
#include <chrono>
#include <cstdlib>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fds_process.h>
#include <MigrationUtility.h>


namespace fds {

static const std::string logname = "sm_track_req";

void
thrStartReqs(MigrationTrackIOReqs& migTrackReqs, uint32_t numReqs)
{
    for (uint32_t i = 0; i < numReqs; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        bool ret = migTrackReqs.startTrackIOReqs();
        if (ret) {
            migTrackReqs.finishTrackIOReqs();
        }
    }
}


void
thrWaitCompleteReqs(MigrationTrackIOReqs& migTrackReqs)
{
    int duration = (rand() % 2) + 1;  // 1 or 2 secs
    sleep(duration);
    migTrackReqs.waitForTrackIOReqs();
}


TEST(MigrationTrackIOReqs, trackIOReqs1)
{
    const uint32_t numReqs = 100;
    MigrationTrackIOReqs pendReqs;

    pendReqs.startTrackIOReqs();

    std::thread t1(thrStartReqs, std::ref(pendReqs), numReqs);
    std::thread t3(thrWaitCompleteReqs, std::ref(pendReqs));

    t1.join();
    std::cout << "Thread t1 exited" << std::endl;
    pendReqs.finishTrackIOReqs();
    std::cout << "Finished tracking io" << std::endl;
    t3.join();
    std::cout << "Thread t3 exited" << std::endl;
}

}  // namespace fds

int
main(int argc, char *argv[])
{
    srand(time(NULL));
    fds::init_process_globals(fds::logname);
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

