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
#include <google/profiler.h>

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
        // std::cout << "pubcb: " << error << std::endl;
    }
 protected:
    std::atomic<uint32_t> putsIssued_;
    std::atomic<uint32_t> putsSuccessCnt_;
    std::atomic<uint32_t> putsFailedCnt_;
};


/**
* @brief Tests basic puts and gets
*
*/
TEST_F(SMApi, put_get)
{
    int nPuts =  this->getArg<int>("puts-cnt");
    int nGets = 10;

    fpi::SvcUuid svcUuid;
    svcUuid = TestUtils::getAnyNonResidentSmSvcuuid(gModuleProvider->get_plf_manager());
    ASSERT_NE(svcUuid.svc_uuid, 0);

    /* To generate random data between 10 to 100 bytes */
    auto g = boost::make_shared<RandDataGenerator<>>(10, 100);
    ProfilerStart("/tmp/output.prof");
    /* Issue puts */
    for (int i = 0; i < nPuts; i++) {
        auto putObjMsg = SvcMsgFactory::newPutObjectMsg(volId_, g);
        auto asyncPutReq = gSvcRequestPool->newEPSvcRequest(svcUuid);
        asyncPutReq->setPayload(FDSP_MSG_TYPEID(fpi::PutObjectMsg), putObjMsg);
        asyncPutReq->onResponseCb(std::bind(&SMApi::putCb, this,
                                            std::placeholders::_1,
                                            std::placeholders::_2, std::placeholders::_3));
        putsIssued_++;
        asyncPutReq->invoke();
    }

    /* Poll for completion.  For now giving 1000ms/per put.  We should tighten that */
    POLL_MS((putsIssued_ == putsSuccessCnt_ + putsFailedCnt_), 500, nPuts * 1000);

    ProfilerStop();

    ASSERT_TRUE(putsIssued_ == putsSuccessCnt_) << "putsIssued: " << putsIssued_
        << " putsSuccessCnt: " << putsSuccessCnt_;
}

/**
* @brief Tests dropping puts fault injection
*
*/
TEST_F(SMApi, drop_puts)
{
    int nPuts =  this->getArg<int>("puts-cnt");

    fpi::SvcUuid svcUuid;
    svcUuid = TestUtils::getAnyNonResidentSmSvcuuid(gModuleProvider->get_plf_manager());
    ASSERT_NE(svcUuid.svc_uuid, 0);

    /* Set fault to drop all puts */
    ASSERT_TRUE(TestUtils::enableFault(svcUuid, "svc.drop.putobject"));

    /* To generate random data between 10 to 100 bytes */
    auto g = boost::make_shared<RandDataGenerator<>>(10, 100);

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
    POLL_MS((putsIssued_ == putsSuccessCnt_ + putsFailedCnt_), 500, nPuts * 100000);

    ASSERT_TRUE(putsIssued_ == putsFailedCnt_) << "putsIssued: " << putsIssued_
        << " putsFailedCnt: " << putsFailedCnt_;

    /* Disable fault injection */
    ASSERT_TRUE(TestUtils::disableFault(svcUuid, "svc.drop.putobject"));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message")
        ("puts-cnt", po::value<int>(), "puts count");
    SMApi::init(argc, argv, opts, "vol1");
    return RUN_ALL_TESTS();
}
