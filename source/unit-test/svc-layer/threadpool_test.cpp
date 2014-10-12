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
    inline void work()
    {
        util::StopWatch s;
        s.start();
        std::unordered_map<int, std::string> map;
        auto datagen = boost::make_shared<RandDataGenerator<>>(20, 20);
        for (int i = 0; i < 20; i++) {
            map[i] = *datagen->nextItem();
        }
        auto elapsed = s.getElapsedNanos();
        // std::cout << "elapsed: " << elapsed << std::endl;
        latency_.update(elapsed);
    }
    void doWork(fds_threadpool *tp) {
        work();
        tp->schedule(&ThreadPoolTest::workDoneCb, this);
    }
    void workDoneCb() {
        completed_++;
        if (issued_ == completed_) {
           endTs_ = util::getTimeStampNanos();
        }
    }

    void doWork2(ThreadPool *tp) {
        work();
        tp->enqueue(&ThreadPoolTest::workDoneCb, this);
    }

    std::atomic<int> issued_;
    std::atomic<int> completed_;
    util::TimeStamp startTs_;
    util::TimeStamp endTs_;
    LatencyCounter latency_;
};

#if 0
struct DoWorkTask : Runnable {
    DoWorkTask(ThreadPoolTest* tf, ThreadManager* tm) {
        tf_ = tf;
        tm_ = tm;
    }
    virtual void run() {
        tf_->work();
    }
    ThreadPoolTest *tf_;
    ThreadManager* tm_;
};

struct WorkDoneTask : Runnable {
    explicit WorkDoneTask(ThreadPoolTest* tf) {
        tf_ = tf;
    }
    virtual void run() {
        tf_->workDoneCb();
    }
    ThreadPoolTest *tf_;
};
#endif


TEST_F(ThreadPoolTest, fdsthreadpool)
{
    int cnt = this->getArg<int>("cnt");
    std::unique_ptr<fds_threadpool> tp(new fds_threadpool());
    startTs_ = util::getTimeStampNanos();
    for (int i = 0; i < cnt; i++) {
        issued_++;
        tp->schedule(&ThreadPoolTest::doWork, this, tp.get());
    }
    /* Poll for completion */
    POLL_MS((issued_ == completed_), issued_, (issued_* 3));
    std::cout << "Total Time taken: " << endTs_ - startTs_ << "(ns)\n"
            << "Avg time taken: " << (static_cast<double>(endTs_ - startTs_)) / issued_
            << "(ns)\n";
    std::cout << "Avg latency : " << latency_.latency() << " (ns)\n";
    ASSERT_TRUE(issued_ == completed_) << "Issued: " << issued_
        << " completed_: " << completed_;
}

TEST_F(ThreadPoolTest, singlethread)
{
    ASSERT_EQ(completed_, 0);
    int cnt = this->getArg<int>("cnt");
    startTs_ = util::getTimeStampNanos();
    for (int i = 0; i < cnt; i++) {
        issued_++;
        work();
        workDoneCb();
    }
    /* Poll for completion */
    POLL_MS((issued_ == completed_), issued_, (issued_* 3));
    std::cout << "Total Time taken: " << endTs_ - startTs_ << "(ns)\n"
            << "Avg time taken: " << (static_cast<double>(endTs_ - startTs_)) / issued_
            << "(ns)\n";
    std::cout << "Avg latency : " << latency_.latency() << " (ns)\n";
    ASSERT_TRUE(issued_ == completed_) << "Issued: " << issued_
        << " completed_: " << completed_;
}

TEST_F(ThreadPoolTest, threadpool)
{
    int cnt = this->getArg<int>("cnt");
    std::unique_ptr<ThreadPool> tp(new ThreadPool(10));
    startTs_ = util::getTimeStampNanos();
    for (int i = 0; i < cnt; i++) {
        issued_++;
        tp->enqueue(&ThreadPoolTest::doWork2, this, tp.get());
    }
    /* Poll for completion */
    POLL_MS((issued_ == completed_), issued_, (issued_* 3));
    std::cout << "Total Time taken: " << endTs_ - startTs_ << "(ns)\n"
            << "Avg time taken: " << (static_cast<double>(endTs_ - startTs_)) / issued_
            << "(ns)\n";
    std::cout << "Avg latency : " << latency_.latency() << " (ns)\n";
    ASSERT_TRUE(issued_ == completed_) << "Issued: " << issued_
        << " completed_: " << completed_;
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message")
        ("cnt", po::value<int>()->default_value(10000), "cnt");
    ThreadPoolTest::init(argc, argv, opts);
    return RUN_ALL_TESTS();
}
