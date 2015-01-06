/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#define GTEST_USE_OWN_TR1_TUPLE 0

#include <unistd.h>
#include <string>
#include <list>
#include <iostream>
#include <boost/make_shared.hpp>
#include <testlib/TestFixtures.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <archive/ArchiveClient.h>

using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;  // NOLINT


struct ArchiveClientTest : public ::testing::Test
{
    ArchiveClientTest() {
    }
    void prepareSnap(const fds_volid_t &volId,
                     const int64_t &snapId) {
        // TODO(Rao): Implement this
    }
 protected:
};


/**
* @brief Tests basic puts and gets
*
*/
TEST_F(ArchiveClientTest, put_get)
{
    fds_volid_t volId = 1;
    int64_t snapId = 1;
    prepareSnap(volId, snapId);

    /* Create archive client */
    fds_threadpoolPtr threadpool = boost::make_shared<fds_threadpool>();
    ArchiveClientPtr archiveCl = boost::make_shared<ArchiveClient>(
        "/home/nbayyana/tmp/snaps", threadpool);
    archiveCl->connect("localhost:8000", "admin", "secret-key");

    /* put the file */
    archiveCl->putSnapSync(volId, snapId);
    /* get the file */
    archiveCl->getSnapSync(volId, snapId);
    /* Optional: make sure the contents match */
#if 0
    int nPuts =  this->getArg<int>("puts-cnt");

    fpi::SvcUuid svcUuid;
    std::list<ObjectID> objIds;
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

        objIds.push_back(ObjectID(putObjMsg->data_obj_id.digest));
    }

    /* Poll for completion.  For now giving 1000ms/per put.  We should tighten that */
    POLL_MS((putsIssued_ == putsSuccessCnt_ + putsFailedCnt_), 500, nPuts * 1000);

    /* Issue gets */
    for (int i = 0; i < nPuts; i++) {
        auto getObjMsg = SvcMsgFactory::newGetObjectMsg(volId_, objIds.front());
        auto asyncGetReq = gSvcRequestPool->newEPSvcRequest(svcUuid);
        asyncGetReq->setPayload(FDSP_MSG_TYPEID(fpi::GetObjectMsg), getObjMsg);
        asyncGetReq->onResponseCb(std::bind(&SMApi::getCb, this,
                                            std::placeholders::_1,
                                            std::placeholders::_2, std::placeholders::_3));
        getsIssued_++;
        asyncGetReq->invoke();

        objIds.pop_front();
    }

    /* Poll for completion.  For now giving 1000ms/per get.  We should tighten that */
    POLL_MS((getsIssued_ == getsSuccessCnt_ + getsFailedCnt_), 500, nPuts * 1000);

    ProfilerStop();

    ASSERT_TRUE(getsIssued_ == getsSuccessCnt_) << "getsIssued: " << getsIssued_
        << " getsSuccessCnt: " << getsSuccessCnt_;
#endif
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message")
        ("puts-cnt", po::value<int>(), "puts count");
    fds::GetLog()->setSeverityFilter(fds_log::trace);
    return RUN_ALL_TESTS();
}
