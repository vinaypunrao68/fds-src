/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include "./dm_mocks.h"
#include "./dm_gtest.h"
#include "./dm_utils.h"

#include <testlib/SvcMsgFactory.h>
#include <vector>
#include <string>
#include <thread>
#include <google/profiler.h>

#include <timeline/timelinedb.h>

fds::DMTester* dmTester = NULL;
fds::concurrency::TaskStatus taskCount(0);

struct DmUnitTest : ::testing::Test {
    virtual void SetUp() override {
    }

    virtual void TearDown() override {
    }
};

TEST_F(DmUnitTest, Timelinedb) {
    if (!dataMgr->features.isTimelineEnabled()) {
        return;
    }

    fds_volid_t volId = invalid_vol_id;
    dataMgr->timelineMgr->getDB()->removeVolume(volId);
    EXPECT_EQ(ERR_OK, dataMgr->timelineMgr->getDB()->addJournalFile(volId, 10, "hello1"));
    EXPECT_EQ(ERR_OK, dataMgr->timelineMgr->getDB()->addJournalFile(volId, 20, "hello2"));
    EXPECT_EQ(ERR_OK, dataMgr->timelineMgr->getDB()->addJournalFile(volId, 30, "hello3"));
    EXPECT_EQ(ERR_OK, dataMgr->timelineMgr->getDB()->addJournalFile(volId, 40, "hello4"));
    EXPECT_EQ(ERR_OK, dataMgr->timelineMgr->getDB()->addJournalFile(volId, 50, "hello5"));

    std::vector<fds::timeline::JournalFileInfo> vecJournalFiles;
    EXPECT_EQ(ERR_OK, dataMgr->timelineMgr->getDB()->getJournalFiles(volId, 0, 30, vecJournalFiles));
    EXPECT_EQ(3, vecJournalFiles.size());

    vecJournalFiles.clear();
    EXPECT_EQ(ERR_OK, dataMgr->timelineMgr->getDB()->getJournalFiles(volId, 32, 55, vecJournalFiles));
    EXPECT_EQ(3, vecJournalFiles.size());

    EXPECT_EQ(ERR_OK, dataMgr->timelineMgr->getDB()->addSnapshot(volId, fds_volid_t(2), 10));
    EXPECT_EQ(ERR_OK, dataMgr->timelineMgr->getDB()->addSnapshot(volId, fds_volid_t(3), 20));
    EXPECT_EQ(ERR_OK, dataMgr->timelineMgr->getDB()->addSnapshot(volId, fds_volid_t(4), 30));
    EXPECT_EQ(ERR_OK, dataMgr->timelineMgr->getDB()->addSnapshot(volId, fds_volid_t(5), 40));

    fds_volid_t snapshotId;
    EXPECT_EQ(ERR_OK, dataMgr->timelineMgr->getDB()->getLatestSnapshotAt(volId, 22, snapshotId));
    EXPECT_EQ(fds_volid_t(3), snapshotId);
}

int main(int argc, char** argv) {
    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    ::testing::InitGoogleMock(&argc, argv);

    // process command line options
    po::options_description desc("\nDM test Command line options");
    desc.add_options()
            ("help,h"       , "help/ usage message")  // NOLINT
            ("num-volumes,v", po::value<fds_uint32_t>(&NUM_VOLUMES)->default_value(NUM_VOLUMES)        , "number of volumes")  // NOLINT
            ("obj-size,o"   , po::value<fds_uint32_t>(&MAX_OBJECT_SIZE)->default_value(MAX_OBJECT_SIZE), "max object size in bytes")  // NOLINT
            ("blob-size,b"  , po::value<fds_uint64_t>(&BLOB_SIZE)->default_value(BLOB_SIZE)            , "blob size in bytes")  // NOLINT
            ("num-blobs,n"  , po::value<fds_uint32_t>(&NUM_BLOBS)->default_value(NUM_BLOBS)            , "number of blobs")  // NOLINT
            ("profile,p"    , po::value<bool>(&profile)->default_value(profile)                        , "enable profile ")  // NOLINT
            ("puts-only"    , "do put operations only")
            ("no-delete"    , "do put & get operations only");
            ("num-blobs,n"  , po::value<fds_uint32_t>(&NUM_BLOBS)->default_value(NUM_BLOBS)            , "number of blobs");  // NOLINT

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }

    dmTester = new fds::DMTester(argc, argv);
    dmTester->start();

    int retCode = RUN_ALL_TESTS();
    dmTester->stop();
    return retCode;
}
