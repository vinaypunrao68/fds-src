/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <cstdio>
#include <string>
#include <vector>
#include <bitset>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <util/Log.h>
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

static StorMgrVolumeTable* volTbl;
static ObjectStore::unique_ptr objectStore;
static TestVolume::ptr volume1;
static TestVolume::ptr largeCapVolume;
static TestVolume::ptr largeObjVolume;

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
              TestVolume::ptr volume);
    void runMultithreadedTest(TestVolume::StoreOpType opType,
                              TestVolume::ptr volume);

    std::vector<std::thread*> threads_;
    std::atomic<fds_uint32_t> op_count;

    // this has to be initialized before running
    // multi-threaded tests
    fds_uint32_t numOps;
};

void SmObjectStoreTest::TearDown() {
}

void setupTests(fds_uint32_t hddCount,
                fds_uint32_t ssdCount,
                fds_uint32_t concurrency,
                fds_uint32_t datasetSize) {
    fds_volid_t volId = 98;
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
    largeCapVolume.reset(new TestVolume(volId+1, "ut_vol_capacity",
                                        concurrency, 0,
                                        TestVolume::STORE_OP_PUT,
                                        20*datasetSize, 4096));
    volTbl->registerVolume(largeCapVolume->voldesc_);

    // volume with large obj size (2MB)
    largeObjVolume.reset(new TestVolume(volId+2, "ut_vol_2MB",
                                        1, 0,
                                        TestVolume::STORE_OP_PUT,
                                        datasetSize, 2*1024*1024));
    volTbl->registerVolume(largeObjVolume->voldesc_);

    delete dlt;
}

void SmObjectStoreTest::task(TestVolume::StoreOpType opType,
                             TestVolume::ptr volume) {
    Error err(ERR_OK);
    fds_uint64_t seed = RandNumGenerator::getRandSeed();
    RandNumGenerator rgen(seed);
    fds_uint32_t datasetSize = (volume->testdata_).dataset_.size();
    fds_volid_t volId = (volume->voldesc_).volUUID;
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
                    err = objectStore->putObject(volId, oid, data);
                    EXPECT_TRUE(err.ok());
                }
                break;
            case TestVolume::STORE_OP_GET:
                {
                    boost::shared_ptr<const std::string> data;
                    diskio::DataTier usedTier = diskio::maxTier;
                    data = objectStore->getObject(volId, oid, usedTier, err);
                    EXPECT_TRUE(err.ok());
                    EXPECT_TRUE((volume->testdata_).dataset_map_[oid].isValid(data));
                }
                break;
            case TestVolume::STORE_OP_DELETE:
                err = objectStore->deleteObject(volId, oid);
                EXPECT_TRUE(err.ok());
                break;
            default:
                fds_verify(false);   // new type added?
        };
        ops = atomic_fetch_add(&op_count, (fds_uint32_t)1);
    }
}

void
SmObjectStoreTest::runMultithreadedTest(TestVolume::StoreOpType opType,
                                        TestVolume::ptr volume) {
    GLOGDEBUG << "Will do multi-threded op type " << opType
              << " volume " << (volume->voldesc_).name
              << " dataset size " << (volume->testdata_).dataset_.size()
              << " num ops " << numOps
              << " concurrency " << volume->concurrency_;

    // setup the run
    EXPECT_EQ(threads_.size(), 0);
    op_count = ATOMIC_VAR_INIT(0);

    for (unsigned i = 0; i < volume->concurrency_; ++i) {
        std::thread* new_thread = new std::thread(&SmObjectStoreTest::task, this,
                                                  opType, volume);
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

TEST_F(SmObjectStoreTest, one_thread_puts) {
    Error err(ERR_OK);

    // populate store
    for (fds_uint32_t i = 0; i < (volume1->testdata_).dataset_.size(); ++i) {
        ObjectID oid = (volume1->testdata_).dataset_[i];
        boost::shared_ptr<std::string> data =
                (volume1->testdata_).dataset_map_[oid].getObjectData();
        err = objectStore->putObject((volume1->voldesc_).volUUID, oid, data);
        EXPECT_TRUE(err.ok());
    }
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
        err = objectStore->putObject((volume1->voldesc_).volUUID, oid, data);
        EXPECT_TRUE(err.ok());
    }
}

TEST_F(SmObjectStoreTest, move_to_ssd) {
    Error err(ERR_OK);

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

TEST_F(SmObjectStoreTest, concurrent_puts) {
    // for puts, do num ops = dataset size
    GLOGDEBUG << "Running concurrent_puts test";
    numOps = (largeCapVolume->testdata_).dataset_.size();
    runMultithreadedTest(TestVolume::STORE_OP_PUT, largeCapVolume);
}

TEST_F(SmObjectStoreTest, concurrent_gets) {
    // for gets, do num ops = 2*dataset size
    GLOGDEBUG << "Running concurrent_gets test";
    numOps = 2 * (largeCapVolume->testdata_).dataset_.size();
    runMultithreadedTest(TestVolume::STORE_OP_GET, largeCapVolume);
}

TEST_F(SmObjectStoreTest, increase_concurrency_gets) {
    // for gets, do num ops = 2*dataset size
    GLOGDEBUG << "Running increase_concurrent_gets test";
    numOps = 2 * (largeCapVolume->testdata_).dataset_.size();

    for (fds_uint32_t i = 2; i < 70; i *= 2) {
        largeCapVolume->concurrency_ = i;
        runMultithreadedTest(TestVolume::STORE_OP_GET, largeCapVolume);
    }
}

TEST_F(SmObjectStoreTest, increase_concurrency_dup_puts) {
    // for gets, do num ops = 2*dataset size
    GLOGDEBUG << "Running increase_concurrency_dup_puts test";
    numOps = 2 * (largeCapVolume->testdata_).dataset_.size();

    for (fds_uint32_t i = 2; i < 70; i *= 2) {
        largeCapVolume->concurrency_ = i;
        runMultithreadedTest(TestVolume::STORE_OP_DUPLICATE, largeCapVolume);
    }
}

TEST_F(SmObjectStoreTest, concurrent_puts_2mb) {
    // for puts, do num ops = dataset size
    GLOGDEBUG << "Running concurrent_puts_2mb test";
    numOps = (largeObjVolume->testdata_).dataset_.size();
    runMultithreadedTest(TestVolume::STORE_OP_PUT, largeObjVolume);
}

TEST_F(SmObjectStoreTest, concurrent_gets_2mb) {
    // for gets, do num ops = 2*dataset size
    GLOGDEBUG << "Running concurrent_gets_2mb test";
    numOps = 2 * (largeObjVolume->testdata_).dataset_.size();
    runMultithreadedTest(TestVolume::STORE_OP_GET, largeObjVolume);
}

}  // namespace fds

int
main(int argc, char** argv) {
    fds::ObjectStoreTest objectStoreTest(argc, argv, NULL);
    ::testing::InitGoogleMock(&argc, argv);

    fds_uint32_t hddCount = 10;
    fds_uint32_t ssdCount = 2;

    // sets up dataset and object store
    volTbl = new StorMgrVolumeTable();
    setupTests(hddCount, ssdCount, 30, 20);

    int ret = RUN_ALL_TESTS();

    // cleanup
    objectStore->mod_shutdown();
    delete volTbl;
    SmUtUtils::cleanAllInDir(g_fdsprocess->proc_fdsroot()->dir_dev());

    return ret;
}
