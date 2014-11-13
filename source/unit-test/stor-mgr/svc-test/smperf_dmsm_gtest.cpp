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
        getsOutStanding_ = 0;
        getsIssued_ = 0;
        getsSuccessCnt_ = 0;
        getsFailedCnt_ = 0;
    }
#if 0
    void putCb(uint64_t opStartTs, EPSvcRequest* svcReq,
               const Error& error,
               boost::shared_ptr<std::string> payload)
    {
        opTs_[svcReq->getRequestId()] = svcReq->ts;

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
#endif

    void dm_sm_put_cb(uint64_t opStartTs, std::shared_ptr<int> num_acks, EPSvcRequest* svcReq,
                      const Error& error,
                      boost::shared_ptr<std::string> payload)
    {
        fds_verify(num_acks > 0);
        if(0 == --*num_acks) 
        {
            auto endTs = util::getTimeStampNanos();
            avgLatency_.update(endTs - opStartTs);
            outStanding_--;
            {
                Synchronized s(monitor_);
                if (waiting_ && outStanding_ < concurrency_) 
                {
                    monitor_.notify();
                 }
            }
         

             if(error != ERR_OK) {
                    putsFailedCnt_++;
             } else {
                putsSuccessCnt_++;
             }

            if(putsIssued_ == (putsSuccessCnt_ + putsFailedCnt_)) {
                endTs_ = util::getTimeStampNanos();
            }
	}
    }


    void dm_sm_get_cb(fpi::SvcUuid sm_svcUuid, uint64_t opStartTs, EPSvcRequest* svcReq,
                      const Error& error,
                      boost::shared_ptr<std::string> payload)
    {
            getsOutStanding_--;
            {
                Synchronized s(monitor_);
                if (waiting_ && getsOutStanding_ < concurrency_) 
                {
                    monitor_.notify();
                }
            }
         
             if(error != ERR_OK) {
                    getsFailedCnt_++;
                    std::cout << "GetBlob failed! " << error << std::endl;
                    auto endTs = util::getTimeStampNanos();
                    getAvgLatency_.update(endTs - opStartTs);
                    // Request failed so the payload is null nothing to send request to SM
                    return;
             } else {
                getsSuccessCnt_++;
             }


            fpi::QueryCatalogMsgPtr qryCatRsp =
                    net::ep_deserialize<fpi::QueryCatalogMsg>(const_cast<Error&>(error), payload);
           fpi::FDS_ObjectIdType objIdType = qryCatRsp->obj_list.front().data_obj_id;
           ObjectID objId(objIdType.digest);

           auto getObjMsg = SvcMsgFactory::newGetObjectMsg(volId_, objId);
           auto asyncGetReq = gSvcRequestPool->newEPSvcRequest(sm_svcUuid);

           asyncGetReq->setPayload(FDSP_MSG_TYPEID(fpi::GetObjectMsg), getObjMsg);
           asyncGetReq->onResponseCb(std::bind(&SMApi::sm_cb, this, opStartTs,
                                               std::placeholders::_1,
                                               std::placeholders::_2, std::placeholders::_3));
           asyncGetReq->setTimeoutMs(1000);
           asyncGetReq->invoke();



    }

    void sm_cb(uint64_t opStartTs, EPSvcRequest* svcReq,
                          const Error& error,
                          boost::shared_ptr<std::string> payload)
    {
        auto endTs = util::getTimeStampNanos();
        getAvgLatency_.update(endTs - opStartTs);
        

        if(getsIssued_ == (getsSuccessCnt_ + getsFailedCnt_)) {
            getEndTs_ = util::getTimeStampNanos();
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

        for (uint32_t i = 0; i < opTs_.size(); i+=100) {
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
    std::atomic<uint32_t> putsIssued_;
    std::atomic<uint32_t> outStanding_;
    std::atomic<uint32_t> putsSuccessCnt_;
    std::atomic<uint32_t> putsFailedCnt_;
    std::atomic<uint32_t> getsIssued_;
    std::atomic<uint32_t> getsOutStanding_;
    std::atomic<uint32_t> getsSuccessCnt_;
    std::atomic<uint32_t> getsFailedCnt_;

    Monitor monitor_;
    bool waiting_;
    uint32_t concurrency_;
    util::TimeStamp startTs_;
    util::TimeStamp endTs_;
    LatencyCounter avgLatency_;
    LatencyCounter getAvgLatency_;
    std::vector<SvcRequestIf::SvcReqTs> opTs_;
    std::vector<std::string> blobList_;
    util::TimeStamp getStartTs_;
    util::TimeStamp getEndTs_;

};


TEST_F(SMApi, dmsmPerf)
{
    uint64_t nPuts =  this->getArg<uint64_t>("puts-cnt");
    bool profile = this->getArg<bool>("profile");
    bool failSendsbefore = this->getArg<bool>("failsends-before");
    bool failSendsafter = this->getArg<bool>("failsends-after");
    bool uturnAsyncReq = this->getArg<bool>("uturn-asyncreqt");
    bool uturnPuts = this->getArg<bool>("uturn");
    bool disableSchedule = this->getArg<bool>("disable-schedule");
    bool largeFrame = this->getArg<bool>("largeframe");
    std::string blobPrefix("testBlobOnce");
    concurrency_ = this->getArg<uint32_t>("concurrency");
   
   // blobList_.resize(nPuts);	
    if(concurrency_ == 0) {
    	concurrency_ = nPuts;	
    }

    std::cout << "Read all the command line args" << std::endl;

    fpi::SvcUuid sm_svcUuid;
    fpi::SvcUuid dm_svcUuid;

    sm_svcUuid = TestUtils::getAnyNonResidentSmSvcuuid(gModuleProvider->get_plf_manager());
    dm_svcUuid = TestUtils::getAnyNonResidentDmSvcuuid(gModuleProvider->get_plf_manager());

    ASSERT_NE(sm_svcUuid.svc_uuid, 0);
    ASSERT_NE(dm_svcUuid.svc_uuid, 0);

    if(profile) {
        ProfilerStart("/tmp/svc_dmsm_gtest.prof");
    }
    
    auto datagen = boost::make_shared<RandDataGenerator<>>(4096,4096);
    auto putMsgGenF = std::bind(&SvcMsgFactory::newPutObjectMsg, volId_, datagen);
    auto putMsgGen = boost::make_shared<CachedMsgGenerator<fpi::PutObjectMsg>>(nPuts,
                                                                            true, putMsgGenF);

    startTs_ = util::getTimeStampNanos();

    std::cout << "starting the for loop" << std::endl;

    for (uint64_t i = 0; i < nPuts; i++) {
        /*  setup the putBlobOnce request*/
        std::string blobName = blobPrefix + std::to_string(i);
        blobList_.push_back(blobName);
        auto putBlobOnce = SvcMsgFactory::newUpdateCatalogOnceMsg(volId_, blobName);
        auto asyncPutBlobTxReq = gSvcRequestPool->newEPSvcRequest(dm_svcUuid);
       // fds::UpdateBlobInfoNoData(putBlobOnce, MAX_OBJECT_SIZE, BLOB_SIZE);
    //    fullSmDmTestWaiter waiter;
  
        putBlobOnce->txId = i;
        putBlobOnce->dmt_version = 1;
        putBlobOnce->blob_mode = 0;

       
        /* setup the Dm request*/
        auto putObjMsg = putMsgGen->nextItem();
        uint64_t opStartTs = util::getTimeStampNanos();
	auto nAcks = std::make_shared<int>(2);
        asyncPutBlobTxReq->setPayload(FDSP_MSG_TYPEID(fpi::UpdateCatalogOnceMsg), putObjMsg);
        asyncPutBlobTxReq->onResponseCb(std::bind(&SMApi::dm_sm_put_cb, this, opStartTs,
                                                   nAcks, std::placeholders::_1,
                                                   std::placeholders::_2, std::placeholders::_3));
        std::cout << "setup of dm request done" << std::endl;

        auto asyncPutReq = gSvcRequestPool->newEPSvcRequest(sm_svcUuid);
        asyncPutReq->setPayload(FDSP_MSG_TYPEID(fpi::PutObjectMsg), putObjMsg);
        asyncPutReq->onResponseCb(std::bind(&SMApi::dm_sm_put_cb, this, opStartTs,
                                            nAcks, std::placeholders::_1,
                                            std::placeholders::_2, std::placeholders::_3));
        
        std::cout << "Setup of Sm request done" << std::endl;
        asyncPutReq->setTimeoutMs(1000);
        putsIssued_++;
        outStanding_++;

       // waiter.startTs = startTs;

        asyncPutBlobTxReq->invoke();
        asyncPutReq->invoke();

        {
            Synchronized s(monitor_);
            while(outStanding_ >= concurrency_) {
                waiting_ = true;
                std::cout << "Waiting concurrency effect!" << std::endl;
                monitor_.wait();
            }
            waiting_ = false;
        }

    }
    if(profile) {
        ProfilerStop();
    }

    /* Poll for completion */
    POLL_MS((putsIssued_ == putsSuccessCnt_ + putsFailedCnt_), 2000, (nPuts * 10));
    
    std::cout << "PutsSuccessCnt_ " << putsSuccessCnt_ << 
                std::endl << "PutsFailedCnt_" << putsFailedCnt_ << std::endl;
    std::cout << "blobList_ size is " << blobList_.size() << std::endl;
    std::cout << "difference is " << (endTs_ - startTs_) << std::endl;
    uint64_t diff = (endTs_ - startTs_);
    long double throughput = (1000000000 / diff);
    std::cout << "Throughput: " << (throughput * putsIssued_) << std::endl;
    std::cout << "Avg op Latency" << avgLatency_.value() << std::endl;
    std::cout <<  "Avg time taken: " << (static_cast<double>(endTs_ - startTs_)) / putsIssued_ << std::endl;
    ASSERT_TRUE(putsIssued_ == (putsSuccessCnt_ + putsFailedCnt_)) << "putsFailedCnt_" << putsFailedCnt_ << "putsSuccessCnt_" << putsSuccessCnt_;

    /* Start gets for the blob we have put*/

   getStartTs_ = util::getTimeStampNanos();
   for(uint64_t i = 0 ; i < nPuts; i++) {
       std::string blobName = blobPrefix + std::to_string(i);
       uint64_t blobOffset = 0;
       auto qryCat = SvcMsgFactory::newQueryCatalogMsg(volId_, blobName, blobOffset);
       auto asyncQryCatReq = gSvcRequestPool->newEPSvcRequest(dm_svcUuid);
       asyncQryCatReq->setPayload(FDSP_MSG_TYPEID(fpi::QueryCatalogMsg), qryCat);
       uint64_t opStartTs = util::getTimeStampNanos();
       asyncQryCatReq->onResponseCb(std::bind(&SMApi::dm_sm_get_cb, this,
				            sm_svcUuid,  opStartTs,
                                            std::placeholders::_1,
                                            std::placeholders::_2, std::placeholders::_3));
   
       
       getsIssued_++;
       getsOutStanding_++;
       asyncQryCatReq->invoke();
       
       {
            Synchronized s(monitor_);
            while(outStanding_ >= concurrency_) {
                waiting_ = true;
                std::cout << "Waiting concurrency effect!" << std::endl;
                monitor_.wait();
            }
            waiting_ = false;
       }
       
   }

    /* Poll for completion */
    POLL_MS((getsIssued_ == getsSuccessCnt_ + getsFailedCnt_), 2000, (nPuts * 10));
    
    std::cout << "getsSuccessCnt_ " << getsSuccessCnt_ << 
                std::endl << "getsFailedCnt_" << getsFailedCnt_ << std::endl;
    std::cout << "blobList_ size is " << blobList_.size() << std::endl;
    std::cout << "difference is " << (getEndTs_ - getStartTs_) << std::endl;
    double diffG = (getEndTs_ - getStartTs_);
    long double throughputG = (1000000000 / diffG);
    std::cout << "Throughput: " << (throughputG * getsIssued_) << std::endl;
    std::cout << "Avg op Latency" << getAvgLatency_.value() << std::endl;

}


/**
* @brief Tests dropping puts fault injection
*
*/
 #if 0
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
    concurrency_ = this->getArg<uint32_t>("concurrency");
    if (concurrency_ == 0) {
        concurrency_  = nPuts;
    }

    opTs_.resize(nPuts);

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

    /* End profiler */
    if (profile) {
        ProfilerStop();
    }

    uint64_t throughput = (1000000000 / (endTs_ - startTs_)) * putsIssued_;
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
#endif

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
        ("concurrency", po::value<uint32_t>()->default_value(0), "concurrency");
    SMApi::init(argc, argv, opts, "vol1");
    return RUN_ALL_TESTS();
}

