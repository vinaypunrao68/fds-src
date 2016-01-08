/**
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <unistd.h>
#include <set>
#include <string>

#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <fds_process.h>
#include <object-store/ObjectPersistData.h>

#include <sm_ut_utils.h>
#include <sm_dataset.h>

using ::testing::AtLeast;
using ::testing::Return;

static const fds_uint32_t bitsPerDltToken = 8;

namespace fds {

// this seem to be required to properly bring up globals like
// fds root, etc
class ObjPersistDataUtProc : public FdsProcess {
  public:
    ObjPersistDataUtProc(int argc, char * argv[], const std::string & config,
                         const std::string & basePath, Module * vec[]) {
        init(argc, argv, config, basePath, "smpersistdata_ut.log", vec);
    }

    virtual int run() override {
        return 0;
    }
};

class SmObjectPersistDataTest : public ::testing::Test {
  public:
    SmObjectPersistDataTest()
            : persistData(new ObjectPersistData("UT Object Persist Data", NULL)),
              smDiskMap(new SmDiskMap("Test SM Disk Map")) {}

    virtual void SetUp() override;
    virtual void TearDown() override;

    void init();

    /**
     * This will destruct and construct persistData again
     * without cleaning dev dirs. This simulated persistData startup
     * after shutdown
     */
    void restart();
    /**
     * Destructs and constructs persistData and disk map, and in betwee
     * cleans /<fds-root>/dev/<disk>/xxx including superblock
     */
    void cleanRestart();

    void readDataset(
        fds_uint32_t objSize,
        TestDataset& testdata,
        std::unordered_map<ObjectID, obj_phy_loc_t*, ObjectHash>& locMap);
    void writeDataset(
        TestDataset& testdata,
        std::unordered_map<ObjectID, obj_phy_loc_t*, ObjectHash>* retLocMap);
    void getTokenStats(fds_uint64_t* totSize,
                       fds_uint64_t* delSize);

    ObjectPersistData::unique_ptr persistData;
    SmDiskMap::ptr smDiskMap;
    /**
     * this is to remember whether we used fake disk map (created by us)
     * or actual disk map setup by the platform
     */
    fds_bool_t crtFakeDiskMap;
};

void
SmObjectPersistDataTest::init() {
    // init SM disk map
    smDiskMap->mod_init(NULL);
    smDiskMap->loadPersistentState();
    DLT* dlt = new DLT(bitsPerDltToken, 1, 1, true);
    SmUtUtils::populateDlt(dlt, 1);
    GLOGDEBUG << "Using DLT: " << *dlt;
    smDiskMap->handleNewDlt(dlt);
}

void
SmObjectPersistDataTest::SetUp() {
    crtFakeDiskMap = SmUtUtils::setupDiskMap(g_fdsprocess->proc_fdsroot(), 12, 2);
    init();
}

void
SmObjectPersistDataTest::TearDown() {
    // cleanup
    if (crtFakeDiskMap) {
        // not only token files/superblock but also disk-map if
        // created out own fake disk map
        SmUtUtils::cleanFdsTestDev(g_fdsprocess->proc_fdsroot());
    }
}

void
SmObjectPersistDataTest::restart() {
    // first destruct persist data store and sm disk map
    persistData.reset(new ObjectPersistData("UT Object Persist Data", NULL));
    smDiskMap.reset(new SmDiskMap("Test SM Disk Map"));

    // init them again
    init();
}

void
SmObjectPersistDataTest::cleanRestart() {
    // first destruct persist data store and sm disk map
    persistData.reset(new ObjectPersistData("UT Object Persist Data", NULL));
    smDiskMap.reset(new SmDiskMap("Test SM Disk Map"));

    // remove everything from /<fds-root>/dev/<disk>/* including superblock
    SmUtUtils::cleanFdsDev(g_fdsprocess->proc_fdsroot());

    // init them again
    init();
}

diskio::DiskRequest* createPutRequest(ObjectID objId,
                                      ObjectBuf* objBuf,
                                      diskio::DataTier tier) {
    // Construct persistent layer request
    meta_vol_io_t    vio;
    meta_obj_id_t    oid;
    fds_bool_t       sync = true;
    memcpy(oid.metaDigest, objId.GetId(), objId.GetLen());
    diskio::DiskRequest *plReq =
            new diskio::DiskRequest(vio, oid,
                                    const_cast<ObjectBuf *>(objBuf),
                                    sync, tier);
    return plReq;
}

diskio::DiskRequest* createGetRequest(ObjectID objId,
                                      ObjectBuf* objBuf,
                                      fds_uint32_t objSize,
                                      obj_phy_loc_t* loc,
                                      diskio::DataTier tier) {
    // Construct persistent layer request
    meta_vol_io_t    vio;
    meta_obj_id_t    oid;
    fds_bool_t       sync = true;
    memcpy(oid.metaDigest, objId.GetId(), objId.GetLen());
    diskio::DiskRequest *plReq =
            new diskio::DiskRequest(vio, oid,
                                    const_cast<ObjectBuf *>(objBuf),
                                    sync);
    plReq->setTier(tier);
    plReq->set_phy_loc(loc);
    (objBuf->data)->resize(objSize, 0);
    return plReq;
}

void
SmObjectPersistDataTest::writeDataset(
    TestDataset& testdata,
    std::unordered_map<ObjectID, obj_phy_loc_t*, ObjectHash>* retLocMap) {
    Error err(ERR_OK);
    // we will create a map of physical locations, so that we can read
    // these objects back (since we are not using metadata store)
    std::unordered_map<ObjectID, obj_phy_loc_t*, ObjectHash> locMap;
    for (fds_uint32_t i = 0; i < testdata.dataset_.size(); ++i) {
        ObjectID oid = testdata.dataset_[i];
        boost::shared_ptr<std::string> data =
                testdata.dataset_map_[oid].getObjectData();
        ObjectBuf objBuf(data);
        diskio::DiskRequest* req = createPutRequest(oid, &objBuf, diskio::diskTier);
        err = persistData->writeObjectData(oid, req);
        EXPECT_TRUE(err.ok());

        obj_phy_loc_t* loc = req->req_get_phy_loc();
        obj_phy_loc_t* savedLoc = new obj_phy_loc_t();
        memcpy(savedLoc, loc, sizeof(obj_phy_loc_t));
        EXPECT_EQ(locMap.count(oid), 0u);
        locMap[oid] = savedLoc;

        delete req;
    }

    retLocMap->swap(locMap);
}

void
SmObjectPersistDataTest::readDataset(
    fds_uint32_t objSize,
    TestDataset& testdata,
    std::unordered_map<ObjectID, obj_phy_loc_t*, ObjectHash>& locMap) {
    Error err(ERR_OK);
    for (fds_uint32_t i = 0; i < testdata.dataset_.size(); ++i) {
        ObjectID oid = testdata.dataset_[i];
        ObjectBuf objBuf;
        diskio::DiskRequest* req = createGetRequest(oid, &objBuf, objSize,
                                                    locMap[oid], diskio::diskTier);
        err = persistData->readObjectData(oid, req);
        EXPECT_TRUE(err.ok());

        boost::shared_ptr<const std::string> objData(objBuf.data);
        EXPECT_TRUE(testdata.dataset_map_[oid].isValid(objData));
        delete req;
    }
}

void
SmObjectPersistDataTest::getTokenStats(fds_uint64_t* totSize,
                                       fds_uint64_t* delSize)
{
    fds_uint64_t totalSize = 0;
    fds_uint64_t reclaimSize = 0;
    SmTokenSet smToks = smDiskMap->getSmTokens();
    for (SmTokenSet::const_iterator cit = smToks.cbegin();
         cit != smToks.cend();
         ++cit) {
        diskio::TokenStat stat;
        diskio::DataTier tier = diskio::diskTier;
        DiskId diskId = smDiskMap->getDiskId(*cit, tier);
        persistData->getSmTokenStats(diskId, *cit, tier, &stat);
        GLOGDEBUG << "SM token " << *cit << " total bytes "
                  << stat.tkn_tot_size << ", reclaimable bytes "
                  << stat.tkn_reclaim_size;
        totalSize += stat.tkn_tot_size;
        reclaimSize += stat.tkn_reclaim_size;
    }

    // return
    *totSize = totalSize;
    *delSize = reclaimSize;
}


TEST_F(SmObjectPersistDataTest, restart) {
    Error err(ERR_OK);

    // open data store
    err = persistData->openObjectDataFiles(smDiskMap, true);
    EXPECT_TRUE(err.ok());

    // restart without cleaning
    restart();

    // open data store again
    err = persistData->openObjectDataFiles(smDiskMap, false);
    EXPECT_TRUE(err.ok());

    // restart from clean state
    cleanRestart();
    err = persistData->openObjectDataFiles(smDiskMap, true);
    EXPECT_TRUE(err.ok());
}

TEST_F(SmObjectPersistDataTest, write_read) {
    Error err(ERR_OK);
    fds_uint32_t dsize = 500;
    fds_uint32_t objSize = 4096;
    TestDataset testdata;
    testdata.generateDataset(dsize, objSize);

    // open data store
    err = persistData->openObjectDataFiles(smDiskMap, true);
    EXPECT_TRUE(err.ok());

    // do few puts
    // we will create a map of physical locations, so that we can read
    // these objects back (since we are not using metadata store)
    std::unordered_map<ObjectID, obj_phy_loc_t*, ObjectHash> locMap;
    writeDataset(testdata, &locMap);

    // read dataset back and validate
    readDataset(objSize, testdata, locMap);

    // restart, and then read and validate again
    restart();
    err = persistData->openObjectDataFiles(smDiskMap, false);
    EXPECT_TRUE(err.ok());
    readDataset(objSize, testdata, locMap);
}

TEST_F(SmObjectPersistDataTest, sm_token_ownership) {
    Error err(ERR_OK);
    fds_uint32_t dsize = 70;
    fds_uint32_t objSize = 4096;
    const FdsRootDir* rootDir = g_fdsprocess->proc_fdsroot();
    TestDataset testdata;
    testdata.generateDataset(dsize, objSize);

    // open token files -- SM owns all SM tokens
    err = persistData->openObjectDataFiles(smDiskMap, true);
    EXPECT_TRUE(err.ok());

    // all token files should exist
    fds_bool_t exists = false;
    for (fds_token_id tok = 0; tok < SMTOKEN_COUNT; ++tok) {
        exists = SmUtUtils::existsInSubdirs(rootDir->dir_dev(),
                                            std::string("tokenFile_") + std::to_string(tok)
                                            + std::string("_"),
                                            false);
        EXPECT_TRUE(exists);
    }

    // pretend we lost ownership of half of SM tokens
    SmTokenSet tokSet;
    for (fds_token_id tok = 0; tok < SMTOKEN_COUNT; tok += 2) {
        tokSet.insert(tok);
    }
    err = persistData->closeAndDeleteObjectDataFiles(tokSet);
    EXPECT_TRUE(err.ok());

    // verify that corresponding token files are removed
    for (SmTokenSet::const_iterator cit = tokSet.cbegin();
         cit != tokSet.cend();
         ++cit) {
        exists = SmUtUtils::existsInSubdirs(rootDir->dir_dev(),
                                            std::string("tokenFile_") + std::to_string(*cit)
                                            + std::string("_"),
                                            false);
        EXPECT_FALSE(exists);
    }

    // Puts for SM tokens owned by SM should succeed and PUTs for
    // SM tokens not owned by this SM should fail with ERR_NOT_FOUND
    for (fds_uint32_t i = 0; i < testdata.dataset_.size(); ++i) {
        ObjectID oid = testdata.dataset_[i];
        boost::shared_ptr<std::string> data =
                testdata.dataset_map_[oid].getObjectData();
        ObjectBuf objBuf(data);
        diskio::DiskRequest* req = createPutRequest(oid, &objBuf, diskio::diskTier);
        err = persistData->writeObjectData(oid, req);

        fds_token_id smTokId = SmDiskMap::smTokenId(oid, bitsPerDltToken);
        LOGDEBUG << "writeObjectData for " << oid << " smToken " << smTokId
                 << " returned " << err;
        if (tokSet.count(smTokId) == 0) {
            // owned by SM
            EXPECT_TRUE(err.ok());
        } else {
            // not owned by SM
            EXPECT_TRUE(err == ERR_NOT_FOUND);
        }

        delete req;
    }

    // since we did not change smDiskMap, the following call
    // will create and open token files that were removed from the
    // close and delete call
    // we are pretending that DLT changed such that SM gained ownership
    // of all tokens again
    err = persistData->openObjectDataFiles(smDiskMap, false);
    EXPECT_TRUE(err.ok());
}

TEST_F(SmObjectPersistDataTest, write_delete) {
    Error err(ERR_OK);
    fds_uint32_t dsize = 1000;
    fds_uint32_t objSize = 4096;
    TestDataset testdata;
    testdata.generateDataset(dsize, objSize);

    // open data store
    err = persistData->openObjectDataFiles(smDiskMap, true);
    EXPECT_TRUE(err.ok());

    // do few puts
    // we will create a map of physical locations, so that we can read
    // these objects back (since we are not using metadata store)
    std::unordered_map<ObjectID, obj_phy_loc_t*, ObjectHash> locMap;
    writeDataset(testdata, &locMap);

    // read dataset back and validate
    readDataset(objSize, testdata, locMap);

    // "delete" half of dataset
    fds_uint32_t delObjCount = dsize / 2;
    for (fds_uint32_t i = 0; i < delObjCount; ++i) {
        ObjectID oid = testdata.dataset_[i];
        persistData->notifyDataDeleted(oid, objSize, locMap[oid]);
    }

    GLOGDEBUG << "Initially put " << dsize << " objects of size "
              << objSize << ", marked for deletion " << delObjCount
              << " objects";

    // get stats for all the tokens and calculate total size and
    // total reclaimable size
    fds_uint64_t totalSize = 0;
    fds_uint64_t reclaimSize = 0;
    getTokenStats(&totalSize, &reclaimSize);

    // verify stats
    fds_uint64_t expectTotalSize = objSize * dsize;
    fds_uint64_t expectReclaimSize = objSize * delObjCount;
    EXPECT_EQ(expectTotalSize, totalSize);
    EXPECT_EQ(expectReclaimSize, reclaimSize);

    // restart
    restart();
    err = persistData->openObjectDataFiles(smDiskMap, false);
    EXPECT_TRUE(err.ok());

    // verify stats after restart
    getTokenStats(&totalSize, &reclaimSize);
    EXPECT_EQ(expectTotalSize, totalSize);
    // TODO(Anna) below verification will fail, because
    // our reclaim size is not persistent, need to revisit
    // EXPECT_EQ(expectReclaimSize, reclaimSize);

    // "start GC" which will create a new file to which we will write
    SmTokenSet smToks = smDiskMap->getSmTokens();
    for (SmTokenSet::const_iterator cit = smToks.cbegin();
         cit != smToks.cend();
         ++cit) {
        diskio::DataTier tier = diskio::diskTier;
        DiskId diskId = smDiskMap->getDiskId(*cit, tier);
        persistData->notifyStartGc(diskId, *cit, tier);
    }

    // at this point new files are empty
    getTokenStats(&totalSize, &reclaimSize);
    EXPECT_EQ(0u, totalSize);
    EXPECT_EQ(0u, reclaimSize);

    // write the same dataset (data store layer does not check dup, so
    // this will write all data to new file)
    std::unordered_map<ObjectID, obj_phy_loc_t*, ObjectHash> newLocMap;
    writeDataset(testdata, &newLocMap);

    // get stats (which are always for new file we are writing)
    getTokenStats(&totalSize, &reclaimSize);
    EXPECT_EQ(expectTotalSize, totalSize);
    EXPECT_EQ(0u, reclaimSize);

    // "stop GC" which will remove old files
    for (SmTokenSet::const_iterator cit = smToks.cbegin();
         cit != smToks.cend();
         ++cit) {
        diskio::DataTier tier = diskio::diskTier;
        DiskId diskId = smDiskMap->getDiskId(*cit, tier);
        persistData->notifyEndGc(diskId, *cit, tier);
    }

    // read dataset back from the new location and validate
    readDataset(objSize, testdata, newLocMap);

    // verify that old and new locations are different
    for (fds_uint32_t i = 0; i < testdata.dataset_.size(); ++i) {
        ObjectID oid = testdata.dataset_[i];
        obj_phy_loc_t* oldLoc = locMap[oid];
        obj_phy_loc_t* newLoc = newLocMap[oid];
        EXPECT_FALSE(oldLoc->obj_file_id == newLoc->obj_file_id);

        // delete location
        obj_phy_loc_t* loc = newLocMap[oid];
        delete loc;
        newLocMap.erase(oid);
        loc = locMap[oid];
        delete loc;
        locMap.erase(oid);
    }
}


}  // namespace fds

int main(int argc, char * argv[]) {
    fds::ObjPersistDataUtProc objPeristProc(argc, argv, "platform.conf",
                                            "fds.sm.", NULL);
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

