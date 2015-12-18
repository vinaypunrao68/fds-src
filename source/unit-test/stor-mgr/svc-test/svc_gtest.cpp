/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#define GTEST_USE_OWN_TR1_TUPLE 0

#include <unistd.h>
#include <string>
#include <iostream>
#include <stdexcept>
#include <boost/make_shared.hpp>
#include <net/SvcRequestPool.h>
#include <ObjectId.h>
#include <fiu-control.h>
#include <testlib/DataGen.hpp>
#include <testlib/SvcMsgFactory.h>
#include <testlib/TestUtils.h>
#include <testlib/TestFixtures.h>
#include "fdsp/ConfigurationService.h"
#include "fdsp/sm_api_types.h"
#include <util/fiu_util.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <google/profiler.h>
// #include "IThreadpool.h"

using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;  // NOLINT

struct SMApi : BaseTestFixture
{
    SMApi() {
        putsIssued_ = 0;
        putsSuccessCnt_ = 0;
        putsFailedCnt_ = 0;
    }
    static void SetUpTestCase() {
        // tp = new IThreadpool(10);
        platform_.reset(new TestPlatform(argc_, argv_, "SingleNodeTest.log", nullptr));
        platform_->start_modules();
    }
    static void TearDownTestCase() {
        // delete tp;
        platform_->shutdown_modules();
    }
    void putCb(uint64_t opStartTs, EPSvcRequest* svcReq,
               const Error& error,
               boost::shared_ptr<std::string> payload)
    {
        auto opEndTs = util::getTimeStampNanos();
        avgLatency_.update(opEndTs - opStartTs);
        if (error != ERR_OK) {
            GLOGWARN << "Req Id: " << svcReq->getRequestId() << " " << error;
            putsFailedCnt_++;
        } else {
            putsSuccessCnt_++;
        }
        if (putsIssued_ == (putsSuccessCnt_ + putsFailedCnt_)) {
           endTs_ = util::getTimeStampNanos();
        }
    }

    static std::unique_ptr<TestPlatform> platform_;
    // static IThreadpool *tp;
    std::atomic<uint32_t> putsIssued_;
    std::atomic<uint32_t> putsSuccessCnt_;
    std::atomic<uint32_t> putsFailedCnt_;
    util::TimeStamp startTs_;
    util::TimeStamp endTs_;
    LatencyCounter avgLatency_;
};
std::unique_ptr<TestPlatform> SMApi::platform_;
// IThreadpool* SMApi::tp;


void invokeWork(SMApi *smapi,
        uint64_t opStartTs,
        SvcRequestId reqId,
        fpi::FDSPMsgTypeId msgTypeId,
        fpi::SvcUuid srcUuid,
        fpi::SvcUuid dstUuid)
{
    // auto header = SvcRequestPool::newSvcRequestHeader(reqId, msgTypeId, srcUuid, dstUuid);
    // header.msg_type_id = msgTypeId;

    try {
        auto respHdr = gSvcRequestPool->\
                       newSvcRequestHeaderPtr(reqId, msgTypeId, dstUuid, srcUuid,
                                              DLT_VER_INVALID, 0, 0);
        respHdr->msg_code = ERR_SVC_REQUEST_INVOCATION;
        // auto hdr = new fpi::AsyncHdr();
        // auto respHdr = SvcRequestPool::newSvcRequestHeader(reqId, msgTypeId, srcUuid, dstUuid);
        // respHdr.msg_code = ERR_SVC_REQUEST_INVOCATION;
        // smapi->putCb(opStartTs, nullptr, ERR_OK, nullptr);
        // gSvcRequestPool->postError(respHdr);
        MODULEPROVIDER()->proc_thrpool()->schedule(&SMApi::putCb, smapi,
                                                  opStartTs, nullptr, ERR_OK, nullptr);
        // smapi->tp->enqueue(std::bind(&SMApi::putCb, smapi, opStartTs, nullptr, ERR_OK, nullptr));
    } catch (std::exception &e) {
    }
    return;
}

void tempInvoke(SMApi *smapi, uint64_t opStartTs,
    SvcRequestId reqId,
    fpi::FDSPMsgTypeId msgTypeId,
    fpi::SvcUuid srcUuid,
    fpi::SvcUuid dstUuid)
{
    invokeWork(smapi, opStartTs, reqId,
            msgTypeId,
            srcUuid,
            dstUuid);
    return;
}

/**
* @brief Tests dropping puts fault injection
*
*/
TEST_F(SMApi, putsPerf2)
{
    int dataCacheSz = 100;
    int nPuts =  this->getArg<int>("puts-cnt");
    bool profile = this->getArg<bool>("profile");
    bool failSendsbefore = this->getArg<bool>("failsends-before");
    bool failSendsafter = this->getArg<bool>("failsends-after");
    bool uturnAsyncReq = this->getArg<bool>("uturn-asyncreqt");
    bool uturnPuts = this->getArg<bool>("uturn");
    bool disableSchedule = this->getArg<bool>("disable-schedule");

    fpi::SvcUuid svcUuid;
    svcUuid.svc_uuid = this->getArg<uint64_t>("smuuid");

    /* To generate random data between 10 to 100 bytes */
    auto datagen = boost::make_shared<RandDataGenerator<>>(4096, 4096);
    auto putMsgGenF = std::bind(&SvcMsgFactory::newPutObjectMsg, 1, datagen);
    /* To geenrate put messages and cache them.  Wraps datagen from above */
    auto putMsgGen = boost::make_shared<
                    CachedMsgGenerator<fpi::PutObjectMsg>>(dataCacheSz,
                                                           true,
                                                           putMsgGenF);

    /* Start timer */
    startTs_ = util::getTimeStampNanos();

    if (profile) {
        ProfilerStart("/tmp/output.prof");
    }

    /* Issue puts */
    for (int i = 0; i < nPuts; i++) {
        auto opStartTs = util::getTimeStampNanos();
        auto putObjMsg = putMsgGen->nextItem();
        auto asyncPutReq = gSvcRequestPool->newEPSvcRequest(svcUuid);
        // auto asyncPutReq = boost::make_shared<EPSvcRequest>();
        asyncPutReq->setPayload(FDSP_MSG_TYPEID(fpi::PutObjectMsg), putObjMsg);
        asyncPutReq->onResponseCb(std::bind(&SMApi::putCb, this, opStartTs,
                                            std::placeholders::_1,
                                            std::placeholders::_2, std::placeholders::_3));
        asyncPutReq->setTimeoutMs(1000);
        putsIssued_++;
        tempInvoke(this, opStartTs,
                asyncPutReq->getRequestId(),
                FDSP_MSG_TYPEID(fpi::PutObjectMsg),
                svcUuid, svcUuid);
    }

    std::cout << "Completed " << putsSuccessCnt_ << std::endl;

    /* Poll for completion */
    POLL_MS((putsIssued_ == putsSuccessCnt_ + putsFailedCnt_), 2000, (nPuts * 10));

    if (profile) {
        ProfilerStop();
    }

    std::cout << "Total Time taken: " << endTs_ - startTs_ << "(ns)\n"
            << "Avg time taken: " << (static_cast<double>(endTs_ - startTs_)) / putsIssued_
            << "(ns) Avg op latency: " << avgLatency_.value() << std::endl;
    ASSERT_TRUE(putsIssued_ == putsSuccessCnt_) << "putsIssued: " << putsIssued_
        << " putsSuccessCnt_: " << putsSuccessCnt_
        << " putsFailedCnt_: " << putsFailedCnt_;
}

TEST(Perf, allocation_4k)
{
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message")
        ("smuuid", po::value<uint64_t>()->default_value(0), "smuuid")
        ("puts-cnt", po::value<int>(), "puts count")
        ("profile", po::value<bool>()->default_value(false), "google profile")
        ("failsends-before", po::value<bool>()->default_value(false), "fail sends before")
        ("failsends-after", po::value<bool>()->default_value(false), "fail sends after")
        ("uturn-asyncreqt", po::value<bool>()->default_value(false), "uturn async reqt")
        ("uturn", po::value<bool>()->default_value(false), "uturn")
        ("disable-schedule", po::value<bool>()->default_value(false), "disable scheduling");
    SMApi::init(argc, argv, opts);
    return RUN_ALL_TESTS();
}
