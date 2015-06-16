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
#include <testlib/DataGen.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <testlib/TestFixtures.h>
#include <fds_volume.h>
#include <DataMgrIf.h>
#include <archive/ArchiveClient.h>

using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;  // NOLINT


struct ArchiveClientTest : public BaseTestFixture, DataMgrIf
{
    ArchiveClientTest()
        : volDesc_("dummy", invalid_vol_id)
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
        RandDataGenerator<> gen(-1, 1024, 1024);
        for (int i = 0; i < 1; i++) {
            out << *(gen.nextItem());
        }
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
    fds_volid_t volId(1);
    int64_t snapId = 1;
    std::string script = this->getArg<std::string>("script");

    ASSERT_TRUE(prepareSnap(volId, snapId));

    /* Create archive client */
    ArchiveClientPtr archiveCl = boost::make_shared<BotoArchiveClient>("127.0.0.1", 8443,
            "https://localhost:7443", "admin", "admin",
            script, this);
    archiveCl->connect();

    /* put the file */
    auto ret = archiveCl->putSnap(volId, snapId);
    EXPECT_EQ(ret , ERR_OK);

    /* get the file */
    ret = archiveCl->getSnap(volId, snapId);
    EXPECT_EQ(ret, ERR_OK);
    /* Optional: make sure the contents match */
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message")
        ("script", po::value<std::string>(), "archive script");
    fds::GetLog()->setSeverityFilter(fds_log::trace);
    ArchiveClientTest::init(argc, argv, opts);
    return RUN_ALL_TESTS();
}
