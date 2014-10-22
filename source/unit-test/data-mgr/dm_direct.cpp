/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include "./dm_mocks.h"
#include "./dm_gtest.h"
#include "./dm_utils.h"
#include <vector>
#include <string>
#include <thread>

static std::atomic<fds_uint32_t> taskCount;
fds::DMTester* dmTester = NULL;

struct DmUnitTest : ::testing::Test {
    virtual void SetUp() override {
        if (dmTester->start()) {
            generateVolumes(dmTester->volumes);
        }
    }

    virtual void TearDown() override {
        dmTester->stop();
    }

    Error addVolume(uint num) {
        const auto& volumes = dmTester->volumes;
        Error err(ERR_OK);
        std::cout << "adding volume: " << volumes[num]->volUUID
                  << ":" << volumes[num]->name
                  << ":" << dataMgr->getPrefix()
                  << std::endl;
        return dataMgr->_process_add_vol(dataMgr->getPrefix() +
                                        std::to_string(volumes[num]->volUUID),
                                        volumes[num]->volUUID, volumes[num].get(),
                                        false);
    }
};

TEST_F(DmUnitTest, AddVolume) {
    for (fds_uint32_t i = 0; i < dmTester->volumes.size(); i++) {
        EXPECT_EQ(ERR_OK, addVolume(i));
    }
}



int main(int argc, char** argv) {
    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    ::testing::InitGoogleMock(&argc, argv);

    dmTester = new fds::DMTester(argc, argv);

    // process command line options
    po::options_description desc("\nDM test Command line options");
    desc.add_options()
            ("help,h"       , "help/ usage message")  // NOLINT
            ("num-volumes,v", po::value<fds_uint32_t>(&NUM_VOLUMES)->default_value(NUM_VOLUMES)        , "number of volumes")  // NOLINT
            ("obj-size,o"   , po::value<fds_uint32_t>(&MAX_OBJECT_SIZE)->default_value(MAX_OBJECT_SIZE), "max object size in bytes")  // NOLINT
            ("blob-size,b"  , po::value<fds_uint64_t>(&BLOB_SIZE)->default_value(BLOB_SIZE)            , "blob size in bytes")  // NOLINT
            ("num-blobs,n"  , po::value<fds_uint32_t>(&NUM_BLOBS)->default_value(NUM_BLOBS)            , "number of blobs")  // NOLINT
            ("puts-only"    , "do put operations only")
            ("no-delete"    , "do put & get operations only");

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }

    return RUN_ALL_TESTS();
}
