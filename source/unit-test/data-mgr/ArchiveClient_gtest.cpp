/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#define GTEST_USE_OWN_TR1_TUPLE 0

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <list>
#include <fstream>
#include <iostream>
#include <boost/make_shared.hpp>
#include <boost/filesystem.hpp>
#include <testlib/TestFixtures.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <fds_volume.h>
#include <DataMgrIf.h>
#include <archive/ArchiveClient.h>

using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;  // NOLINT


struct ArchiveClientTest : public ::testing::Test, DataMgrIf
{
    ArchiveClientTest()
        : volDesc_("dummy", 0)
    {
        volDesc_.tennantId = 1;
    }
    bool prepareSnap(const fds_volid_t &volId,
                     const int64_t &snapId) {
        /* Get home dir */
        std::string homeDir(getenv("HOME"));
        if (homeDir.empty()) {
            std::cerr << "Please set HOME env. variable";
            return false;
        }
        /* Set snapshot base directory */
        snapDirBase_ = homeDir + "/snapdir";
        boost::filesystem::remove_all(boost::filesystem::path(snapDirBase_));

        /* remove any existing snapdir */
        /* Prepare a snapshot under snap base directory */
        boost::filesystem::path snapdir(getSnapDirName(volId, snapId));
        if (!boost::filesystem::create_directories(snapdir)) {
            std::cerr << "Failed to create snapdir at: " << snapdir;
            return false;
        }
        /* Create files under snap directory */
        boost::filesystem::path file1(snapdir / "file1");
        std::ofstream out(file1.string());
        out << "hello";
        out.close();
        return true;
    }
    virtual std::string getSysVolumeName(const fds_volid_t &volId) const override
    {
        return "sysvol1";
    }
    virtual std::string getSnapDirBase() const override
    {
        return snapDirBase_;
    }
    virtual std::string getSnapDirName(const fds_volid_t &volId,
                                       const int64_t snapId) const override
    {
        std::stringstream stream;
        stream << snapDirBase_ << "/" << volId << "/" << snapId;
        return stream.str();
    }
 protected:
    VolumeDesc volDesc_;
    std::string snapDirBase_;
};


/**
* @brief Tests basic puts and gets
*
*/
TEST_F(ArchiveClientTest, put_get)
{
    fds_volid_t volId = 1;
    int64_t snapId = 1;

    ASSERT_TRUE(prepareSnap(volId, snapId));

    /* Create archive client */
    fds_threadpoolPtr threadpool = boost::make_shared<fds_threadpool>();
    ArchiveClientPtr archiveCl = boost::make_shared<ArchiveClient>(this, threadpool);
    archiveCl->connect("localhost:8000", "admin", "secret-key");

    /* put the file */
    EXPECT_EQ(archiveCl->putSnapSync(volId, snapId), ERR_OK);
    /* get the file */
    EXPECT_EQ(archiveCl->getSnapSync(volId, snapId), ERR_OK);
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
