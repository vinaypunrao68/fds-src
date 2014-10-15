/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#define GTEST_USE_OWN_TR1_TUPLE 0

#include <chrono>
#include <atomic>
#include <thread>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TThreadedServer.h>
#include <fdsp_utils.h>
#include <ObjectId.h>
#include <testlib/DataGen.hpp>
#include <testlib/SvcMsgFactory.h>
#include <testlib/TestUtils.h>
#include <testlib/TestFixtures.h>
#include "TestAMSvc.h"
#include "TestSMSvc.h"
#include "Threadpool.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <google/profiler.h>

using ::testing::AtLeast;
using ::testing::Return;

using namespace fds;  // NOLINT
using namespace apache::thrift; // NOLINT
using namespace apache::thrift::protocol; // NOLINT
using namespace apache::thrift::transport; // NOLINT
using namespace apache::thrift::server; // NOLINT
using namespace apache::thrift::concurrency; // NOLINT

struct ThreadPoolTest: BaseTestFixture
{
    ThreadPoolTest()
    {
        issued_ = 0;
        completed_ = 0;
    }
    void work(uint64_t iterCnt)
    {
        util::StopWatch s;
        s.start();
        std::unordered_map<int, std::string> map;
#if 0
        auto datagen = boost::make_shared<RandDataGenerator<>>(20, 20);
        for (int i = 0; i < 10; i++) {
            map[i] = *datagen->nextItem();
        }
#endif
#if 0
        for (int i = 0; i < 1000; i++) {
            map[i] = std::string("hello");
        }
#endif
        int sum = 0;
        for (uint64_t i = 0; i < iterCnt; i++) {
            sum += i * i * i;
        }
        auto elapsed = s.getElapsedNanos();
        // std::cout << "elapsed: " << elapsed << std::endl;
        latency_.update(elapsed);
    }
    void doWork(fds_threadpool *tp, uint64_t iterCnt) {
        work(iterCnt);
        // tp->schedule(&ThreadPoolTest::workDoneCb, this);
        workDoneCb();
    }
    void workDoneCb() {
        completed_++;
        if (issued_ == completed_) {
           endTs_ = util::getTimeStampNanos();
        }
    }

    void doWork2(ThreadPool *tp, uint64_t iterCnt) {
        work(iterCnt);
        tp->enqueue(&ThreadPoolTest::workDoneCb, this);
    }
    void printStats() {
        std::cout << "Total time: " << endTs_ - startTs_ << " (ns)\n"
            << "Avg time: " << (static_cast<double>(endTs_ - startTs_)) / issued_
            << " (ns)\n";
        std::cout << "Avg latency : " << latency_.latency() << " (ns)\n";
    }
    void test() {
        int cnt = this->getArg<int>("cnt");
        uint64_t iterCnt = this->getArg<uint64_t>("iter-cnt");
        bool profile = this->getArg<bool>("profile");
        if (profile) {
            ProfilerStart("/tmp/singlethread2.prof");
        }
        startTs_ = util::getTimeStampNanos();
        for (int i = 0; i < cnt; i++) {
            issued_++;
            work(iterCnt);
            workDoneCb();
        }
        /* Poll for completion */
        POLL_MS((issued_ == completed_), 2000, (issued_* 20));
        if (profile) {
            ProfilerStop();
        }
        printStats();
        ASSERT_TRUE(issued_ == completed_) << "Issued: " << issued_
            << " completed_: " << completed_;
    }

    std::atomic<int> issued_;
    std::atomic<int> completed_;
    util::TimeStamp startTs_;
    util::TimeStamp endTs_;
    LatencyCounter latency_;
};

TEST_F(ThreadPoolTest, fdsthreadpool)
{
    int cnt = this->getArg<int>("cnt");
    uint64_t iterCnt = this->getArg<uint64_t>("iter-cnt");
    int tpSize = this->getArg<int>("tp-size");
    bool profile = this->getArg<bool>("profile");
    std::unique_ptr<fds_threadpool> tp(new fds_threadpool(tpSize));
    sleep(4);
    if (profile) {
        ProfilerStart("/tmp/fdsthreadpool.prof");
    }
    startTs_ = util::getTimeStampNanos();
    for (int i = 0; i < cnt; i++) {
        issued_++;
        tp->schedule(&ThreadPoolTest::doWork, this, tp.get(), iterCnt);
        // sleep(2);
    }
    auto dispLatency = latency_.total_latency();
    auto dispCount = latency_.count();

    /* Poll for completion */
    POLL_MS((issued_ == completed_), 2000, (issued_* 20));
    if (profile) {
        ProfilerStop();
    }
    printStats();
    std::cout << "disp latency: " << dispLatency << " cnt: " << dispCount << std::endl;
    std::cout << "Avg dispatch latency : "
        << static_cast<double>(dispLatency) / dispCount << " (ns)\n";
    std::cout << "Avg w.o dispatch latency : "
        << static_cast<double>(latency_.total_latency() - dispLatency) / (latency_.count()-dispCount) << " (ns)\n";  // NOLINT
    ASSERT_TRUE(issued_ == completed_) << "Issued: " << issued_
        << " completed_: " << completed_;
}

TEST_F(ThreadPoolTest, singlethread)
{
    ASSERT_EQ(completed_, 0);
    int cnt = this->getArg<int>("cnt");
    bool profile = this->getArg<bool>("profile");
    uint64_t iterCnt = this->getArg<uint64_t>("iter-cnt");
    if (profile) {
        ProfilerStart("/tmp/singlethread.prof");
    }
    startTs_ = util::getTimeStampNanos();
    for (int i = 0; i < cnt; i++) {
        issued_++;
        work(iterCnt);
        workDoneCb();
    }
    /* Poll for completion */
    POLL_MS((issued_ == completed_), 2000, (issued_* 20));
    if (profile) {
        ProfilerStop();
    }
    printStats();
    ASSERT_TRUE(issued_ == completed_) << "Issued: " << issued_
        << " completed_: " << completed_;
}

TEST_F(ThreadPoolTest, singlethread2)
{
    std::thread t(std::bind(&ThreadPoolTest::test, this));
    t.join();
}

TEST_F(ThreadPoolTest, DISABLED_threadpool)
{
    int cnt = this->getArg<int>("cnt");
    uint64_t iterCnt = this->getArg<uint64_t>("iter-cnt");
    std::unique_ptr<ThreadPool> tp(new ThreadPool(10));
    startTs_ = util::getTimeStampNanos();
    for (int i = 0; i < cnt; i++) {
        issued_++;
        tp->enqueue(&ThreadPoolTest::doWork2, this, tp.get(), iterCnt);
    }
    /* Poll for completion */
    POLL_MS((issued_ == completed_), 2000, (issued_* 20));
    printStats();
    ASSERT_TRUE(issued_ == completed_) << "Issued: " << issued_
        << " completed_: " << completed_;
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message")
        ("tp-size", po::value<int>()->default_value(10), "threadpool size")
        ("cnt", po::value<int>()->default_value(10000), "cnt")
        ("profile", po::value<bool>()->default_value(false), "profile")
        ("iter-cnt", po::value<uint64_t>()->default_value(10000), "iter-cnt");
    ThreadPoolTest::init(argc, argv, opts);
    std::cout << std::fixed;
    return RUN_ALL_TESTS();
}
