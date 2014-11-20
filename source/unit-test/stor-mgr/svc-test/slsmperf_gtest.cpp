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
#include <testlib/Datasets.h>
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
        getsIssued_ = 0;
        getsSuccessCnt_ = 0;
        getsFailedCnt_ = 0;

    }
    void putCb(uint64_t opStartTs, EPSvcRequest* svcReq,
               const Error& error,
               boost::shared_ptr<std::string> payload)
    {
        auto opEndTs = util::getTimeStampNanos();
        avgPutLatency_.update(opEndTs - opStartTs);
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
    void getCb(uint64_t opStartTs, EPSvcRequest* svcReq,
               const Error& error,
               boost::shared_ptr<std::string> payload)
    {
        auto opEndTs = util::getTimeStampNanos();
        avgGetLatency_.update(opEndTs - opStartTs);
        outStanding_--;
        {
            Synchronized s(monitor_);
            if (waiting_ && outStanding_ < concurrency_) {
                monitor_.notify();
            }
        }

        if (error != ERR_OK) {
            GLOGWARN << "Req Id: " << svcReq->getRequestId() << " " << error;
            getsFailedCnt_++;
        } else {
            getsSuccessCnt_++;
        }
        if (getsIssued_ == (getsSuccessCnt_ + getsFailedCnt_)) {
           endTs_ = util::getTimeStampNanos();
        }
    }

 protected:
    std::atomic<uint32_t> putsIssued_;
    std::atomic<uint32_t> outStanding_;
    std::atomic<uint32_t> putsSuccessCnt_;
    std::atomic<uint32_t> putsFailedCnt_;
    // gets
    std::atomic<uint32_t> getsIssued_;
    std::atomic<uint32_t> getsSuccessCnt_;
    std::atomic<uint32_t> getsFailedCnt_;

    Monitor monitor_;
    bool waiting_;
    uint32_t concurrency_;
    util::TimeStamp startTs_;
    util::TimeStamp endTs_;
    LatencyCounter avgPutLatency_;
    LatencyCounter avgGetLatency_;

    std::ofstream statfile_;
};

/**
* @brief Tests dropping puts fault injection
*
*/
TEST_F(SMApi, putsPerf)
{
    int nPuts =  this->getArg<int>("puts-cnt");
    int nGets =  this->getArg<int>("gets-cnt");
    if (nGets == 0) {
      nGets = 10*nPuts;
    }
    bool profile = this->getArg<bool>("profile");
    bool failSendsbefore = this->getArg<bool>("failsends-before");
    bool failSendsafter = this->getArg<bool>("failsends-after");
    bool uturnAsyncReq = this->getArg<bool>("uturn-asyncreqt");
    bool uturn = this->getArg<bool>("uturn");
    bool disableSchedule = this->getArg<bool>("disable-schedule");
    bool largeFrame = this->getArg<bool>("largeframe");
    bool lftp = this->getArg<bool>("lftp");
    concurrency_ = this->getArg<uint32_t>("concurrency");
    if (concurrency_ == 0) {
        concurrency_  = nPuts;
    }
    bool dropCaches = false;

    std::string fname("sl_sm_perf_stats.csv");
    statfile_.open(fname.c_str(), std::ios::out | std::ios::app);

    fpi::SvcUuid svcUuid;
    svcUuid.svc_uuid = this->getArg<uint64_t>("smuuid");
    if (svcUuid.svc_uuid == 0) {
        svcUuid = TestUtils::getAnyNonResidentSmSvcuuid(gModuleProvider->get_plf_manager());
    }
    ASSERT_NE(svcUuid.svc_uuid, 0);;
    DltTokenGroupPtr tokGroup = boost::make_shared<DltTokenGroup>(1);
    tokGroup->set(0, NodeUuid(svcUuid));
    auto epProvider = boost::make_shared<DltObjectIdEpProvider>(tokGroup);

    // create dataset of size nPuts
    FdsObjectDataset dataset;
    dataset.generateDataset(nPuts, 4096);

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
    if (uturn) {
        ASSERT_TRUE(TestUtils::enableFault(svcUuid, "svc.uturn.putobject"));
        ASSERT_TRUE(TestUtils::enableFault(svcUuid, "svc.uturn.getobject"));
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
    for (int i = 0; i < nPuts; i++) {
        auto opStartTs = util::getTimeStampNanos();
	ObjectID oid = dataset.dataset_[i];
        auto putObjMsg = SvcMsgFactory::newPutObjectToSmMsg(volId_, oid,
                                                        dataset.dataset_map_[oid].getObjectData());
        auto asyncPutReq = gSvcRequestPool->newEPSvcRequest(svcUuid);
        asyncPutReq->setPayload(FDSP_MSG_TYPEID(fpi::PutObjectMsg), putObjMsg);
        asyncPutReq->onResponseCb(std::bind(&SMApi::putCb, this, opStartTs,
                                            std::placeholders::_1,
                                            std::placeholders::_2, std::placeholders::_3));
        asyncPutReq->setTimeoutMs(5000);
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

    double throughput = (static_cast<double>(1000000000) /
                           (endTs_ - startTs_)) * putsIssued_;
    std::cout << "Total Time taken: " << endTs_ - startTs_ << "(ns)\n"
            << "putsCnt: " << putsIssued_ << "\n"
            << "Throughput: " << throughput << "\n"
            << "Avg time taken: " << (static_cast<double>(endTs_ - startTs_)) / putsIssued_
            << "(ns) Avg put latency: " << avgPutLatency_.value() << std::endl
            << "svc sendLat: " << gSvcRequestCntrs->sendLat.value() << std::endl
            << "svc sendPayloadLat: " << gSvcRequestCntrs->sendPayloadLat.value() << std::endl
            << "svc serialization latency: " << gSvcRequestCntrs->serializationLat.value() << std::endl
            << "svc op latency: " << gSvcRequestCntrs->reqLat.value() << std::endl;

    std::cout << "putsIssued: " << putsIssued_
              << " putsSuccessCnt_: " << putsSuccessCnt_
              << " putsFailedCnt_: " << putsFailedCnt_ << std::endl;

    double latMs = static_cast<double>(avgPutLatency_.value()) / 1000000.0;
    statfile_ << "put," << putsIssued_ << "," << concurrency_ << ","
              << throughput << "," << latMs << std::endl;

    // do GETs (=10x number of PUTs)
    // reset counters
    outStanding_ = 0;
    waiting_ = false;

    // only makes sense on localhost
    if (dropCaches) {
      const std::string cmd = "echo 3 > /proc/sys/vm/drop_caches";
      int ret = std::system((const char *)cmd.c_str());
      fds_verify(ret == 0);
      std::cout << "sleeping after drop_caches" << std::endl;
      sleep(5);
      std::cout << "finished sleeping" << std::endl;
    }

    /* Start google profiler */
    if (profile) {
        ProfilerStart("/tmp/slsmget_output.prof");
    }

    /* Start timer */
    startTs_ = util::getTimeStampNanos();
    fds_uint32_t cur_ind = 0;
    fds_uint32_t dataset_size = nPuts;
    for (int i = 0; i < nGets; i++) {
        auto opStartTs = util::getTimeStampNanos();
	fds_uint32_t index = cur_ind % dataset_size;
        cur_ind += 123;
	ObjectID oid = dataset.dataset_[index];
        auto getObjMsg = SvcMsgFactory::newGetObjectMsg(volId_, oid);
        auto asyncGetReq = gSvcRequestPool->newEPSvcRequest(svcUuid);
        asyncGetReq->setPayload(FDSP_MSG_TYPEID(fpi::GetObjectMsg), getObjMsg);
        asyncGetReq->onResponseCb(std::bind(&SMApi::getCb, this, opStartTs,
                                            std::placeholders::_1,
                                            std::placeholders::_2, std::placeholders::_3));
        asyncGetReq->setTimeoutMs(5000);
        getsIssued_++;
        outStanding_++;
        asyncGetReq->invoke();

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
    POLL_MS((getsIssued_ == getsSuccessCnt_ + getsFailedCnt_), 2000, nGets);

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
    if (uturn) {
        ASSERT_TRUE(TestUtils::disableFault(svcUuid, "svc.uturn.putobject"));
        ASSERT_TRUE(TestUtils::disableFault(svcUuid, "svc.uturn.getobject"));
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

    throughput = (static_cast<double>(1000000000) /
                           (endTs_ - startTs_)) * getsIssued_;
    std::cout << "GET experiment concurrency " << concurrency_ << std::endl;
    std::cout << "Total Time taken: " << endTs_ - startTs_ << "(ns)\n"
            << "getsCnt: " << getsIssued_ << "\n"
            << "Throughput: " << throughput << "\n"
            << "Avg time taken: " << (static_cast<double>(endTs_ - startTs_)) / getsIssued_
            << "(ns) Avg op latency: " << avgGetLatency_.value() << std::endl;

    std::cout << "putsIssued: " << getsIssued_
              << " putsSuccessCnt_: " << getsSuccessCnt_
              << " putsFailedCnt_: " << getsFailedCnt_ << std::endl;

    latMs = static_cast<double>(avgGetLatency_.value()) / 1000000.0;
    statfile_ << "get," << putsIssued_ << "," << concurrency_ << ","
              << throughput << "," << latMs << std::endl;
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message")
        ("smuuid", po::value<uint64_t>()->default_value(0), "smuuid")
        ("puts-cnt", po::value<int>(), "puts count")
        ("gets-cnt", po::value<int>()->default_value(0), "gets count")
        ("profile", po::value<bool>()->default_value(false), "google profile")
        ("failsends-before", po::value<bool>()->default_value(false), "fail sends before")
        ("failsends-after", po::value<bool>()->default_value(false), "fail sends after")
        ("uturn-asyncreqt", po::value<bool>()->default_value(false), "uturn async reqt")
        ("uturn", po::value<bool>()->default_value(false), "uturn")
        ("largeframe", po::value<bool>()->default_value(false), "largeframe")
        ("output", po::value<std::string>()->default_value("stats.txt"), "stats output")
        ("disable-schedule", po::value<bool>()->default_value(false), "disable scheduling")
        ("lftp", po::value<bool>()->default_value(false), "use lockfree threadpool")
        ("concurrency", po::value<uint32_t>()->default_value(0), "concurrency");
    SMApi::init(argc, argv, opts, "vol1");
    return RUN_ALL_TESTS();
}
