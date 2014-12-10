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

#include <dm-tvc/TimelineDB.h>

fds::DMTester* dmTester = NULL;
fds::concurrency::TaskStatus taskCount(0);

struct DmUnitTest : ::testing::Test {
    virtual void SetUp() override {
    }

    virtual void TearDown() override {
    }
};

TEST_F(DmUnitTest, Timelinedb) {
    fds::TimelineDB timeline;
    timeline.removeVolume(1);
    timeline.addJournalFile(1, 10, "hello1");
    timeline.addJournalFile(1, 20, "hello2");
    timeline.addJournalFile(1, 30, "hello3");
    timeline.addJournalFile(1, 40, "hello4");
    timeline.addJournalFile(1, 50, "hello5");

    std::vector<fds::JournalFileInfo> vecJournalFiles;
    timeline.getJournalFiles(1, 0, 30, vecJournalFiles);
    for (auto fileinfo : vecJournalFiles) {
        LOGDEBUG << "[start:" << fileinfo.startTime << " file:" << fileinfo.journalFile << "]";
    }
    EXPECT_EQ(3, vecJournalFiles.size());

    vecJournalFiles.clear();
    timeline.getJournalFiles(1, 32, 55, vecJournalFiles);
    for (auto fileinfo : vecJournalFiles) {
        LOGDEBUG << "[start:" << fileinfo.startTime << " file:" << fileinfo.journalFile << "]";
    }
    EXPECT_EQ(3, vecJournalFiles.size());

    timeline.addSnapshot(1, 2, 10);
    timeline.addSnapshot(1, 3, 20);
    timeline.addSnapshot(1, 4, 30);
    timeline.addSnapshot(1, 5, 40);

    fds_volid_t snapshotId;
    timeline.getLatestSnapshotAt(1, 22, snapshotId);
    EXPECT_EQ(3, snapshotId);
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
