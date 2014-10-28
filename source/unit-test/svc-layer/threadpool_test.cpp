/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#define GTEST_USE_OWN_TR1_TUPLE 0

#include <chrono>
#include <atomic>
#include <thread>
#include <string>
#include <vector>
#include <unistd.h>
#include <algorithm>
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
#include "Threadpool.h"
#include "Threadpool2.h"
#include "IThreadpool.h"

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

enum WorkType {
    CPUBOUND,
    HASHTABLE
};

struct Producer {
    Producer(std::function<void (Producer*, util::TimeStamp)> dispFunc,  // NOLINT
            int workType, int dispCnt, uint64_t workIterCnt);
    virtual ~Producer() {}
    void produce();
    void workDoneCb();
    void hashTableWork();
    void work(util::TimeStamp dispTs);
    void join();
    void printStats();

    static void singleThreadDispFunc(Producer *p, util::TimeStamp dispTs);
    static void fdsTpDispFunc(fds_threadpool *tp, Producer *p, util::TimeStamp dispTs);

    struct LockFreeTaskImpl : LockFreeTask {
        explicit LockFreeTaskImpl(Producer *p, util::TimeStamp dispTs) {
            p_ = p;
            dispTs_ = dispTs;
        }
        virtual void operator()() {
            p_->work(dispTs_);
        }
        Producer *p_;
        util::TimeStamp dispTs_;
    };
    static void ITpDispFunc(IThreadpool *tp, Producer *p, util::TimeStamp dispTs);
    static void LFSQDispFunc(LFSQThreadpool *tp, Producer *p, util::TimeStamp dispTs);

    int workType_;
    std::atomic<int> issued_;
    std::atomic<int> completed_;
    util::TimeStamp startTs_;
    util::TimeStamp endTs_;
    uint64_t workTime_;
    uint64_t opTime_;
    std::function<void (Producer*, util::TimeStamp)> dispFunc_;  // NOLINT
    int dispCnt_;
    uint64_t workIterCnt_;

    RandDataGenerator<> dataGen_;

    std::thread t_;
};

Producer::Producer(std::function<void (Producer*, util::TimeStamp)> dispFunc,  // NOLINT
        int workType, int dispCnt, uint64_t workIterCnt)
: issued_(0),
    completed_(0),
    workTime_(0),
    opTime_(0),
    dispFunc_(dispFunc),
    workType_(workType),
    dispCnt_(dispCnt),
    workIterCnt_(workIterCnt),
    dataGen_(1024, 1024),
    t_(std::bind(&Producer::produce, this))
{
}

void Producer::produce() {
    startTs_ = util::getTimeStampNanos();
    /* Do bunch dispatches onto threadpool */
    for (int j = 0; j < dispCnt_; j++) {
        auto dispTs = util::getTimeStampNanos();
        issued_++;
        dispFunc_(this, dispTs);
    }
    /* Wait for work to complete */
    POLL_MS((issued_ == completed_), 2000, (issued_* 2000));
}

void Producer::workDoneCb() {
    completed_++;
    if (issued_ == completed_) {
        endTs_ = util::getTimeStampNanos();
    }
}

void Producer::hashTableWork()
{
    std::unordered_map<uint64_t, StringPtr> ht;
    for (uint64_t i = 0; i < workIterCnt_; i++) {
        ht[i] = boost::make_shared<std::string>(
                "lsjlsjldsjlkdjsljsdsjfdslfjldkfjslsdjllfjdslfjdskljfsljldsjsl");
    }
}

void Producer::work(util::TimeStamp dispTs)
{
    util::TimeStamp workStartTs = util::getTimeStampNanos();
    util::TimeStamp workEndTs;
    std::unordered_map<int, std::string> map;
    if (workType_ == CPUBOUND) {
        uint64_t sum = 0;
        for (uint64_t i = 0; i < workIterCnt_; i++) {
            sum += i * i * i;
        }
    } else if (workType_ == HASHTABLE) {
        hashTableWork();
    }
    workEndTs = util::getTimeStampNanos();
    workTime_ += (workEndTs - workStartTs);
    opTime_ += (workEndTs - dispTs);
    workDoneCb();
}

void Producer::join()
{
    t_.join();
}


void Producer::printStats() {
    std::cout << "Producer stats: \n";
    std::cout << "Issued: " << issued_ << " completed: " << completed_ << std::endl;
    std::cout << "Total time: " << endTs_ - startTs_ << " (ns)\n"
        << "Avg latency: " << static_cast<double>(opTime_) / issued_
        << " (ns)\n";
    std::cout << "Avg worktime: " << static_cast<double>(workTime_) / issued_ << " (ns)\n";
}

void Producer::singleThreadDispFunc(Producer *p, util::TimeStamp dispTs)
{
    p->work(dispTs);
}

void Producer::fdsTpDispFunc(fds_threadpool *tp, Producer *p, util::TimeStamp dispTs)
{
    tp->schedule(&Producer::work, p, dispTs);
}

void Producer::ITpDispFunc(IThreadpool *tp, Producer *p, util::TimeStamp dispTs) {
    // tp->enqueue(new LockFreeTaskImpl(p, dispTs));
    // tp->enqueue(new LockFreeTask(std::bind(&Producer::work, p, dispTs)));
    tp->schedule(&Producer::work, p, dispTs);
}

void Producer::LFSQDispFunc(LFSQThreadpool *tp, Producer *p, util::TimeStamp dispTs) {
    // tp->enqueue(new LockFreeTaskImpl(p, dispTs));
    // tp->enqueue(new LockFreeTask(std::bind(&Producer::work, p, dispTs)));
    tp->schedule(&Producer::work, p, dispTs);
}

struct ThreadPoolTest: BaseTestFixture
{
    ThreadPoolTest()
    {
    }

    uint64_t computeThroughput() {
        auto minItr = std::min_element(producers_.begin(), producers_.end(),
                [](const Producer* a1, const Producer* a2) {
                    return a1->startTs_ < a2->startTs_;
                });
        auto maxItr = std::max_element(producers_.begin(), producers_.end(),
                [](const Producer* a1, const Producer* a2) {
                    return a1->endTs_ < a2->endTs_;
                });
        uint64_t totalOps = producers_.size() * producers_[0]->issued_;
        uint64_t totalLatencyNs = (**maxItr).endTs_ - (**minItr).startTs_;
        uint64_t throughput = (totalOps * 1000 * 1000 * 1000) / totalLatencyNs;
        return throughput;
    }

    void runWorkload(std::function<void (Producer*, util::TimeStamp)> dispFunc)  // NOLINT
    {
        auto workType = this->getArg<int>("type");
        auto nProducers = this->getArg<int>("producers");
        auto dispCnt = this->getArg<int>("cnt");
        uint64_t workIterCnt = this->getArg<uint64_t>("iter-cnt");
        producers_.resize(nProducers);

        /* Start producers */
        for (uint32_t i = 0; i < producers_.size(); i++) {
            producers_[i] = new Producer(dispFunc, workType, dispCnt, workIterCnt);
        }

        /* Join all producer threads */
        double avgLatency = 0;
        double avgWorkTime = 0;
        uint64_t totalWkCnt = 0;
        for (uint32_t i = 0; i < producers_.size(); i++) {
            producers_[i]->join();
            avgLatency += producers_[i]->opTime_;
            avgWorkTime += producers_[i]->workTime_;
            totalWkCnt += producers_[i]->issued_;
            EXPECT_TRUE(producers_[i]->issued_ == producers_[i]->completed_) << "Issued: "
                << producers_[i]->issued_ << " completed_: " << producers_[i]->completed_;
        }
        avgLatency /= totalWkCnt;
        avgWorkTime /= totalWkCnt;
        uint64_t throughput = computeThroughput();
        for (auto &e : producers_) {
            delete e;
        }
        std::cout << "Avg latency: " << avgLatency << " (ns)\n";
        std::cout << "Avg worktime: " << avgWorkTime << " (ns)\n";
        std::cout << "Throughput: " << throughput << "\n";
    }

    std::vector<Producer*> producers_;
};

TEST_F(ThreadPoolTest, singlethread)
{
    runWorkload(std::bind(&Producer::singleThreadDispFunc,
                std::placeholders::_1, std::placeholders::_2));
}

TEST_F(ThreadPoolTest, fdsthreadpool)
{
    int tpSize = this->getArg<int>("tp-size");
    std::unique_ptr<fds_threadpool> tp(new fds_threadpool(tpSize));
    runWorkload(std::bind(&Producer::fdsTpDispFunc, tp.get(),
                std::placeholders::_1, std::placeholders::_2));
}

TEST_F(ThreadPoolTest, ithreadpool)
{
    int tpSize = this->getArg<int>("tp-size");
    std::unique_ptr<IThreadpool> tp(new IThreadpool(tpSize, true));
    runWorkload(std::bind(&Producer::ITpDispFunc, tp.get(),
                std::placeholders::_1, std::placeholders::_2));
}

TEST_F(ThreadPoolTest, lfsqthreadpool)
{
    int tpSize = this->getArg<int>("tp-size");
    std::unique_ptr<LFSQThreadpool> tp(new LFSQThreadpool(tpSize));
    runWorkload(std::bind(&Producer::LFSQDispFunc, tp.get(),
                std::placeholders::_1, std::placeholders::_2));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message")
        ("tp-size", po::value<int>()->default_value(10), "threadpool size")
        ("producers", po::value<int>()->default_value(1), "# of producers")
        ("type", po::value<int>()->default_value(0), "work type [0=cpuboudn, 1=hashtable]")
        ("cnt", po::value<int>()->default_value(10000), "cnt")
        ("profile", po::value<bool>()->default_value(false), "profile")
        ("iter-cnt", po::value<uint64_t>()->default_value(10000), "iter-cnt");
    ThreadPoolTest::init(argc, argv, opts);
    std::cout << std::fixed;
    return RUN_ALL_TESTS();
}
