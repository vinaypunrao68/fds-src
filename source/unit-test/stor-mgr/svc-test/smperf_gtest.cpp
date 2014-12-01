/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#define GTEST_USE_OWN_TR1_TUPLE 0

#include <unistd.h>
#include <string>
#include <iostream>
#include <boost/make_shared.hpp>
#include <platform/platform-lib.h>
#include <net/SvcRequestPool.h>
#include <fdsp_utils.h>
#include <ObjectId.h>
#include <fiu-control.h>
#include <testlib/DataGen.hpp>
#include <testlib/SvcMsgFactory.h>
#include <testlib/TestUtils.h>
#include <testlib/TestFixtures.h>
#include <apis/ConfigurationService.h>
#include <thrift/concurrency/Monitor.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <google/profiler.h>

using ::testing::AtLeast;
using ::testing::Return;
using namespace apache::thrift; // NOLINT
using namespace apache::thrift::concurrency; // NOLINT
using namespace fds;  // NOLINT

struct SMApi : SingleNodeTest
{
    SMApi() {
        outStanding_ = 0;
        concurrency_ = 0;
        waiting_ = false;
        putsIssued_ = 0;
        putsSuccessCnt_ = 0;
        putsFailedCnt_ = 0;
    }
    void putCb(uint64_t opStartTs, EPSvcRequest* svcReq,
               const Error& error,
               boost::shared_ptr<std::string> payload)
    {
        if (opTs_.size() > 0) {
            opTs_[svcReq->getRequestId()] = svcReq->ts;
        }

        auto opEndTs = util::getTimeStampNanos();
        avgLatency_.update(opEndTs - opStartTs);
        outStanding_--;
        {
            Synchronized s(monitor_);
            if (waiting_ && outStanding_ < concurrency_) {
                monitor_.notify();
            }
        }

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
    void printOpTs(const std::string fileName) {
        std::ofstream o(fileName);
        for (auto &t : opTs_) {
            fds_verify(t.rqSendStartTs != 0 && t.rqSendEndTs != 0);
            o << t.rqStartTs << "\t"
                << t.rqSendStartTs << "\t"
                << t.rqSendEndTs << "\t"
                << t.rqRcvdTs << "\t"
                << t.rqHndlrTs << "\t"
                << t.rspSerStartTs << "\t"
                << t.rspSendStartTs << "\t"
                << t.rspRcvdTs << "\t"
                << t.rspHndlrTs << "\n";
        }
        o.close();
        std::cout << "||st-snd|"
            << "rq-snd|"
            << "rq-snd-rcv|"
            << "rq-rcv-hndlr|"
            << "rsp-hndlr-ser|"
            << "rsp-ser|"
            << "rsp-snd-rcv|"
            << "rsp-rcv-hndlr|"
            << "total||\n";

        for (uint64_t i = 0; i < opTs_.size(); i+=100) {
            auto &t = opTs_[i];
            std::cout << "|"
                << t.rqSendStartTs - t.rqStartTs << "|"
                << t.rqSendEndTs - t.rqSendStartTs << "|"
                << t.rqRcvdTs - t.rqSendEndTs << "|"
                << t.rqHndlrTs - t.rqRcvdTs << "|"
                << t.rspSerStartTs - t.rqHndlrTs << "|"
                << t.rspSendStartTs - t.rspSerStartTs << "|"
                << t.rspRcvdTs - t.rspSendStartTs << "|"
                << t.rspHndlrTs - t.rspRcvdTs << "|"
                << t.rspHndlrTs - t.rqStartTs
                << "|\n";
        }

        /* Compute averages */
        uint64_t st_snd_avg = 0;
        uint64_t rq_snd_avg = 0;
        uint64_t rq_snd_rcv_avg = 0;
        uint64_t rq_rcv_hndlr_avg = 0;
        uint64_t rsp_hndlr_ser_avg = 0;
        uint64_t rsp_ser_avg = 0;
        uint64_t rsp_snd_rcv_avg = 0;
        uint64_t rsp_rcv_hndlr_avg = 0;
        uint64_t total_avg = 0;
        for (auto &t : opTs_) {
            st_snd_avg += (t.rqSendStartTs - t.rqStartTs);
            rq_snd_avg += (t.rqSendEndTs - t.rqSendStartTs);
            rq_snd_rcv_avg += (t.rqRcvdTs - t.rqSendEndTs);
            rq_rcv_hndlr_avg += (t.rqHndlrTs - t.rqRcvdTs);
            rsp_hndlr_ser_avg += (t.rspSerStartTs - t.rqHndlrTs);
            rsp_ser_avg += (t.rspSendStartTs - t.rspSerStartTs);
            rsp_snd_rcv_avg += (t.rspRcvdTs - t.rspSendStartTs);
            rsp_rcv_hndlr_avg += (t.rspHndlrTs - t.rspRcvdTs);
            total_avg += (t.rspHndlrTs - t.rqStartTs);
        }
        std::cout << "|"
            << st_snd_avg / opTs_.size() << "|"
            << rq_snd_avg / opTs_.size() << "|"
            << rq_snd_rcv_avg / opTs_.size() << "|"
            << rq_rcv_hndlr_avg / opTs_.size() << "|"
            << rsp_hndlr_ser_avg / opTs_.size() << "|"
            << rsp_ser_avg / opTs_.size() << "|"
            << rsp_snd_rcv_avg / opTs_.size() << "|"
            << rsp_rcv_hndlr_avg / opTs_.size() << "|"
            << total_avg / opTs_.size()
            << "|" << std::endl;
    }
 protected:
    std::atomic<uint64_t> putsIssued_;
    std::atomic<uint64_t> outStanding_;
    std::atomic<uint64_t> putsSuccessCnt_;
    std::atomic<uint64_t> putsFailedCnt_;
    Monitor monitor_;
    bool waiting_;
    uint64_t concurrency_;
    util::TimeStamp startTs_;
    util::TimeStamp endTs_;
    LatencyCounter avgLatency_;
    std::vector<SvcRequestIf::SvcReqTs> opTs_;
};

/**
* @brief Tests dropping puts fault injection
*
*/
TEST_F(SMApi, putsPerf)
{
    int dataCacheSz = 100;
    uint64_t nPuts =  this->getArg<uint64_t>("puts-cnt");
    bool profile = this->getArg<bool>("profile");
    bool failSendsbefore = this->getArg<bool>("failsends-before");
    bool failSendsafter = this->getArg<bool>("failsends-after");
    bool uturnAsyncReq = this->getArg<bool>("uturn-asyncreqt");
    bool uturnPuts = this->getArg<bool>("uturn");
    bool disableSchedule = this->getArg<bool>("disable-schedule");
    bool largeFrame = this->getArg<bool>("largeframe");
    bool lftp = this->getArg<bool>("lftp");
    concurrency_ = this->getArg<uint64_t>("concurrency");
    if (concurrency_ == 0) {
        concurrency_  = nPuts;
    }

    if (nPuts <= 10000) {
        opTs_.resize(nPuts);
    }

    fpi::SvcUuid svcUuid;
    svcUuid.svc_uuid = this->getArg<uint64_t>("smuuid");
    if (svcUuid.svc_uuid == 0) {
        svcUuid = TestUtils::getAnyNonResidentSmSvcuuid(gModuleProvider->get_plf_manager());
    }
    ASSERT_NE(svcUuid.svc_uuid, 0);;
    DltTokenGroupPtr tokGroup = boost::make_shared<DltTokenGroup>(1);
    tokGroup->set(0, NodeUuid(svcUuid));
    auto epProvider = boost::make_shared<DltObjectIdEpProvider>(tokGroup);

    /* To generate random data between 10 to 100 bytes */
    auto datagen = boost::make_shared<RandDataGenerator<>>(4096, 4096);
    auto putMsgGenF = std::bind(&SvcMsgFactory::newPutObjectMsg, volId_, datagen);
    /* To geenrate put messages and cache them.  Wraps datagen from above */
    auto putMsgGen = boost::make_shared<
                    CachedMsgGenerator<fpi::PutObjectMsg>>(dataCacheSz,
                                                           true,
                                                           putMsgGenF);
    /* Set fault to uturn all puts */
    if (failSendsbefore) {
        fiu_enable("svc.fail.sendpayload_before", 1, NULL, 0);
    }
    if (failSendsafter) {
        fiu_enable("svc.fail.sendpayload_after", 1, NULL, 0);
        ASSERT_TRUE(TestUtils::enableFault(svcUuid, "svc.drop.putobject"));
    }
    if (uturnAsyncReq) {
        ASSERT_TRUE(TestUtils::enableFault(svcUuid, "svc.uturn.asyncreqt"));
    }
    if (uturnPuts) {
        ASSERT_TRUE(TestUtils::enableFault(svcUuid, "svc.uturn.putobject"));
    }
    if (disableSchedule) {
        fiu_enable("svc.disable.schedule", 1, NULL, 0);
    }
    if (largeFrame) {
        fiu_enable("svc.largebuffer", 1, NULL, 0);
    }
    if (lftp) {
        fiu_enable("svc.use.lftp", 1, NULL, 0);
        ASSERT_TRUE(TestUtils::enableFault(svcUuid, "svc.use.lftp"));
    }

    /* Start google profiler */
    if (profile) {
        ProfilerStart("/tmp/output.prof");
    }

    /* Start timer */
    startTs_ = util::getTimeStampNanos();

    /* Issue puts */
    for (uint64_t i = 0; i < nPuts; i++) {
        auto opStartTs = util::getTimeStampNanos();
        auto putObjMsg = putMsgGen->nextItem();
        auto asyncPutReq = gSvcRequestPool->newEPSvcRequest(svcUuid);
        asyncPutReq->setPayload(FDSP_MSG_TYPEID(fpi::PutObjectMsg), putObjMsg);
        asyncPutReq->onResponseCb(std::bind(&SMApi::putCb, this, opStartTs,
                                            std::placeholders::_1,
                                            std::placeholders::_2, std::placeholders::_3));
        asyncPutReq->setTimeoutMs(1000);
        putsIssued_++;
        outStanding_++;
        asyncPutReq->invoke();

        {
            Synchronized s(monitor_);
            while (outStanding_ >= concurrency_) {
                waiting_ = true;
                monitor_.wait();
            }
            waiting_ = false;
        }
    }

    /* Poll for completion */
    POLL_MS((putsIssued_ == putsSuccessCnt_ + putsFailedCnt_), 2000, (nPuts * 10));

    /* Disable fault injection */
    if (failSendsbefore) {
        fiu_disable("svc.fail.sendpayload_before");
    }
    if (failSendsafter) {
        fiu_disable("svc.fail.sendpayload_after");
        ASSERT_TRUE(TestUtils::disableFault(svcUuid, "svc.drop.putobject"));
    }
    if (uturnAsyncReq) {
        ASSERT_TRUE(TestUtils::disableFault(svcUuid, "svc.uturn.asyncreqt"));
    }
    if (uturnPuts) {
        ASSERT_TRUE(TestUtils::disableFault(svcUuid, "svc.uturn.putobject"));
    }
    if (disableSchedule) {
        fiu_disable("svc.disable.schedule");
    }
    if (largeFrame) {
        fiu_disable("svc.largebuffer");
    }
    if (lftp) {
        fiu_disable("svc.use.lftp");
        ASSERT_TRUE(TestUtils::disableFault(svcUuid, "svc.use.lftp"));
    }

    /* End profiler */
    if (profile) {
        ProfilerStop();
    }

    double throughput = (static_cast<double>(1000000000) /
                           (endTs_ - startTs_)) * putsIssued_;
    std::cout << "Total Time taken: " << endTs_ - startTs_ << "(ns)\n"
            << "putsCnt: " << putsIssued_ << "\n"
            << "Throughput: " << throughput << "\n"
            << "Avg time taken: " << (static_cast<double>(endTs_ - startTs_)) / putsIssued_
            << "(ns) Avg op latency: " << avgLatency_.value() << std::endl
            << "svc sendLat: " << gSvcRequestCntrs->sendLat.value() << std::endl
            << "svc sendPayloadLat: " << gSvcRequestCntrs->sendPayloadLat.value() << std::endl
            << "svc serialization latency: " << gSvcRequestCntrs->serializationLat.value() << std::endl
            << "svc op latency: " << gSvcRequestCntrs->reqLat.value() << std::endl;
    printOpTs(this->getArg<std::string>("output"));

    ASSERT_TRUE(putsIssued_ == putsSuccessCnt_) << "putsIssued: " << putsIssued_
        << " putsSuccessCnt_: " << putsSuccessCnt_
        << " putsFailedCnt_: " << putsFailedCnt_;
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message")
        ("smuuid", po::value<uint64_t>()->default_value(0), "smuuid")
        ("puts-cnt", po::value<uint64_t>(), "puts count")
        ("profile", po::value<bool>()->default_value(false), "google profile")
        ("failsends-before", po::value<bool>()->default_value(false), "fail sends before")
        ("failsends-after", po::value<bool>()->default_value(false), "fail sends after")
        ("uturn-asyncreqt", po::value<bool>()->default_value(false), "uturn async reqt")
        ("uturn", po::value<bool>()->default_value(false), "uturn")
        ("largeframe", po::value<bool>()->default_value(false), "largeframe")
        ("output", po::value<std::string>()->default_value("stats.txt"), "stats output")
        ("disable-schedule", po::value<bool>()->default_value(false), "disable scheduling")
        ("lftp", po::value<bool>()->default_value(false), "use lockfree threadpool")
        ("concurrency", po::value<uint64_t>()->default_value(0), "concurrency");
    SMApi::init(argc, argv, opts, "vol1");
    return RUN_ALL_TESTS();
}
