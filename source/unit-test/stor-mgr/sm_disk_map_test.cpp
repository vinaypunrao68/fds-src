/**
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <unistd.h>

#include <vector>
#include <chrono>
#include <thread>
#include <string>
#include <boost/shared_ptr.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <util/Log.h>
#include <fds_assert.h>
#include <fds_process.h>
#include <ObjectId.h>
#include <FdsRandom.h>
#include <object-store/SmDiskMap.h>

#include <sm_ut_utils.h>

/**
 * Unit test for SmDiskMap class
 */

namespace fds {

class SmDiskMapUtProc : public FdsProcess {
  public:
    SmDiskMapUtProc(int argc, char * argv[], const std::string & config,
                    const std::string & basePath, Module * vec[]) {
        init(argc, argv, config, basePath, "smdiskmap_ut.log", vec);
    }

    virtual int run() override {
        return 0;
    }
};

SmDiskMap::ptr
loadDiskMap(fds_uint32_t sm_count) {
    Error err(ERR_OK);
    SmDiskMap::ptr smDiskMap(new SmDiskMap("Test SM Disk Map"));
    smDiskMap->mod_init(NULL);

    fds_uint32_t cols = (sm_count < 4) ? sm_count : 4;
    DLT* dlt = new DLT(10, cols, 1, true);
    SmUtUtils::populateDlt(dlt, sm_count);
    GLOGDEBUG << "Using DLT: " << *dlt;
    smDiskMap->handleNewDlt(dlt);

    // we don't need dlt anymore
    delete dlt;

    return smDiskMap;
}

void
printSmTokens(const SmDiskMap::const_ptr& smDiskMap) {
    SmTokenSet smToks = smDiskMap->getSmTokens();
    for (SmTokenSet::const_iterator cit = smToks.cbegin();
         cit != smToks.cend();
         ++cit) {
        GLOGDEBUG << "Token " << *cit << " disk path: "
                  << smDiskMap->getDiskPath(*cit, diskio::diskTier);
    }
}

TEST(SmDiskMap, basic) {
    Error err(ERR_OK);
    fds_uint32_t sm_count = 1;

    // start clean
    const FdsRootDir *dir = g_fdsprocess->proc_fdsroot();
    SmUtUtils::cleanFdsDev(dir);

    SmDiskMap::ptr smDiskMap = loadDiskMap(sm_count);
    printSmTokens(smDiskMap);
}

TEST(SmDiskMap, up_down) {
    Error err(ERR_OK);
    const FdsRootDir *dir = g_fdsprocess->proc_fdsroot();

    // bring up with different number of SMs
    // each time from clean state
    for (fds_uint32_t i = 1; i < 300; i+=24) {
        // start clean
        SmUtUtils::cleanFdsDev(dir);
        SmDiskMap::ptr smDiskMap = loadDiskMap(i);
    }
}

}  // namespace fds

int main(int argc, char * argv[]) {
    fds::SmDiskMapUtProc diskMapProc(argc, argv, "sm_ut.conf",
                                     "fds.diskmap_ut.", NULL);
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

