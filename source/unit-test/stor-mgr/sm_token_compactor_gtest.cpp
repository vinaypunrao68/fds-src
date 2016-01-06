/**
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <unistd.h>
#include <set>
#include <string>

#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>

#include <chrono>
#include <condition_variable>
#include <sm_ut_utils.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <fds_process.h>
#include <FdsRandom.h>
#include <odb.h>
#include <ObjectId.h>
#include <object-store/ObjectPersistData.h>
#include <object-store/TokenCompactor.h>

using ::testing::AtLeast;
using ::testing::Return;

namespace fds {

static std::string logname = "sm_token_compactor";
static const fds_uint32_t qosThreads = 10;
static const fds_uint16_t activeFileId = 2;
static const fds_uint16_t shadowFileId = 0xfff2;

class SmTokenCompactorUtProc : public FdsProcess {
  public:
    SmTokenCompactorUtProc(int argc, char * argv[], const std::string & config,
                           const std::string & basePath, Module * vec[]) {
        init(argc, argv, config, basePath, logname, vec);
    }

    virtual int run() override {
        return 0;
    }
};

// Test implementation of SmIoReqHandler
class TestReqHandler: public SmIoReqHandler {
  public:
    TestReqHandler() : SmIoReqHandler(), curObjsCopied(0) {
        threadPool = new fds_threadpool(qosThreads);
        odb = NULL;
        totalObjsInDb = 0;
        copiedObjsInDb = 0;
    }
    virtual ~TestReqHandler() {
        delete threadPool;
        delete odb;
    }

    void createObjectDB() {
        const FdsRootDir *dir = g_fdsprocess->proc_fdsroot();
        const std::string odbDir = dir->dir_fds_var_tests();
        std::string filename = odbDir + "/TestObjectIndex";
        odb = SmUtUtils::createObjectDB(odbDir, filename);
        EXPECT_TRUE(odb != NULL);
    }

    void removeObjectDB() {
        odb->closeAndDestroy();
        delete odb;
        odb = NULL;
        fds_uint32_t countZero = 0;
        std::atomic_store(&curObjsCopied, countZero);
        totalObjsInDb = 0;
        copiedObjsInDb = 0;
    }

    // we populate DB with metadata; we only care about fileID
    // we set (to simulate objects that are already copied, and
    // objects that need to be either garbage collected or copied
    // into a new location; Token compactor does not do real copy
    // work, that's why we don't care about ref count in this test
    void populateObjectDB(fds_uint32_t totalObjects,
                          fds_uint32_t copiedObjects,
                          fds_uint16_t diskId,
                          diskio::DataTier tier) {
        Error err(ERR_OK);
        GLOGNOTIFY << "Populating Object DB with " << totalObjects
                   << " Objects, that includes " << copiedObjects
                   << " already copied objects";
        EXPECT_LE(copiedObjects, totalObjects);
        // save this for verifying later
        totalObjsInDb = totalObjects;
        copiedObjsInDb = copiedObjects;

        std::set<ObjectID> objs;
        fds_uint64_t seed = RandNumGenerator::getRandSeed();
        RandNumGenerator rgen(seed);
        fds_uint32_t rnum = (fds_uint32_t)rgen.genNum();
        fds_uint64_t offset = 0;
        for (fds_uint32_t i = 0; i < totalObjects; ++i) {
            std::string obj_data = std::to_string(rnum);
            ObjectID oid = ObjIdGen::genObjectId(obj_data.c_str(), obj_data.size());
            // we want every object ID in the dataset to be unique
            while (objs.count(oid) > 0) {
                rnum = (fds_uint32_t)rgen.genNum();
                obj_data = std::to_string(rnum);
                oid = ObjIdGen::genObjectId(obj_data.c_str(), obj_data.size());
            }
            objs.insert(oid);
            fds_uint16_t fileId = (i < copiedObjects) ? shadowFileId : activeFileId;
            ObjMetaData::ptr meta = allocMeta(fileId, diskId, tier, offset, oid);
            GLOGNORMAL << "Generated meta " << *meta;
            ObjectBuf buf;
            meta->serializeTo(buf);
            err = odb->Put(oid, buf);
            EXPECT_TRUE(err.ok());
            offset += 4096;
        }
    }

    virtual Error enqueueMsg(fds_volid_t volId, SmIoReq* ioReq) {
        // we'll schedule a request on threadpool
        switch (ioReq->io_type) {
            case FDS_SM_SNAPSHOT_TOKEN:
                threadPool->schedule(&TestReqHandler::snapshotToken, this, ioReq);
                break;
            case FDS_SM_COMPACT_OBJECTS:
                threadPool->schedule(&TestReqHandler::compactObjects, this, ioReq);
                break;
            default:
                return ERR_INVALID_ARG;
        };
        return ERR_OK;
    }

    void snapshotToken(SmIoReq* ioReq) {
        SmIoSnapshotObjectDB *snapReq = static_cast<SmIoSnapshotObjectDB*>(ioReq);
        // actually do a snapshot of leveldb
        GLOGNORMAL << "Will Snapshot token " << snapReq->token_id;
        EXPECT_TRUE(odb != NULL);

        leveldb::ReadOptions options;
        std::shared_ptr<leveldb::DB> db = odb->GetDB();
        options.snapshot = db->GetSnapshot();
        snapReq->smio_snap_resp_cb(ERR_OK, NULL, options, db, false, 0);
    }
    void compactObjects(SmIoReq* ioReq) {
        SmIoCompactObjects *cobjs_req =  static_cast<SmIoCompactObjects*>(ioReq);
        // here we just pretend we are copying
        // for now we are always successful
        fds_uint32_t objCount = (cobjs_req->oid_list).size();
        fds_uint32_t compacted_before = std::atomic_fetch_add(&curObjsCopied,
                                                              objCount);
        GLOGNORMAL << "Compacted objects successfully";
        cobjs_req->smio_compactobj_resp_cb(ERR_OK, cobjs_req);
    }

    // this supposed to be called when compaction is done
    // to verify that number of objects copied is what expected
    void verifyCompactionWork() {
        fds_uint32_t objCopiedLastGc = std::atomic_load(&curObjsCopied);
        EXPECT_EQ(totalObjsInDb, copiedObjsInDb + objCopiedLastGc);
    }

    ObjMetaData::ptr allocMeta(fds_uint16_t fileId,
                               fds_uint16_t diskId,
                               diskio::DataTier tier,
                               fds_uint64_t offset,
                               const ObjectID& objId);

  private:
    // threadpool to schedule "qos" requests
    fds_threadpool *threadPool;

    // Persistent object metadata store which we will
    // populate to test different scenarios of removed objs vs.
    // valid objects, etc.
    osm::ObjectDB *odb;

    fds_uint32_t totalObjsInDb;
    fds_uint32_t copiedObjsInDb;
    std::atomic<fds_uint32_t> curObjsCopied;
};

// Test implementation of SmPersistStoreHandler
class TestPersistStorHandler: public SmPersistStoreHandler {
  public:
    TestPersistStorHandler() {}
    virtual ~TestPersistStorHandler() {}

    // not used by TokenCompactor
    virtual void getSmTokenStats(DiskId diskId,
                                 fds_token_id smTokId,
                                 diskio::DataTier tier,
                                 diskio::TokenStat* retStat) {
        fds_panic("should not be used by TokenCompactor");
    }

    //  Notify about start garbage collection for given token id 'tok_id'
    virtual void notifyStartGc(DiskId diskId,
                               fds_token_id smTokId,
                               diskio::DataTier tier) {
        GLOGNORMAL << "Will start GC for SM token " << smTokId
                   << " tier " << tier;
    }

    //  Notify about end of garbage collection for a given token id
    virtual Error notifyEndGc(fds_token_id smTokId,
                              diskio::DataTier tier) {
        GLOGNORMAL << "Will finish GC for SM token " << smTokId
                   << " tier " << tier;
        return ERR_OK;
    }

     // Returns true if a given location is a shadow file
    virtual fds_bool_t isShadowLocation(obj_phy_loc_t* loc,
                                        fds_token_id smTokId) {
        return (loc->obj_file_id == shadowFileId);
    }

    // dummy function
    virtual void evaluateSMTokenObjSets(const fds_token_id &smToken,
                                        const diskio::DataTier &tier,
                                        diskio::TokenStat &tokStats) {
    }
};

class SmTokenCompactorTest : public ::testing::Test {
  public:
    SmTokenCompactorTest() : compaction_done(false) {
        dataStore = new(std::nothrow) TestReqHandler();
        EXPECT_TRUE(dataStore != NULL);
        persistStore = new(std::nothrow) TestPersistStorHandler();
        EXPECT_TRUE(persistStore != NULL);
        tokenCompactor = TokenCompactorPtr(new TokenCompactor(dataStore,
                                                              persistStore));
    }

    ~SmTokenCompactorTest() {
        delete dataStore;
        delete persistStore;
    }

    virtual void SetUp() override;
    virtual void TearDown() override;

    // Callback from token compactor that compaction for a token is done
    void compactionDoneCb(fds_uint32_t token_id, const Error& error);

    TokenCompactorPtr tokenCompactor;
    TestReqHandler* dataStore;
    TestPersistStorHandler* persistStore;

    std::condition_variable done_cond;
    std::mutex cond_mutex;
    std::atomic<fds_bool_t> compaction_done;
};

void
SmTokenCompactorTest::SetUp() {
    // open empty object index DB
    dataStore->createObjectDB();
}

void
SmTokenCompactorTest::TearDown() {
    dataStore->removeObjectDB();
}

ObjMetaData::ptr
TestReqHandler::allocMeta(fds_uint16_t fileId,
                          fds_uint16_t diskId,
                          diskio::DataTier tier,
                          fds_uint64_t offset,
                          const ObjectID& objId) {
    ObjMetaData::ptr meta(new ObjMetaData());
    obj_phy_loc_t loc;
    loc.obj_stor_loc_id = diskId;
    loc.obj_file_id = fileId;
    loc.obj_stor_offset = offset;
    loc.obj_tier = tier;
    meta->initialize(objId, 4096);
    meta->updateAssocEntry(objId, fds_volid_t(37));
    meta->updatePhysLocation(&loc);
    return meta;
}

void
SmTokenCompactorTest::compactionDoneCb(fds_uint32_t token_id,
                                       const Error& error) {
    GLOGNOTIFY << "Finished compaction job!!!";
    compaction_done = true;

    // verify if we compacted all objects that we expected
    dataStore->verifyCompactionWork();

    // notify the test that compaction is finished
    done_cond.notify_all();
}

TEST_F(SmTokenCompactorTest, normal_operation) {
    // this test starts runs compaction process for different number of
    // total objects and number of objects that are already copied to
    // shadow file when compaction is started
    Error err(ERR_OK);
    fds_token_id tokId = 1;
    fds_uint16_t diskId = 2;
    diskio::DataTier tier = diskio::diskTier;
    fds_uint32_t totalObjs = 0;
    fds_uint32_t copiedObjs = 0;
    for (fds_uint32_t i = 0; i < 10; ++i) {
        GLOGNOTIFY << "Total objects " << totalObjs
                   << ", copied objects " << copiedObjs;
        dataStore->populateObjectDB(totalObjs, copiedObjs, diskId, tier);

        err = tokenCompactor->startCompaction(tokId, diskId, tier, false, std::bind(
            &SmTokenCompactorTest::compactionDoneCb, this,
            std::placeholders::_1, std::placeholders::_2));
        EXPECT_TRUE(err.ok());

        // wait on condition variable to get notified
        // when compaction is done
        std::unique_lock<std::mutex> lk(cond_mutex);
        if (done_cond.wait_for(lk, std::chrono::milliseconds(10000),
                               [this](){return atomic_load(&compaction_done);})) {
           GLOGNOTIFY << "Finished waiting on compaction done condition!";
        } else {
            // 10 seconds should be enough to compact 100 objects ;) failing the test...
            GLOGNOTIFY << "Timed out waiting on compaction done, we should have been done!";
            EXPECT_EQ(0, 1);
        }

        // prepare for the next iteration
        totalObjs = 20 + i*10;
        copiedObjs = totalObjs - i*5;
        dataStore->removeObjectDB();
        dataStore->createObjectDB();
        compaction_done = false;
    }
    GLOGNOTIFY << "Done normal operation test";
}

TEST_F(SmTokenCompactorTest, second_start) {
    Error err(ERR_OK);
    fds_token_id tokId = 1;
    fds_uint16_t diskId = 2;
    diskio::DataTier tier = diskio::diskTier;

    dataStore->populateObjectDB(1000, 0, diskId, tier);
    err = tokenCompactor->startCompaction(tokId, diskId, tier, false, std::bind(
        &SmTokenCompactorTest::compactionDoneCb, this,
        std::placeholders::_1, std::placeholders::_2));
    EXPECT_TRUE(err.ok());

    // try to start compaction again
    err = tokenCompactor->startCompaction(tokId, diskId, tier, false, std::bind(
        &SmTokenCompactorTest::compactionDoneCb, this,
        std::placeholders::_1, std::placeholders::_2));
    EXPECT_EQ(err, ERR_NOT_READY);

    // wait on condition variable to get notified
    // when compaction is done
    std::unique_lock<std::mutex> lk(cond_mutex);
    if (done_cond.wait_for(lk, std::chrono::milliseconds(20000),
                           [this](){return atomic_load(&compaction_done);})) {
        GLOGNOTIFY << "Finished waiting on compaction done condition!";
    } else {
        // 20 seconds should be enough to compact 1000 objects (copy itself is noop)
        GLOGNOTIFY << "Timed out waiting on compaction done, we should have been done!";
        EXPECT_EQ(0, 1);
    }
}

}  // namespace fds

int main(int argc, char * argv[]) {
    fds::SmTokenCompactorUtProc scavProc(argc, argv, "platform.conf",
                                         "fds.sm.", NULL);
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

