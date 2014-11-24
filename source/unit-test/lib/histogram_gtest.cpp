/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <limits>
#include <string>
#include <Histogram.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::AtLeast;
using ::testing::Return;

using namespace fds;  // NOLINT

int const MaxThreads = 10;

class testBarrier {
  public:
    explicit testBarrier(uint32_t barrierCnt);
    ~testBarrier();

    void testBarrierWait();
  private:
    std::mutex barrierMtx;
    std::condition_variable barrierCondVar;
    uint32_t barrierCount;
};

testBarrier::testBarrier(uint32_t barrierCnt)
{
    barrierCount = barrierCnt;
}

testBarrier::~testBarrier()
{
}

void
testBarrier::testBarrierWait()
{
    std::unique_lock<std::mutex> lock(barrierMtx);
    --barrierCount;
    if (barrierCount == 0) {
        barrierCondVar.notify_all();
    }
    while (barrierCount > 0) {
        barrierCondVar.wait(lock);
    }
}


TEST(Histogram, basic)
{
    uint32_t upperLimit = 10001;
    std::string name = "histogramTestBasic";
    Histogram *histPtr = new Histogram(name, 5);

    uint64_t range[6] = {0, 10, 100, 1000, 10000, std::numeric_limits<uint64_t>::max()};
    EXPECT_TRUE(histPtr->histogramSetRanges(range, 6));

    for (int i = 0; i < 10001; ++i) {
        histPtr->histogramIncrement(i);
    }

    size_t totalSum = 0;
    for (size_t i = 0; i < 5; ++i) {
        totalSum += histPtr->histogramGetIth(i);
    }
    EXPECT_EQ(totalSum, upperLimit);

    delete histPtr;
}

TEST(Histogram, uniform)
{
    uint64_t upperLimit = 110;
    uint64_t lowerLimit = 10;
    std::string name = "histogramTestUniform";
    Histogram *histPtr = new Histogram(name, 10);

    histPtr->histogramSetRangesUniform(lowerLimit, upperLimit);

    for (int i = 10; i < 110; ++i) {
        histPtr->histogramIncrement(i);
    }

    size_t totalSum = 0;
    for (size_t i = 0; i < 10; ++i) {
        totalSum += histPtr->histogramGetIth(i);
    }
    EXPECT_EQ(totalSum, upperLimit - lowerLimit);

    delete histPtr;
}


void
histoThread(Histogram *histPtr, testBarrier &barrier, uint32_t upperLimit)
{
    barrier.testBarrierWait();

    for (uint32_t i = 0; i < upperLimit; ++i) {
        histPtr->histogramIncrementAtomic(i);
    }
}

TEST(Histogram, basicMultiThreadBasic)
{
    uint32_t upperLimit = 10000;
    std::string name = "histogramBasicMultiThread";
    Histogram *histPtr = new Histogram(name, 5);


    uint64_t range[6] = {0, 10, 100, 1000, 10000, std::numeric_limits<uint64_t>::max()};
    EXPECT_TRUE(histPtr->histogramSetRanges(range, 6));

    std::vector<std::thread> threads;
    testBarrier barrier(MaxThreads + 1);

    for (int i = 0; i < MaxThreads; ++i) {
        threads.push_back(std::thread(histoThread,
                          histPtr,
                          std::ref(barrier),
                          upperLimit));
    }

    barrier.testBarrierWait();

    for (auto& thread : threads) {
        thread.join();
    }

    size_t totalSum = 0;
    for (size_t i = 0; i < 5; ++i) {
        totalSum += histPtr->histogramGetIth(i);
    }
    EXPECT_EQ(totalSum, upperLimit * MaxThreads);
}

TEST(Histogram, basicMultiThreadUniform)
{
    std::string name = "histogramUniformMultiThread";
    Histogram *histPtr = new Histogram(name, 10);

    histPtr->histogramSetRangesUniform(10, 110);

    testBarrier barrier(MaxThreads + 1);
    std::vector<std::thread> threads;

    for (int i = 0; i < MaxThreads; ++i) {
        threads.push_back(std::thread(histoThread,
                          histPtr,
                          std::ref(barrier),
                          111));
    }

    barrier.testBarrierWait();

    for (auto& thread : threads) {
        thread.join();
    }

    for (size_t i = 0; i < 10; ++i) {
        EXPECT_EQ(histPtr->histogramGetIth(i), 100);
    }

    delete histPtr;
}



int
main(int argc, char** argv) {
    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

