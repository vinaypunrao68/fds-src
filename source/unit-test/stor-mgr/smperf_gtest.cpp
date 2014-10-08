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
#include <testlib/DataGen.hpp>
#include <testlib/SvcMsgFactory.h>
#include <testlib/TestUtils.h>
#include <testlib/TestFixtures.h>
#include <apis/ConfigurationService.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>


using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;  // NOLINT


struct SMApi : SingleNodeTest
{
    SMApi() {
        putsIssued_ = 0;
        putsSuccessCnt_ = 0;
        putsFailedCnt_ = 0;
    }
    void putCb(EPSvcRequest* svcReq,
               const Error& error,
               boost::shared_ptr<std::string> payload)
    {
        if (error != ERR_OK) {
            putsFailedCnt_++;
        } else {
            putsSuccessCnt_++;
        }
        if (putsIssued_ == (putsSuccessCnt_ + putsFailedCnt_)) {
           endTs_ = util::getTimeStampNanos();
        }
    }
 protected:
    std::atomic<uint32_t> putsIssued_;
    std::atomic<uint32_t> putsSuccessCnt_;
    std::atomic<uint32_t> putsFailedCnt_;
    util::TimeStamp startTs_;
    util::TimeStamp endTs_;
};

/**
* @brief Tests dropping puts fault injection
*
*/
TEST_F(SMApi, putsPerf)
{
    int dataCacheSz = 100;
    int nPuts =  this->getArg<int>("puts-cnt");
    bool uturnPuts = this->getArg<bool>("uturn");

    fpi::SvcUuid svcUuid;
    svcUuid = TestUtils::getAnyNonResidentSmSvcuuid(gModuleProvider->get_plf_manager());
    ASSERT_NE(svcUuid.svc_uuid, 0);

    /* Set fault to uturn all puts */
    if (uturnPuts) {
        ASSERT_TRUE(TestUtils::enableFault(svcUuid, "svc.uturn.putobject"));
    }

    /* To generate random data between 10 to 100 bytes */
    auto g = boost::make_shared<CachedRandDataGenerator<>>(dataCacheSz, true, 4096, 4096);

    /* Start timer */
    startTs_ = util::getTimeStampNanos();
    /* Issue puts */
    for (int i = 0; i < nPuts; i++) {
        auto putObjMsg = SvcMsgFactory::newPutObjectMsg(volId_, g);
        auto asyncPutReq = gSvcRequestPool->newEPSvcRequest(svcUuid);
        asyncPutReq->setPayload(FDSP_MSG_TYPEID(fpi::PutObjectMsg), putObjMsg);
        asyncPutReq->onResponseCb(std::bind(&SMApi::putCb, this,
                                            std::placeholders::_1,
                                            std::placeholders::_2, std::placeholders::_3));
        asyncPutReq->setTimeoutMs(1000);
        putsIssued_++;
        asyncPutReq->invoke();
    }

    /* Poll for completion */
    POLL_MS((putsIssued_ == putsSuccessCnt_ + putsFailedCnt_), 500, (nPuts+2) * 1000);

    /* Disable fault injection */
    if (uturnPuts) {
        ASSERT_TRUE(TestUtils::enableFault(svcUuid, "svc.uturn.putobject"));
    }

    ASSERT_TRUE(putsIssued_ == putsSuccessCnt_) << "putsIssued: " << putsIssued_
        << " putsSuccessCnt_: " << putsFailedCnt_;
    std::cout << "Time taken: " << endTs_ - startTs_ << "(ns)\n";
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message")
        ("puts-cnt", po::value<int>(), "puts count")
        ("uturn", po::value<bool>()->default_value(false), "uturn");
    SMApi::init(argc, argv, opts, "vol1");
    return RUN_ALL_TESTS();
}
