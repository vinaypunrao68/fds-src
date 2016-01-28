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
#include <include/util/disk_utils.h>

#include <sm_ut_utils.h>

#include <persistent-layer/dm_io.h>

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
    smDiskMap->loadPersistentState();

    fds_uint32_t cols = (sm_count < 4) ? sm_count : 4;
    DLT* dlt = new DLT(10, cols, 1, true);
    SmUtUtils::populateDlt(dlt, sm_count);
    GLOGDEBUG << "Using DLT: " << *dlt;
    smDiskMap->handleNewDlt(dlt);

    // we don't need dlt anymore
    delete dlt;

    return smDiskMap;
}

boost::log::formatting_ostream& operator<< (boost::log::formatting_ostream& out,
                                            const SmTokenSet& toks) {
    out << "tokens {";
    if (toks.size() == 0) out << "none";
    for (SmTokenSet::const_iterator cit = toks.cbegin();
         cit != toks.cend();
         ++cit) {
        if (cit != toks.cbegin()) {
            out << ", ";
        }
        out << *cit;
    }
    out << "}\n";
    return out;
}

void
printSmTokens(const SmDiskMap::const_ptr& smDiskMap) {
    SmTokenSet smToks = smDiskMap->getSmTokens();
    diskio::DataTier tier = diskio::diskTier;
    // if there are no HDDs, tokens will be on SSDs only
    DiskIdSet hddIds = smDiskMap->getDiskIds(diskio::diskTier);    
    if (hddIds.size() == 0) {
        tier = diskio::flashTier;
    }
    // in other cases, print HDD location
    for (SmTokenSet::const_iterator cit = smToks.cbegin();
         cit != smToks.cend();
         ++cit) {
        GLOGDEBUG << "Token " << *cit << " disk path: "
                  << smDiskMap->getDiskPath(*cit, tier);
    }
}

TEST(SmDiskMap, getDiskConsumedSize) {
    fds_uint32_t sm_count = 1;

    // start clean
    const FdsRootDir *dir = g_fdsprocess->proc_fdsroot();
    const std::string devPath = dir->dir_dev();
    SmUtUtils::cleanAllInDir(devPath);

    // create our own disk map
    SmUtUtils::setupDiskMap(dir, 10, 2);
    SmDiskMap::ptr smDiskMap = loadDiskMap(sm_count);

    for (auto diskId : smDiskMap->getDiskIds()) {
        DiskUtils::CapacityPair cap_info = smDiskMap->getDiskConsumedSize(diskId);
        ASSERT_TRUE(cap_info.usedCapacity < cap_info.totalCapacity);
        ASSERT_TRUE(cap_info.totalCapacity > 0);
    }
}

TEST(SmDiskMap, basic) {
    Error err(ERR_OK);
    fds_uint32_t sm_count = 1;

    // start clean
    const FdsRootDir *dir = g_fdsprocess->proc_fdsroot();
    const std::string devPath = dir->dir_dev();
    SmUtUtils::cleanAllInDir(devPath);

    // create our own disk map
    SmUtUtils::setupDiskMap(dir, 10, 2);

    SmDiskMap::ptr smDiskMap = loadDiskMap(sm_count);
    printSmTokens(smDiskMap);

    // print disk ids
    DiskIdSet diskIds = smDiskMap->getDiskIds(diskio::diskTier);
    GLOGDEBUG << "HDD tier disk IDs " << diskIds;
    for (DiskIdSet::const_iterator cit = diskIds.cbegin();
         cit != diskIds.cend();
         ++cit) {
        SmTokenSet toks = smDiskMap->getSmTokens(*cit);
        GLOGDEBUG << "Disk " << *cit << " " << toks;
        // get disk path
        GLOGDEBUG << "Disk " << *cit << " path "
                  << smDiskMap->getDiskPath(*cit);
    }

    diskIds = smDiskMap->getDiskIds(diskio::flashTier);
    GLOGDEBUG << "Flash tier disk Ids " << diskIds;
    for (DiskIdSet::const_iterator cit = diskIds.cbegin();
         cit != diskIds.cend();
         ++cit) {
        SmTokenSet toks = smDiskMap->getSmTokens(*cit);
        GLOGDEBUG << "Disk " << *cit << " " << toks;
        // we should be able to get disk path for each token
        for (SmTokenSet::const_iterator ctok = toks.cbegin();
             ctok != toks.cend();
             ++ctok) {
            GLOGDEBUG << "Disk " << *cit << " token " << *ctok << " "
                      << smDiskMap->getDiskPath(*ctok, diskio::flashTier);
        }
    }

    // cleanup
    SmUtUtils::cleanAllInDir(devPath);
}

TEST(SmDiskMap, all_hdds) {
    Error err(ERR_OK);
    fds_uint32_t sm_count = 1;
    fds_uint32_t hdd_count = 10;

    // start clean
    const FdsRootDir *dir = g_fdsprocess->proc_fdsroot();
    const std::string devPath = dir->dir_dev();
    SmUtUtils::cleanAllInDir(devPath);

    // create our own disk map
    SmUtUtils::setupDiskMap(dir, hdd_count, 0);

    SmDiskMap::ptr smDiskMap = loadDiskMap(sm_count);
    printSmTokens(smDiskMap);

    // we should get empty disk set for SSDs
    DiskIdSet ssdIds = smDiskMap->getDiskIds(diskio::flashTier);
    EXPECT_EQ(0u, ssdIds.size());

    // we should get 'hdd_count' number of disk IDs for HDDs
    DiskIdSet hddIds = smDiskMap->getDiskIds(diskio::diskTier);
    EXPECT_EQ(hdd_count, hddIds.size());

    // cleanup
    SmUtUtils::cleanAllInDir(devPath);
}

TEST(SmDiskMap, all_flash) {
    Error err(ERR_OK);
    fds_uint32_t sm_count = 1;
    fds_uint32_t ssd_count = 1;

    for (ssd_count = 1; ssd_count <= 16; ssd_count += 7) {
        // start clean
        const FdsRootDir *dir = g_fdsprocess->proc_fdsroot();
        const std::string devPath = dir->dir_dev();
        SmUtUtils::cleanAllInDir(devPath);

        // create our own disk map
        SmUtUtils::setupDiskMap(dir, 0, ssd_count);

        SmDiskMap::ptr smDiskMap = loadDiskMap(sm_count);
        printSmTokens(smDiskMap);

        // we should get ssd_count number of disk IDs for SSDs
        DiskIdSet ssdIds = smDiskMap->getDiskIds(diskio::flashTier);
        EXPECT_EQ(ssd_count, ssdIds.size());

        // we should get empty disk set for HDDs
        DiskIdSet hddIds = smDiskMap->getDiskIds(diskio::diskTier);
        EXPECT_EQ(0u, hddIds.size());

        // cleanup
        SmUtUtils::cleanAllInDir(devPath);
    }
}

TEST(SmDiskMap, up_down) {
    Error err(ERR_OK);
    const FdsRootDir *dir = g_fdsprocess->proc_fdsroot();
    const std::string devPath = dir->dir_dev();
    SmUtUtils::cleanAllInDir(devPath);

    // bring up with different number of SMs
    // each time from clean state
    for (fds_uint32_t i = 1; i < 300; i+=24) {
        // create our own disk map
        SmUtUtils::setupDiskMap(dir, 10, 2);
        SmDiskMap::ptr smDiskMap = loadDiskMap(i);

        // cleanup
        SmUtUtils::cleanAllInDir(devPath);
    }
}

TEST(SmDiskMap, token_ownership) {
    Error err(ERR_OK);
    const FdsRootDir *dir = g_fdsprocess->proc_fdsroot();
    const std::string devPath = dir->dir_dev();
    SmUtUtils::cleanAllInDir(devPath);

    // create our own disk map
    SmUtUtils::setupDiskMap(dir, 10, 2);

    // only do initialization
    SmDiskMap::ptr smDiskMap(new SmDiskMap("Test SM Disk Map"));
    smDiskMap->mod_init(NULL);
    smDiskMap->loadPersistentState();

    // at this point, none of the SM tokens are valid
    SmTokenSet smToks = smDiskMap->getSmTokens();
    EXPECT_EQ(smToks.size(), 0u);

    // pretend this is new SM added to the domain
    // first comes DLT update
    NodeUuid myNodeUuid(1);
    fds_uint32_t sm_count = 8;
    fds_uint32_t cols = (sm_count < 4) ? sm_count : 4;
    // we are using very small DLT, where DLT token = SM token
    // so we can verify things easily...
    DLT* dlt = new DLT(8, cols, 1, true);
    SmUtUtils::populateDlt(dlt, sm_count);
    GLOGDEBUG << "Using DLT: " << *dlt;
    err = smDiskMap->handleNewDlt(dlt, myNodeUuid);
    EXPECT_TRUE(err == ERR_SM_NOERR_GAINED_SM_TOKENS);

    // since we made DLT tokens match SM tokens, verify it is true
    smToks.clear();
    smToks = smDiskMap->getSmTokens();

    const TokenList& dlt_toks = dlt->getTokens(1);
    GLOGDEBUG << "SM is responsible for " << dlt_toks.size()
              << " DLT tokens" << *dlt;

    SmTokenSet verifySmToks;
    for (TokenList::const_iterator cit = dlt_toks.cbegin();
         cit != dlt_toks.cend();
         ++cit) {
        verifySmToks.insert(smDiskMap->smTokenId(*cit));
    }
    EXPECT_EQ(smToks.size(), verifySmToks.size());

    // next we recieve DLT close, but nothing should change
    SmTokenSet rmToks = smDiskMap->handleDltClose(dlt);
    // since it was the first DLT, we should not 'lose' any SM tokens
    EXPECT_EQ(rmToks.size(), 0u);
    smToks.clear();
    smToks = smDiskMap->getSmTokens();
    EXPECT_EQ(smToks.size(), verifySmToks.size());

    // we are done with this dlt
    delete dlt;
    dlt = nullptr;

    // pretend DLT changes -- 2 SMs go away
    // sine we are using round-robing algorithm in this test
    // SM should gain and should lose ownership of some DLT tokens
    sm_count -= 2;
    dlt = new DLT(8, cols, 2, true);
    SmUtUtils::populateDlt(dlt, sm_count);
    GLOGDEBUG << "Using DLT: " << *dlt;
    err = smDiskMap->handleNewDlt(dlt, myNodeUuid);

    const TokenList& dlt2_toks = dlt->getTokens(1);
    GLOGDEBUG << "SM is responsible for " << dlt2_toks.size()
              << " DLT tokens" << *dlt;
    SmTokenSet verifySmToks2;
    for (TokenList::const_iterator cit = dlt2_toks.cbegin();
         cit != dlt2_toks.cend();
         ++cit) {
        verifySmToks2.insert(smDiskMap->smTokenId(*cit));
    }

    std::set<fds_uint32_t> gainedToks;
    std::set<fds_uint32_t> lostToks;
    std::set<fds_uint32_t>::iterator it;
    std::set_difference(verifySmToks2.begin(),
                        verifySmToks2.end(),
                        verifySmToks.begin(),
                        verifySmToks.end(), std::inserter(gainedToks, gainedToks.end()));
    std::set_difference(verifySmToks.begin(),
                        verifySmToks.end(),
                        verifySmToks2.begin(),
                        verifySmToks2.end(), std::inserter(lostToks, lostToks.end()));

    if (gainedToks.size() > 0) {
        EXPECT_TRUE(err == ERR_SM_NOERR_GAINED_SM_TOKENS);
    } else {
        EXPECT_TRUE(err.ok());
    }

    // see if SM tokens match what we expect
    smToks.clear();
    smToks = smDiskMap->getSmTokens();
    // disk map did not update lost tokens yet
    EXPECT_EQ(smToks.size(), verifySmToks.size() + gainedToks.size());

    // next we recieve DLT close, see if we lost ownership
    rmToks = smDiskMap->handleDltClose(dlt);
    EXPECT_EQ(rmToks.size(), lostToks.size());
    smToks.clear();
    smToks = smDiskMap->getSmTokens();
    // now disk map should match DLT
    EXPECT_EQ(smToks.size(), verifySmToks2.size());

    // cleanup
    SmUtUtils::cleanAllInDir(devPath);
}

TEST(SmDiskMap, restart) {
    Error err(ERR_OK);
    const FdsRootDir *dir = g_fdsprocess->proc_fdsroot();
    const std::string devPath = dir->dir_dev();
    SmUtUtils::cleanAllInDir(devPath);

    // create our own disk map
    SmUtUtils::setupDiskMap(dir, 10, 2);

    // only do initialization
    SmDiskMap::ptr smDiskMap(new SmDiskMap("Test SM Disk Map"));
    smDiskMap->mod_init(NULL);
    err = smDiskMap->loadPersistentState();
    EXPECT_TRUE(err == ERR_SM_NOERR_PRISTINE_STATE);

    // pretend this is new SM added to the domain
    // first comes DLT update
    NodeUuid myNodeUuid(1);
    fds_uint32_t sm_count = 16;
    // we are using very small DLT, where DLT token = SM token
    // so we can verify things easily...
    DLT* dlt = new DLT(8, 4, 1, true);
    SmUtUtils::populateDlt(dlt, sm_count);
    GLOGDEBUG << "Using DLT: " << *dlt;
    err = smDiskMap->handleNewDlt(dlt, myNodeUuid);
    EXPECT_TRUE(err == ERR_SM_NOERR_GAINED_SM_TOKENS);

    // next we receive DLT close, should not 'lose' any SM tokens
    SmTokenSet rmToks = smDiskMap->handleDltClose(dlt);
    EXPECT_EQ(rmToks.size(), 0u);

    // restart
    smDiskMap->mod_shutdown();
    smDiskMap.reset(new SmDiskMap("Test SM Disk Map"));
    smDiskMap->mod_init(NULL);
    err = smDiskMap->loadPersistentState();
    EXPECT_TRUE(err.ok());

    // SM will receive DLT that was there before restart
    err = smDiskMap->handleNewDlt(dlt, myNodeUuid);
    EXPECT_TRUE(err == ERR_SM_NOERR_NEED_RESYNC);

    // second DLT notification about the same DLT should
    // return an error
    err = smDiskMap->handleNewDlt(dlt, myNodeUuid);
    EXPECT_TRUE(err == ERR_DUPLICATE);

    // cleanup
    delete dlt;
    dlt = nullptr;

    // create the same DLT but with next version
    // we should be able to restart as long as SM does not
    // gain any new tokens
    dlt = new DLT(8, 4, 2, true);
    SmUtUtils::populateDlt(dlt, sm_count);
    GLOGDEBUG << "Restart with next DLT version: Using DLT: " << *dlt;
    smDiskMap->mod_shutdown();
    smDiskMap.reset(new SmDiskMap("Test SM Disk Map"));
    smDiskMap->mod_init(NULL);
    err = smDiskMap->loadPersistentState();
    EXPECT_TRUE(err.ok());

    // restart should work since SM did not gain any tokens
    err = smDiskMap->handleNewDlt(dlt, myNodeUuid);
    EXPECT_TRUE(err == ERR_SM_NOERR_NEED_RESYNC);

    // cleanup
    delete dlt;
    dlt = nullptr;

    // restart again with next DLT version
    // make SM lose a DLT token from one DLT column
    dlt = new DLT(8, 4, 2, true);
    SmUtUtils::populateDlt(dlt, sm_count);
    fds_uint64_t numTokens = pow(2, dlt->getWidth());
    DltTokenGroup tg(dlt->getDepth());
    fds_bool_t done = false;
    for (fds_token_id i = 0; i < numTokens; i++) {
        for (fds_uint32_t j = 0; j < dlt->getDepth(); j++) {
            NodeUuid uuid = dlt->getNode(i, j);
            if (uuid == myNodeUuid) {
                // replace with some other uuid
                NodeUuid newUuid(sm_count + 1);
                dlt->setNode(i, j, newUuid);
                done = true;
                break;
            }
        }
        if (done) {
            break;
        }
    }
    EXPECT_TRUE(done);
    dlt->generateNodeTokenMap();

    GLOGDEBUG << "Restart with next DLT version: Using DLT: " << *dlt;
    smDiskMap->mod_shutdown();
    smDiskMap.reset(new SmDiskMap("Test SM Disk Map"));
    smDiskMap->mod_init(NULL);
    err = smDiskMap->loadPersistentState();
    EXPECT_TRUE(err.ok());

    // restart should work since SM did not gain any tokens
    err = smDiskMap->handleNewDlt(dlt, myNodeUuid);
    EXPECT_TRUE(err == ERR_SM_NOERR_LOST_SM_TOKENS);

    // cleanup
    delete dlt;
    SmUtUtils::cleanAllInDir(devPath);
}

}  // namespace fds

int main(int argc, char * argv[]) {
    fds::SmDiskMapUtProc diskMapProc(argc, argv, "platform.conf",
                                     "fds.sm.", NULL);
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

