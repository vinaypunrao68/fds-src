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

#if 0
void taskFunc(int qid, int seqId)
{
    // std::cout << "qid: " << qid << " seqId: " << seqId << std::endl;
    fds_assert(inprogMap[qid] == false)

    inprogMap[qid] = true;

    for (uint32_t i = 0; i < nReps; i++) {}

    inprogMap[qid] = false;
    completedCnt++;
}

void taskFunc2(int qid, int seqId)
{
    fds::fds_scoped_lock l(locks[qid]);
    fds_assert(inprogMap[qid] == false)

    inprogMap[qid] = true;

    for (uint32_t i = 0; i < nReps; i++) {}

    inprogMap[qid] = false;
    completedCnt++;
}

/**
 * Tests schedule function of SynchronizedTaskExecutor
 */
TEST(SynchronizedTaskExecutor, schedule) {
    completedCnt = 0;

    for (int i = 0; i < nQCnt; i++) {
        inprogMap[i] = false;
    }

    for (int i = 0; i < nReqs; i++) {
        auto qid = i%nQCnt;
        executor.scheduleOnHashKey(qid, std::bind(taskFunc, qid, i));
    }

    while (completedCnt < nReqs) {
        sleep(1);
    }
}


/**
 * Tests schedule function of SynchronizedTaskExecutor
 */
TEST(Threadpool, schedule) {
    completedCnt = 0;
    fds::fds_threadpool tp;

    for (int i = 0; i < nQCnt; i++) {
        inprogMap[i] = false;
    }

    for (int i = 0; i < nReqs; i++) {
        auto qid = i%nQCnt;
        tp.schedule(taskFunc2, qid, i);
    }

    while (completedCnt < nReqs) {
        sleep(1);
    }
}
#endif

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

int main(int argc, char** argv) {
    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
