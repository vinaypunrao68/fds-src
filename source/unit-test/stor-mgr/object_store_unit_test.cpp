/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <cstdio>
#include <string>
#include <vector>
#include <bitset>

#include <boost/program_options.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fiu-control.h>
#include <fdsp/sm_types_types.h>
#include <util/Log.h>
#include <util/bloomfilter.h>
#include <object-store/LiveObjectsDB.h>
#include <fdsp_utils.h>
#include <fds_types.h>
#include <ObjectId.h>
#include <fds_module.h>
#include <fds_process.h>
#include <object-store/ObjectStore.h>
#include <sm_dataset.h>
#include <sm_ut_utils.h>

using ::testing::AtLeast;
using ::testing::Return;

namespace fds {

static fds_uint32_t hddCount = 0;
static fds_uint32_t ssdCount = 10;
static StorMgrVolumeTable* volTbl;
static ObjectStore::unique_ptr objectStore;
static TestVolume::ptr volume1;
static TestVolume::ptr largeCapVolume;
static TestVolume::ptr vlargeCapVolume;
static TestVolume::ptr largeObjVolume;
static TestVolume::ptr migrVolume;

class ObjectStoreTest : public FdsProcess {
  public:
    ObjectStoreTest(int argc, char *argv[],
                    Module **mod_vec) :
            FdsProcess(argc,
                       argv,
                       "platform.conf",
                       "fds.sm.",
                       "objstore-test.log",
                       mod_vec) {
    }
    ~ObjectStoreTest() {
    }
    int run() override {
        return 0;
    }
};

class SmObjectStoreTest: public ::testing::Test {
  public:
    SmObjectStoreTest()
            : numOps(0) {
        op_count = ATOMIC_VAR_INIT(0);
    }
    ~SmObjectStoreTest() {
    }

    virtual void TearDown() override;
    void task(TestVolume::StoreOpType opType,
              TestVolume::ptr volume,
              const Error& expectedError);
    void runMultithreadedTest(TestVolume::StoreOpType opType,
                              TestVolume::ptr volume,
                              const Error& expectedError);

    std::vector<std::thread*> threads_;
    std::atomic<fds_uint32_t> op_count;

    // this has to be initialized before running
    // multi-threaded tests
    fds_uint32_t numOps;
};

void SmObjectStoreTest::TearDown() {
}

void setupTests(fds_uint32_t concurrency,
                fds_uint32_t datasetSize) {
    fds_volid_t volId(98);
    fds_uint32_t sm_count = 1;
    fds_uint32_t cols = (sm_count < 4) ? sm_count : 4;

    const FdsRootDir *dir = g_fdsprocess->proc_fdsroot();
    const std::string devPath = dir->dir_dev();
    SmUtUtils::cleanAllInDir(devPath);

    const std::string statsPath = dir->dir_fds_var_stats();
    FdsRootDir::fds_mkdir(statsPath.c_str());
    SmUtUtils::cleanAllInDir(statsPath);

    // create our own disk map
    SmUtUtils::setupDiskMap(dir, hddCount, ssdCount);

    objectStore = ObjectStore::unique_ptr(
        new ObjectStore("Unit Test Object Store", NULL, volTbl));
    objectStore->mod_init(NULL);

    DLT* dlt = new DLT(16, cols, 1, true);
    SmUtUtils::populateDlt(dlt, sm_count);
    objectStore->handleNewDlt(dlt);
    LOGTRACE << "Starting...";

    // volume for single-volume test
    // we will ignore optype so just set put
    volume1.reset(new TestVolume(volId, "objectstore_ut_vol",
                                 1, 0,
                                 TestVolume::STORE_OP_PUT,
                                 datasetSize, 4096));
    volTbl->registerVolume(volume1->voldesc_);

    // large capacity volume for multi-threaded testing
    largeCapVolume.reset(new TestVolume(fds_volid_t(volId.get()+1), "ut_vol_capacity",
                                        concurrency, 0,
                                        TestVolume::STORE_OP_PUT,
                                        20*datasetSize, 4096));
    volTbl->registerVolume(largeCapVolume->voldesc_);

    // volume with large obj size (2MB)
    largeObjVolume.reset(new TestVolume(fds_volid_t(volId.get()+2), "ut_vol_2MB",
                                        1, 0,
                                        TestVolume::STORE_OP_PUT,
                                        datasetSize, 2*1024*1024));
    volTbl->registerVolume(largeObjVolume->voldesc_);

    // volume we will use to test migration code
    migrVolume.reset(new TestVolume(fds_volid_t(volId.get()+3), "ut_migration_vol",
                                    1, 0,
                                    TestVolume::STORE_OP_PUT,
                                    datasetSize, 4096));
    volTbl->registerVolume(migrVolume->voldesc_);

    delete dlt;
}

void SmObjectStoreTest::task(TestVolume::StoreOpType opType,
                             TestVolume::ptr volume,
                             const Error& expectedError) {
    Error err(ERR_OK);
    fds_uint64_t seed = RandNumGenerator::getRandSeed();
    RandNumGenerator rgen(seed);
    fds_uint32_t datasetSize = (volume->testdata_).dataset_.size();
    fds_volid_t volId((volume->voldesc_).volUUID);
    fds_uint32_t ops = atomic_fetch_add(&op_count, (fds_uint32_t)1);

    while ((ops + 1) <= numOps) {
        fds_uint32_t index = ops % datasetSize;
        if ((opType == TestVolume::STORE_OP_GET) ||
            (opType == TestVolume::STORE_OP_DUPLICATE)) {
            index = ((fds_uint32_t)rgen.genNum()) % datasetSize;
        }
        ObjectID oid = (volume->testdata_).dataset_[index];
        switch (opType) {
            case TestVolume::STORE_OP_PUT:
            case TestVolume::STORE_OP_DUPLICATE:
                {
                    boost::shared_ptr<std::string> data =
                            (volume->testdata_).dataset_map_[oid].getObjectData();
                    err = objectStore->putObject(volId, oid, data, false);
                    EXPECT_EQ(err, expectedError);
                }
                break;
            case TestVolume::STORE_OP_GET:
                {
                    boost::shared_ptr<const std::string> data;
                    diskio::DataTier usedTier = diskio::maxTier;
                    data = objectStore->getObject(volId, oid, usedTier, err);
                    EXPECT_EQ(err, expectedError);
                    if (err.ok()) {
                        EXPECT_TRUE((volume->testdata_).dataset_map_[oid].isValid(data));
                    }
                }
                break;
            case TestVolume::STORE_OP_DELETE:
                err = objectStore->deleteObject(volId, oid, false);
                EXPECT_EQ(err, expectedError);
                break;
            default:
                fds_verify(false);   // new type added?
        };
        ops = atomic_fetch_add(&op_count, (fds_uint32_t)1);
    }
}

void
SmObjectStoreTest::runMultithreadedTest(TestVolume::StoreOpType opType,
                                        TestVolume::ptr volume,
                                        const Error& expectedError) {
    GLOGDEBUG << "Will do multi-threded op type " << opType
              << " volume " << (volume->voldesc_).name
              << " dataset size " << (volume->testdata_).dataset_.size()
              << " num ops " << numOps
              << " concurrency " << volume->concurrency_;

    // setup the run
    EXPECT_EQ(threads_.size(), 0u);
    op_count = ATOMIC_VAR_INIT(0);

    for (unsigned i = 0; i < volume->concurrency_; ++i) {
        std::thread* new_thread = new std::thread(&SmObjectStoreTest::task, this,
                                                  opType, volume, expectedError);
        threads_.push_back(new_thread);
    }
    // wait for all threads
    for (unsigned x = 0; x < volume->concurrency_; ++x) {
        threads_[x]->join();
    }

    // cleanup
    for (unsigned x = 0; x < volume->concurrency_; ++x) {
        std::thread* th = threads_[x];
        delete th;
        threads_[x] = NULL;
    }
    threads_.clear();
}

/**
 * Expunge TC 1
 * This testcase adds only N/2 of the objects into the bloom filter out
 * of total N objects put in the object DB. Basically trying to simulate
 * deleted objects.
 */
TEST_F(SmObjectStoreTest, evaluate_object_sets) {
    Error err(ERR_OK);
    fds_token_id smToken = 1; // test sm token.
    //test vol
    vlargeCapVolume.reset(new TestVolume(fds_volid_t(200), "ut_vol_vcapacity",
                                         20, 0,
                                         TestVolume::STORE_OP_PUT,
                                         400, 4096, smToken));
    volTbl->registerVolume(vlargeCapVolume->voldesc_);
    std::unique_ptr<util::BloomFilter> bf(new util::BloomFilter());

    bool ignorePut = true;
    // Add only N/2 objects to the bloom filter.
    for (fds_uint32_t i = 0; i < (vlargeCapVolume->testdata_).dataset_.size(); ++i) {
        ObjectID oid = (vlargeCapVolume->testdata_).dataset_[i];
        boost::shared_ptr<std::string> data =
                (vlargeCapVolume->testdata_).dataset_map_[oid].getObjectData();
        if ((SmDiskMap::smTokenId(oid, bitsPerDltToken) == smToken)) {
            err = objectStore->putObject((vlargeCapVolume->voldesc_).volUUID, oid, data, false);
            if (ignorePut) {
                bf->add(oid);
                ignorePut = false;
            } else {
                ignorePut = true;
            }
            EXPECT_TRUE(err.ok());
        }
    }
    std::string bfFileName("vlargeCapVolume.bf");
    serialize::Serializer* s=serialize::getFileSerializer(bfFileName);
    if (bf->write(s) <= 0) {
        std::cout << "write failed" << std::endl;
    }
    delete s;

    diskio::TokenStat tokStats;
    /**
     * Simulating the situation where SM received object delete count threshold number of
     * bloom filters. Here each bloom filter basically has N/2 absent objects. And once the
     * total delete count crosses the threshold, the token_reclaim_size takes note of that.
     */
    fds_uint8_t deleteCntThreshold = objectStore->getObjectDelCnt();
    for (fds_uint8_t incDelCount = 0; incDelCount < deleteCntThreshold; ++incDelCount) {
        objectStore->addObjectSet(smToken, (vlargeCapVolume->voldesc_).volUUID, 1,
                                  util::getTimeStampNanos(), bfFileName);
        objectStore->evaluateObjectSets(smToken, diskio::maxTier, tokStats);

        // object will be ready to be claimed only when the delete count threshold is reached.
        if (incDelCount < (deleteCntThreshold - 1)) {
            EXPECT_EQ(tokStats.tkn_reclaim_size, 0);
        } else {
            fds_uint64_t toDelete = tokStats.tkn_tot_size / 2;
            EXPECT_EQ(tokStats.tkn_reclaim_size, toDelete);
        }

        objectStore->removeObjectSet(smToken, (vlargeCapVolume->voldesc_).volUUID);
    }

    float_t used_pct = objectStore->getUsedCapacityAsPct();
    EXPECT_TRUE(used_pct > 0);

    if (unlink(bfFileName.c_str()) < 0) {
        GLOGERROR << "Can't unlink file : " << bfFileName;
    }
    objectStore->dropLiveObjectDB();
}

TEST_F(SmObjectStoreTest, one_thread_puts) {
    Error err(ERR_OK);

    // populate store
    for (fds_uint32_t i = 0; i < (volume1->testdata_).dataset_.size(); ++i) {
        ObjectID oid = (volume1->testdata_).dataset_[i];
        boost::shared_ptr<std::string> data =
                (volume1->testdata_).dataset_map_[oid].getObjectData();
        err = objectStore->putObject((volume1->voldesc_).volUUID, oid, data, false);
        EXPECT_TRUE(err.ok());
    }
    float_t used_pct = objectStore->getUsedCapacityAsPct();
    EXPECT_TRUE(used_pct > 0);
}

TEST_F(SmObjectStoreTest, one_thread_gets) {
    Error err(ERR_OK);

    // read back and validate
    for (fds_uint32_t i = 0; i < (volume1->testdata_).dataset_.size(); ++i) {
        ObjectID oid = (volume1->testdata_).dataset_[i];
        boost::shared_ptr<const std::string> objData;
        diskio::DataTier usedTier = diskio::maxTier;
        objData = objectStore->getObject((volume1->voldesc_).volUUID, oid, usedTier, err);
        EXPECT_TRUE(err.ok());
        EXPECT_TRUE((volume1->testdata_).dataset_map_[oid].isValid(objData));
    }
}

TEST_F(SmObjectStoreTest, one_thread_dup_puts) {
    Error err(ERR_OK);

    // every third object is dup
    for (fds_uint32_t i = 0; i < (volume1->testdata_).dataset_.size(); i+=3) {
        ObjectID oid = (volume1->testdata_).dataset_[i];
        boost::shared_ptr<std::string> data =
                (volume1->testdata_).dataset_map_[oid].getObjectData();
        err = objectStore->putObject((volume1->voldesc_).volUUID, oid, data, false);
        EXPECT_TRUE(err.ok());
        if (!err.ok()) {
            std::cout << oid << " putObject returned " << err << std::endl;
        }
    }
}

TEST_F(SmObjectStoreTest, move_to_ssd) {
    Error err(ERR_OK);

    // we only run this test when both HDDs and SSDs are present
    if ((ssdCount == 0) || (hddCount == 0)) {
        return;
    }

    for (fds_uint32_t i = 0; i < (volume1->testdata_).dataset_.size(); ++i) {
        ObjectID oid = (volume1->testdata_).dataset_[i];
        boost::shared_ptr<const std::string> retData;

        // move this object to SSD tier
        err = objectStore->moveObjectToTier(oid, diskio::diskTier, diskio::flashTier, false);
        EXPECT_TRUE(err.ok());

        // read again (should read from SSD tier)
        diskio::DataTier usedTier = diskio::maxTier;
        retData = objectStore->getObject((volume1->voldesc_).volUUID, oid, usedTier, err);
        EXPECT_TRUE(err.ok());
        EXPECT_TRUE(usedTier == diskio::flashTier);
        EXPECT_TRUE((volume1->testdata_).dataset_map_[oid].isValid(retData));
    }

    // move one object again
    ObjectID oid = (volume1->testdata_).dataset_[0];
    err = objectStore->moveObjectToTier(oid, diskio::diskTier, diskio::flashTier, false);
    EXPECT_TRUE(err == ERR_DUPLICATE);
}

void
initMetaDataPropagate(fpi::CtrlObjectMetaDataPropagate &msg) {
    // cleanup first
    msg.objectVolumeAssoc.clear();
    msg.objectData.clear();
    msg.objectSize = 0;

    // init
    fpi::MetaDataVolumeAssoc volAssoc;
    volAssoc.volumeAssoc = (migrVolume->voldesc_).volUUID.get();
    volAssoc.volumeRefCnt = 1;
    msg.objectReconcileFlag = fpi::OBJ_METADATA_NO_RECONCILE;
    msg.objectVolumeAssoc.push_back(volAssoc);
    msg.objectRefCnt = 1;
    msg.objectCompressType = 0;
    msg.objectCompressLen = 0;
    msg.objectBlkLen = 4096;
    msg.objectFlags = 0;
    msg.objectExpireTime = 0;
}

TEST_F(SmObjectStoreTest, apply_deltaset) {
    Error err(ERR_OK);
    fpi::CtrlObjectMetaDataPropagate msg;
    initMetaDataPropagate(msg);

    // apply first half of objects
    fds_uint32_t firstSetSize = (migrVolume->testdata_).dataset_.size() / 2;
    fds_uint32_t secondSetSize = (migrVolume->testdata_).dataset_.size() - firstSetSize;

    LOGDEBUG << "Applying first half of objects to object store";
    for (fds_uint32_t i = 0; i < firstSetSize; ++i) {
        ObjectID oid = (migrVolume->testdata_).dataset_[i];
        boost::shared_ptr<std::string> data =
                (migrVolume->testdata_).dataset_map_[oid].getObjectData();

        // update msg fields that depend on particular object/test
        initMetaDataPropagate(msg);
        fds::assign(msg.objectID, oid);
        msg.objectData.clear();
        msg.objectData.resize(data->length());
        msg.objectData.assign(*data);
        msg.objectSize = data->length();

        // apply this delta -- this this the first time
        err = objectStore->applyObjectMetadataData(oid, msg);
        EXPECT_TRUE(err.ok());

        // read and see if data is there...
        boost::shared_ptr<const std::string> retData;
        diskio::DataTier usedTier = diskio::maxTier;
        retData = objectStore->getObject((migrVolume->voldesc_).volUUID, oid, usedTier, err);
        EXPECT_TRUE(err.ok());
        EXPECT_TRUE((migrVolume->testdata_).dataset_map_[oid].isValid(retData));
    }

    // apply next object without data, must return error
    fds_uint32_t index = firstSetSize;
    EXPECT_TRUE(index < (migrVolume->testdata_).dataset_.size());
    msg.objectData.clear();
    msg.objectSize = 0;
    err = objectStore->applyObjectMetadataData((migrVolume->testdata_).dataset_[index], msg);
    EXPECT_TRUE(err == ERR_SM_TOK_MIGRATION_NO_DATA_RECVD);

    // apply object with data during second phase, when object
    // did not exist during the first phase
    ++index;
    EXPECT_TRUE(index < (migrVolume->testdata_).dataset_.size());
    ObjectID newOid = (migrVolume->testdata_).dataset_[index];
    boost::shared_ptr<std::string> newData =
            (migrVolume->testdata_).dataset_map_[newOid].getObjectData();
    initMetaDataPropagate(msg);
    fds::assign(msg.objectID, newOid);
    // if we add data during second phase, we should still set reconcile = false
    msg.objectReconcileFlag = fpi::OBJ_METADATA_NO_RECONCILE;
    msg.objectData.clear();
    msg.objectData.resize(newData->length());
    msg.objectData.assign(*newData);
    msg.objectSize = newData->length();
    LOGDEBUG << "Object with data during second phase " << newOid;
    err = objectStore->applyObjectMetadataData(newOid, msg);
    EXPECT_TRUE(err.ok());
    // read and see if data is there...
    boost::shared_ptr<const std::string> retNewData;
    diskio::DataTier usedTierNew = diskio::maxTier;
    retNewData = objectStore->getObject((migrVolume->voldesc_).volUUID, newOid, usedTierNew, err);
    EXPECT_TRUE(err.ok());
    EXPECT_TRUE((migrVolume->testdata_).dataset_map_[newOid].isValid(retNewData));
    ++index;

    // apply second set of objects + one more metadata change (refcnt ++)
    LOGDEBUG << "Applying second half of objects to object store";
    for (fds_uint32_t i = index; i < (migrVolume->testdata_).dataset_.size(); ++i) {
        ObjectID oid = (migrVolume->testdata_).dataset_[i];
        boost::shared_ptr<std::string> data =
                (migrVolume->testdata_).dataset_map_[oid].getObjectData();

        // update msg fields that depend on particular object/test
        initMetaDataPropagate(msg);
        fds::assign(msg.objectID, oid);
        msg.objectReconcileFlag = fpi::OBJ_METADATA_NO_RECONCILE;
        msg.objectData.clear();
        msg.objectData.resize(data->length());
        msg.objectData.assign(*data);
        msg.objectSize = data->length();

        // apply this delta -- this this the first time
        err = objectStore->applyObjectMetadataData(oid, msg);
        EXPECT_TRUE(err.ok());

        // read and see if data is there...
        boost::shared_ptr<const std::string> retData;
        diskio::DataTier usedTier = diskio::maxTier;
        retData = objectStore->getObject((migrVolume->voldesc_).volUUID, oid, usedTier, err);
        EXPECT_TRUE(err.ok());
        EXPECT_TRUE((migrVolume->testdata_).dataset_map_[oid].isValid(retData));

        // increase refcount (+2) and apply metadata again
        msg.objectReconcileFlag = fpi::OBJ_METADATA_RECONCILE;
        msg.objectRefCnt = 2;
        msg.objectVolumeAssoc[0].volumeRefCnt = 2;
        err = objectStore->applyObjectMetadataData(oid, msg);
        EXPECT_TRUE(err.ok());

        // decrease refcnt (-1) and apply metadata again
        msg.objectReconcileFlag = fpi::OBJ_METADATA_RECONCILE;
        msg.objectRefCnt = -1;
        msg.objectVolumeAssoc[0].volumeRefCnt = -1;
        err = objectStore->applyObjectMetadataData(oid, msg);
        EXPECT_TRUE(err.ok());

        // add new volume association
        fpi::MetaDataVolumeAssoc volAssoc2;
        volAssoc2.volumeAssoc = (volume1->voldesc_).volUUID.get();
        volAssoc2.volumeRefCnt = 3;
        msg.objectRefCnt = 3;
        // setting it to 0, so volume[0] doesn't factor into reconciling.
        msg.objectVolumeAssoc[0].volumeRefCnt = 0;
        msg.objectVolumeAssoc.push_back(volAssoc2);
        err = objectStore->applyObjectMetadataData(oid, msg);
        EXPECT_TRUE(err.ok());

        // remove volume association we just added
        msg.objectVolumeAssoc[1].volumeRefCnt = -3;
        msg.objectRefCnt = -3;
        err = objectStore->applyObjectMetadataData(oid, msg);
        EXPECT_TRUE(err.ok());

        // remove it again -- should be an error
        msg.objectVolumeAssoc[1].volumeRefCnt = -3;
        msg.objectRefCnt = -3;
        err = objectStore->applyObjectMetadataData(oid, msg);
        EXPECT_FALSE(err.ok());

        // read data again
        boost::shared_ptr<const std::string> retDataAgain;
        retDataAgain = objectStore->getObject((migrVolume->voldesc_).volUUID, oid, usedTier, err);
        EXPECT_TRUE(err.ok());
        EXPECT_TRUE((migrVolume->testdata_).dataset_map_[oid].isValid(retDataAgain));
    }
}

/// this test MUST be before concurrent_puts, otherwise put will be dup
/// and will not go to the persistent layer
TEST_F(SmObjectStoreTest, concurrent_puts_fail) {
    // for puts, do num ops = dataset size
    GLOGDEBUG << "Running concurrent_puts_fail test";
    numOps = (largeCapVolume->testdata_).dataset_.size();
    fiu_enable("sm.persist.writefail", 1, NULL, 0);
    runMultithreadedTest(TestVolume::STORE_OP_PUT, largeCapVolume, ERR_DISK_WRITE_FAILED);
    fiu_disable("sm.persist.writefail");
}

TEST_F(SmObjectStoreTest, concurrent_puts) {
    // for puts, do num ops = dataset size
    GLOGDEBUG << "Running concurrent_puts test";
    numOps = (largeCapVolume->testdata_).dataset_.size();
    runMultithreadedTest(TestVolume::STORE_OP_PUT, largeCapVolume, ERR_OK);
}

TEST_F(SmObjectStoreTest, concurrent_gets) {
    // for gets, do num ops = 2*dataset size
    GLOGDEBUG << "Running concurrent_gets test";
    numOps = 2 * (largeCapVolume->testdata_).dataset_.size();
    runMultithreadedTest(TestVolume::STORE_OP_GET, largeCapVolume, ERR_OK);
}

TEST_F(SmObjectStoreTest, increase_concurrency_gets) {
    // for gets, do num ops = 2*dataset size
    GLOGDEBUG << "Running increase_concurrent_gets test";
    numOps = 2 * (largeCapVolume->testdata_).dataset_.size();

    for (fds_uint32_t i = 2; i < 70; i *= 2) {
        largeCapVolume->concurrency_ = i;
        runMultithreadedTest(TestVolume::STORE_OP_GET, largeCapVolume, ERR_OK);
    }
}

TEST_F(SmObjectStoreTest, increase_concurrency_dup_puts) {
    // for gets, do num ops = 2*dataset size
    GLOGDEBUG << "Running increase_concurrency_dup_puts test";
    numOps = 2 * (largeCapVolume->testdata_).dataset_.size();

    for (fds_uint32_t i = 2; i < 70; i *= 2) {
        largeCapVolume->concurrency_ = i;
        runMultithreadedTest(TestVolume::STORE_OP_DUPLICATE, largeCapVolume, ERR_OK);
    }
}

TEST_F(SmObjectStoreTest, concurrent_puts_2mb) {
    // for puts, do num ops = dataset size
    GLOGDEBUG << "Running concurrent_puts_2mb test";
    numOps = (largeObjVolume->testdata_).dataset_.size();
    runMultithreadedTest(TestVolume::STORE_OP_PUT, largeObjVolume, ERR_OK);
}

TEST_F(SmObjectStoreTest, concurrent_gets_2mb) {
    // for gets, do num ops = 2*dataset size
    GLOGDEBUG << "Running concurrent_gets_2mb test";
    numOps = 2 * (largeObjVolume->testdata_).dataset_.size();
    runMultithreadedTest(TestVolume::STORE_OP_GET, largeObjVolume, ERR_OK);
}

TEST_F(SmObjectStoreTest, findSrcSMForTokenSyncTest) {
    fds_uint32_t sm_count = 4;
    fds_uint32_t cols = 4;

    DLT* dlt = new DLT(16, cols, 1, true);
    SmUtUtils::populateDlt(dlt, sm_count);
    DLT::SourceNodeMap srcNodeMap;

    unsigned destSm = 1; // destination SM id
    unsigned srcSm = 2;  // expected source SM id to be assinged for resync
    dlt->getSourceForAllNodeTokens(NodeUuid(destSm), srcNodeMap);

    for (auto obj : srcNodeMap) {
         ASSERT_EQ(obj.first, NodeUuid(srcSm));
    }

    destSm = 3; // destination SM id
    srcSm = 1; // expected source SM id to be assigned for resync
    srcNodeMap.clear();
    dlt->getSourceForAllNodeTokens(NodeUuid(destSm), srcNodeMap);

    for (auto obj : srcNodeMap) {
         ASSERT_EQ(obj.first, NodeUuid(srcSm));
    }

    //invalid destination SM id test
    destSm = 10000; // destination SM id
    srcNodeMap.clear();
    dlt->getSourceForAllNodeTokens(NodeUuid(destSm), srcNodeMap);
    ASSERT_EQ(srcNodeMap.size(), 0u);

    delete dlt;
}

TEST_F(SmObjectStoreTest, findNewSrcSMRetryTest) {
    fds_uint32_t sm_count = 4;
    fds_uint32_t cols = 4;

    DLT* dlt = new DLT(16, cols, 1, true);
    SmUtUtils::populateDlt(dlt, sm_count);
    DLT::SourceNodeMap srcNodeMap;
    uint32_t numOfTokens = pow(2, dlt->getWidth());

    std::set<fds_token_id> dltTokens;
    std::map<NodeUuid, bool> failedSMs;

    for (uint32_t  i = 0; i < numOfTokens; i++) {
        dltTokens.insert(i);
    }
    unsigned curSrcSm = 1;
    unsigned newSrcSm = 2; // expected source SM id to be assinged for resync
    unsigned retryCount = 1;
    NodeTokenMap nodeGroups = dlt->getNewSourceSMs(NodeUuid(curSrcSm),
                                                   dltTokens,
                                                   retryCount,
                                                   failedSMs);
    for (auto obj : nodeGroups) {
         ASSERT_EQ(obj.first, NodeUuid(newSrcSm));
    }
    ASSERT_EQ(nodeGroups[NodeUuid(newSrcSm)].size(), numOfTokens);
    delete dlt;
}

TEST_F(SmObjectStoreTest, findNewSrcSMRetryTwoTest) {
    fds_uint32_t sm_count = 4;
    fds_uint32_t cols = 4;

    DLT* dlt = new DLT(16, cols, 1, true);
    SmUtUtils::populateDlt(dlt, sm_count);
    DLT::SourceNodeMap srcNodeMap;
    uint32_t numOfTokens = pow(2, dlt->getWidth());

    std::set<fds_token_id> dltTokens;
    std::map<NodeUuid, bool> failedSMs;

    for (uint32_t  i = 0; i < numOfTokens; i++) {
        dltTokens.insert(i);
    }

    unsigned curSrcSm = 3;
    unsigned newSrcSm = 2; // expected source SM id to be assinged for resync
    unsigned retryCount = 3;
    NodeTokenMap nodeGroups = dlt->getNewSourceSMs(NodeUuid(curSrcSm),
                                                   dltTokens,
                                                   retryCount,
                                                   failedSMs);
    for (auto obj : nodeGroups) {
         ASSERT_EQ(obj.first, NodeUuid(newSrcSm));
    }
    ASSERT_EQ(nodeGroups[NodeUuid(newSrcSm)].size(), numOfTokens);
    delete dlt;
}

TEST_F(SmObjectStoreTest, findNewSrcSMRetryFailedSrcTest) {
    fds_uint32_t sm_count = 4;
    fds_uint32_t cols = 4;

    DLT* dlt = new DLT(16, cols, 1, true);
    SmUtUtils::populateDlt(dlt, sm_count);
    DLT::SourceNodeMap srcNodeMap;
    uint32_t numOfTokens = pow(2, dlt->getWidth());

    std::set<fds_token_id> dltTokens;
    std::map<NodeUuid, bool> failedSMs;

    for (uint32_t  i = 0; i < numOfTokens; i++) {
        dltTokens.insert(i);
    }
    failedSMs[NodeUuid(4)] = true;
    failedSMs[NodeUuid(1)] = true;

    unsigned curSrcSm = 3;
    unsigned newSrcSm = 2; // expected source SM id to be assinged for resync
    unsigned retryCount = 1;
    NodeTokenMap nodeGroups = dlt->getNewSourceSMs(NodeUuid(curSrcSm),
                                                   dltTokens,
                                                   retryCount,
                                                   failedSMs);
    for (auto obj : nodeGroups) {
         ASSERT_EQ(obj.first, NodeUuid(newSrcSm));
    }
    ASSERT_EQ(nodeGroups[NodeUuid(newSrcSm)].size(), numOfTokens);
    delete dlt;

}

TEST_F(SmObjectStoreTest, findNewSrcSMAllInvalidTest) {
    fds_uint32_t sm_count = 4;
    fds_uint32_t cols = 4;

    DLT* dlt = new DLT(16, cols, 1, true);
    SmUtUtils::populateDlt(dlt, sm_count);
    DLT::SourceNodeMap srcNodeMap;
    uint32_t numOfTokens = pow(2, dlt->getWidth());

    std::set<fds_token_id> dltTokens;
    std::map<NodeUuid, bool> failedSMs;

    for (uint32_t  i = 0; i < numOfTokens; i++) {
        dltTokens.insert(i);
    }
    failedSMs[NodeUuid(4)] = true;
    failedSMs[NodeUuid(3)] = true;
    failedSMs[NodeUuid(1)] = true;

    unsigned curSrcSm = 2;
    unsigned retryCount = 1;
    NodeTokenMap nodeGroups = dlt->getNewSourceSMs(NodeUuid(curSrcSm),
                                                   dltTokens,
                                                   retryCount,
                                                   failedSMs);
    for (auto obj : nodeGroups) {
         ASSERT_EQ(obj.first, INVALID_RESOURCE_UUID);
    }
    ASSERT_EQ(nodeGroups[INVALID_RESOURCE_UUID].size(), numOfTokens);
    delete dlt;
}

TEST_F(SmObjectStoreTest, concurrent_gets_fail) {
    // for gets, do num ops = 2*dataset size
    GLOGDEBUG << "Running concurrent_gets test";
    numOps = 2 * (largeCapVolume->testdata_).dataset_.size();
    fiu_enable("sm.persist.meta_readfail", 1, NULL, 0);
    runMultithreadedTest(TestVolume::STORE_OP_GET, largeCapVolume, ERR_NO_BYTES_READ);
    fiu_disable("sm.persist.meta_readfail");
}

// NEW TESTS SHOULD BE ADDED BEFORE concurrent_gets_fail
// becuase we are 'failing' the disks, so the state of object store not
// got for further testing

}  // namespace fds

int
main(int argc, char** argv) {
    fds::ObjectStoreTest objectStoreTest(argc, argv, NULL);
    ::testing::InitGoogleMock(&argc, argv);

    namespace po = boost::program_options;
    po::options_description progDesc("SM Object Store Unit Test options");
    progDesc.add_options()
            ("help,h", "Help Message")
            ("hdd-count",
             po::value<fds_uint32_t>(&hddCount)->default_value(12),
             "Number of HDDs")
            ("ssd-count",
             po::value<fds_uint32_t>(&ssdCount)->default_value(2),
             "Number of SSDs");
    po::variables_map varMap;
    po::parsed_options parsedOpt =
            po::command_line_parser(argc, argv).options(progDesc).allow_unregistered().run();                                                                               
    po::store(parsedOpt, varMap);
    po::notify(varMap);

    std::cout << "Disk map config: " << hddCount << " HDDs, "
              << ssdCount << " SSDs" << std::endl;

    // sets up dataset and object store
    volTbl = new StorMgrVolumeTable();
    setupTests(30, 20);

    int ret = RUN_ALL_TESTS();

    // cleanup
    objectStore->mod_shutdown();
    delete volTbl;
    SmUtUtils::cleanAllInDir(g_fdsprocess->proc_fdsroot()->dir_dev());

    return ret;
}
