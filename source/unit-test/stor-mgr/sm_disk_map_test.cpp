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
                    const std::string & basePath, Module * vec[]);
    virtual ~SmDiskMapUtProc() {}
    virtual int run() override;

    Error startCleanDiskMap(fds_uint32_t sm_count);
    Error shutdownDiskMap();
    Error printSmTokens();

  private:
     /* helper methods */
    void populateDlt(DLT* dlt, fds_uint32_t sm_count);

    SmDiskMap::unique_ptr smDiskMap;
};
SmDiskMapUtProc* diskMapProc = NULL;

SmDiskMapUtProc::SmDiskMapUtProc(int argc, char * argv[], const std::string & config,
                                 const std::string & basePath, Module * vec[]) {
    smDiskMap = fds::SmDiskMap::unique_ptr(
        new fds::SmDiskMap("Test SM Disk Map"));
    LOGNORMAL << "Will test Sm Disk Map";

    fds::Module *smVec[] = {
        smDiskMap.get(),
        nullptr
    };
    init(argc, argv, config, basePath, "smdiskmap_ut.log", vec);
}

int
SmDiskMapUtProc::run() {
    return 0;
}

Error
SmDiskMapUtProc::startCleanDiskMap(fds_uint32_t sm_count) {
    Error err(ERR_OK);
    const FdsRootDir *dir = g_fdsprocess->proc_fdsroot();
    SmUtUtils::cleanFdsDev(dir);

    smDiskMap->mod_startup();

    fds_uint32_t cols = (sm_count < 4) ? sm_count : 4;
    DLT* dlt = new DLT(10, cols, 1, true);
    SmUtUtils::populateDlt(dlt, sm_count);
    GLOGDEBUG << "Using DLT: " << *dlt;
    smDiskMap->handleNewDlt(dlt);

    // we don't need dlt anymore
    delete dlt;

    return err;
}

Error SmDiskMapUtProc::printSmTokens() {
    SmTokenSet smToks = smDiskMap->getSmTokens();
    for (SmTokenSet::const_iterator cit = smToks.cbegin();
         cit != smToks.cend();
         ++cit) {
        GLOGDEBUG << "Token " << *cit << " disk path: "
                  << smDiskMap->getDiskPath(*cit, diskio::diskTier);
    }
    return ERR_OK;
}

Error
SmDiskMapUtProc::shutdownDiskMap() {
    Error err(ERR_OK);
    smDiskMap->mod_shutdown();
    return err;
}

TEST(SmDiskMap, basic) {
    Error err(ERR_OK);
    fds_uint32_t sm_count = 1;

    err = diskMapProc->startCleanDiskMap(sm_count);
    EXPECT_TRUE(err.ok());

    err = diskMapProc->printSmTokens();
    EXPECT_TRUE(err.ok());

    err = diskMapProc->shutdownDiskMap();
    EXPECT_TRUE(err.ok());
}

TEST(SmDiskMap, up_down) {
    Error err(ERR_OK);

    // bring up with different number of SMs
    // each time from clean state
    for (fds_uint32_t i = 1; i < 300; i+=24) {
        err = diskMapProc->startCleanDiskMap(i);
        err = diskMapProc->shutdownDiskMap();
    }
}

}  // namespace fds

int main(int argc, char * argv[]) {
    fds::diskMapProc = new fds::SmDiskMapUtProc(argc, argv,
                                                "sm_ut.conf",
                                                "fds.diskmap_ut.", NULL);

    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

