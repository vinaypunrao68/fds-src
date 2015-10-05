/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#define GTEST_USE_OWN_TR1_TUPLE 0
#include <cstdlib>
#include <ctime>
#include <set>
#include <iostream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <concurrency/ThreadPool.h>
#include <concurrency/SynchronizedTaskExecutor.hpp>
#include <testlib/TestFixtures.h>
#include <testlib/DataGen.hpp>
#include <leveldb/db.h>
#include <concurrency/taskstatus.h>
#include <util/timeutils.h>
#include <chrono>

using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;  // NOLINT

namespace fds_test {
std::unordered_map<int, std::atomic<bool>> inprogMap;
std::atomic<int> completedCnt;
const int nQCnt = 10000;
int nReqs = nQCnt * 5;
uint32_t nReps = 10000;

fds::fds_threadpool tp;
fds::SynchronizedTaskExecutor<int> executor(tp);
fds::fds_mutex locks[nQCnt];

struct PutReq {
public:
    PutReq(int32_t id) {
        id_ = id;
        putResponsded_ = false;
        updateCatResponded_ = false;
    }

    void putObjectRespLocked()
    {
        putResponsded_ = true;
        checkAndDeleteLocked();
    }

    void updateCatalogRespLocked()
    {
        updateCatResponded_ = true;
        checkAndDeleteLocked();
    }

    void checkAndDeleteLocked()
    {
        bool del = false;
        {
            fds::fds_scoped_lock l(lock_);
            del = putResponsded_ && updateCatResponded_;
        }
        if (del) {
            // delete this;
            completedCnt++;
        }
    }

    void putObjectResp()
    {
        putResponsded_ = true;
        checkAndDelete();
    }

    void updateCatalogResp()
    {
        updateCatResponded_ = true;
        checkAndDelete();
    }

    void checkAndDelete()
    {
        bool del = putResponsded_ && updateCatResponded_;
        if (del) {
            // delete this;
            completedCnt++;
        }
    }

    fds::fds_mutex lock_;
    int32_t id_;
    bool putResponsded_;
    bool updateCatResponded_;
};

struct SH;

struct Sm {
    void putObjectLocked(PutReq *req, SH *sh);
    void putObject(PutReq *req, SH *sh);
};

struct Dm {
    void updateCatalogLocked(PutReq *req, SH *sh);
    void updateCatalog(PutReq *req, SH *sh);
};

struct SH {
    void issuePutLocked(int32_t id);
    void issuePut(int32_t id);
    void putObjectRespLocked(PutReq *r);
    void updateCatalogRespLocked(PutReq *r);
    void putObjectResp(PutReq *r);
    void updateCatalogResp(PutReq *r);

    Sm sm_;
    Dm dm_;
};

void Sm::putObjectLocked(PutReq *req, SH *sh)
{
    tp.schedule(&SH::putObjectRespLocked, sh, req);
}
void Sm::putObject(PutReq *req, SH *sh)
{
    tp.schedule(&SH::putObjectResp, sh, req);
}
void Dm::updateCatalogLocked(PutReq *req, SH *sh)
{
    tp.schedule(&SH::updateCatalogRespLocked, sh, req);
}
void Dm::updateCatalog(PutReq *req, SH *sh)
{
    tp.schedule(&SH::updateCatalogResp, sh, req);
}
void SH::issuePutLocked(int32_t id) {
    PutReq *r = new PutReq(id);
    sm_.putObjectLocked(r, this);
    dm_.updateCatalogLocked(r, this);
}
void SH::issuePut(int32_t id) {
    PutReq *r = new PutReq(id);
    sm_.putObject(r, this);
    dm_.updateCatalog(r, this);
}
void SH::putObjectRespLocked(PutReq *r)
{
    r->putObjectRespLocked();
}

void SH::updateCatalogRespLocked(PutReq *r)
{
    r->updateCatalogRespLocked();
}

void SH::putObjectResp(PutReq *r)
{
    executor.scheduleOnHashKey(r->id_, std::bind(&PutReq::putObjectResp, r));
}

void SH::updateCatalogResp(PutReq *r)
{
    executor.scheduleOnHashKey(r->id_, std::bind(&PutReq::updateCatalogResp, r));
}


/**
 * Tests schedule function of SynchronizedTaskExecutor
 */
TEST(SynchronizedTaskExecutor, putreq) {
    completedCnt = 0;
    SH sh;
    for (int i = 0; i < nReqs; i++) {
        tp.schedule(&SH::issuePut, &sh, i);
    }
    while (completedCnt < nReqs) {
        sleep(1);
    }
}

#if 0
/**
 * Tests schedule function of SynchronizedTaskExecutor
 */
TEST(Threadpool, putReq) {
    completedCnt = 0;
    SH sh;
    for (int i = 0; i < nReqs; i++) {
        tp.schedule(&SH::issuePutLocked, &sh, i);
    }
    while (completedCnt < nReqs) {
        sleep(1);
    }
}
#endif
}  // namespace fds_test

struct ExecutorFixture :  BaseTestFixture {
    void SetUp() override {
        db1Path = "/tmp/testdb1";
        auto res = std::system((std::string("rm -rf ") + db1Path).c_str());

        leveldb::Options options;
        options.create_if_missing = true;
        auto status = leveldb::DB::Open(options, db1Path, &db1);
        ASSERT_TRUE(status.ok());

    }
    void closeDbs() {
        if (db1) {
            delete db1;
            db1 = nullptr;
        }
    }
    void TearDown() override {
        closeDbs();
    }
    static void SetUpTestCase() {
        keyGen = new CachedRandDataGenerator<>(1000, true, 10, 25);
        valueGen = new CachedRandDataGenerator<>(1000, true, 10, 100);
        threadpool = new fds_threadpool(10, true);
    }
    static void TearDownTestCase() {
        delete keyGen;
        delete valueGen;
        delete threadpool;
    }
    void testPuts(bool enableAffinity) {
        SynchronizedTaskExecutor<size_t> executor(*threadpool, enableAffinity);
        std::hash<std::string> hashFn;
        int numTasks = this->getArg<int>("numTasks");
        std::atomic<int> completedTasks;
        completedTasks = 0;
        util::TimeStamp startTs;
        util::TimeStamp endTs;
        concurrency::TaskStatus status;

        auto task = [&enableAffinity, &hashFn, &numTasks, &completedTasks, &endTs, &status](int counter, leveldb::DB* db, StringPtr k, StringPtr v) {
            static thread_local int tid = -1;
            auto taskId = hashFn(*k) % 10;
            if (enableAffinity) {
                /* Affinity check to make sure same thread executes tasks with same id */
                if (tid == -1) {
                    tid = taskId;
                } else {
                    ASSERT_EQ(tid, taskId);
                }
            }

            if (counter % 2 == 0) {
                db->Put(leveldb::WriteOptions(),
                        leveldb::Slice(*k),
                        leveldb::Slice(*v));
            } else {
                std::string value;
                leveldb::Status s = db->Get(leveldb::ReadOptions(), *k, &value);
            }
            completedTasks++;

            if (completedTasks == numTasks) {
                endTs = util::getTimeStampMicros();
                status.done();
            }
        };

        startTs = util::getTimeStampMicros();
        for (int32_t i = 0; i < numTasks; i++) {
            auto k = keyGen->nextItem();
            auto v = keyGen->nextItem();

            executor.scheduleOnHashKey(hashFn(*k), std::bind(task, i, db1, k, v));
        }

        status.await();
        std::cout << "elapsed time: " << endTs-startTs << "micros" << std::endl;
    }

    std::string db1Path;
    leveldb::DB* db1;
    static CachedRandDataGenerator<> *keyGen;
    static CachedRandDataGenerator<> *valueGen;
    static fds_threadpool *threadpool;
};

CachedRandDataGenerator<>* ExecutorFixture::keyGen;
CachedRandDataGenerator<>* ExecutorFixture::valueGen;
fds_threadpool* ExecutorFixture::threadpool;


TEST_F(ExecutorFixture, testSchedule) {
    testPuts(false);
}
TEST_F(ExecutorFixture, testScheduleAffinity) {
    testPuts(true);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message")
        ("numTasks", po::value<int>()->default_value(1000), "num tasks");
    ExecutorFixture::init(argc, argv, opts);
    return RUN_ALL_TESTS();
}
