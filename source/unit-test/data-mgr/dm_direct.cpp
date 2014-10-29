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

static std::atomic<fds_uint32_t> taskCount;
fds::DMTester* dmTester = NULL;

struct DmUnitTest : ::testing::Test {
    virtual void SetUp() override {
    }

    virtual void TearDown() override {
    }
};

void startTxn(fds_volid_t volId, std::string blobName, int txnNum = 1, int blobMode = 0) {
    DMCallback cb;
    DEFINE_SHARED_PTR(AsyncHdr, asyncHdr);

    // start tx
    DEFINE_SHARED_PTR(StartBlobTxMsg, startBlbTx);

    startBlbTx->volume_id = volId;
    startBlbTx->blob_name = blobName;
    startBlbTx->txId = txnNum;
    startBlbTx->blob_mode = blobMode;
    startBlbTx->dmt_version = 1;
    auto dmBlobTxReq = new DmIoStartBlobTx(startBlbTx->volume_id,
                                           startBlbTx->blob_name,
                                           startBlbTx->blob_version,
                                           startBlbTx->blob_mode,
                                           startBlbTx->dmt_version);
    dmBlobTxReq->ioBlobTxDesc = BlobTxId::ptr(new BlobTxId(startBlbTx->txId));
    dmBlobTxReq->dmio_start_blob_tx_resp_cb = BIND_OBJ_CALLBACK(cb, DMCallback::handler, asyncHdr);
    TIMEDBLOCK("start") {
        dataMgr->startBlobTx(dmBlobTxReq);
        cb.wait();
    }
    EXPECT_EQ(ERR_OK, cb.e);
}

void commitTxn(fds_volid_t volId, std::string blobName, int txnNum = 1) {
    DMCallback cb;
    DEFINE_SHARED_PTR(AsyncHdr, asyncHdr);
    DEFINE_SHARED_PTR(CommitBlobTxMsg, commitBlbTx);
    commitBlbTx->volume_id = dmTester->TESTVOLID;
    commitBlbTx->blob_name = dmTester->TESTBLOB;
    commitBlbTx->txId = txnNum;

    auto dmBlobTxReq1 = new DmIoCommitBlobTx(commitBlbTx->volume_id,
                                             commitBlbTx->blob_name,
                                             commitBlbTx->blob_version,
                                             commitBlbTx->dmt_version);
    dmBlobTxReq1->ioBlobTxDesc = BlobTxId::ptr(new BlobTxId(commitBlbTx->txId));
    dmBlobTxReq1->dmio_commit_blob_tx_resp_cb =
            BIND_OBJ_CALLBACK(cb, DMCallback::handler, asyncHdr);

    TIMEDBLOCK("commit") {
        dataMgr->commitBlobTx(dmBlobTxReq1);
        cb.wait();
    }
    EXPECT_EQ(ERR_OK, cb.e);
}

TEST_F(DmUnitTest, AddVolume) {
    for (fds_uint32_t i = 1; i < dmTester->volumes.size(); i++) {
        EXPECT_EQ(ERR_OK, dmTester->addVolume(i));
    }
    printStats();
}


TEST_F(DmUnitTest, PutBlobOnce) {
    DEFINE_SHARED_PTR(AsyncHdr, asyncHdr);

    // start tx
    DEFINE_SHARED_PTR(UpdateCatalogOnceMsg, putBlobOnce);

    putBlobOnce->volume_id = dmTester->TESTVOLID;
    putBlobOnce->dmt_version = 1;
    TIMEDBLOCK("fill") {
        fds::UpdateBlobInfoNoData(putBlobOnce, MAX_OBJECT_SIZE, BLOB_SIZE);
    }
    uint64_t txnId;
    for (uint i = 0; i < NUM_BLOBS; i++) {
        DMCallback cb;
        txnId = dmTester->getNextTxnId();
        putBlobOnce->blob_name = dmTester->getBlobName(i);
        putBlobOnce->txId = txnId;


        auto dmCommitBlobOnceReq = new DmIoCommitBlobOnce(putBlobOnce->volume_id,
                                                          putBlobOnce->blob_name,
                                                          putBlobOnce->blob_version,
                                                          putBlobOnce->dmt_version);
        dmCommitBlobOnceReq->ioBlobTxDesc = BlobTxId::ptr(new BlobTxId(putBlobOnce->txId));
        dmCommitBlobOnceReq->dmio_commit_blob_tx_resp_cb =
                BIND_OBJ_CALLBACK(cb, DMCallback::handler, asyncHdr);


        auto dmUpdCatReq = new DmIoUpdateCatOnce(putBlobOnce, dmCommitBlobOnceReq);
        dmCommitBlobOnceReq->parent = dmUpdCatReq;

        TIMEDBLOCK("process") {
            dataMgr->updateCatalogOnce(dmUpdCatReq);
            cb.wait();
        }
        EXPECT_EQ(ERR_OK, cb.e);
    }
    printStats();
}

TEST_F(DmUnitTest, PutBlob) {
    uint64_t txnId;
    std::string blobName;
    DEFINE_SHARED_PTR(AsyncHdr, asyncHdr);

    // update
    DEFINE_SHARED_PTR(UpdateCatalogMsg, updcatMsg);
    updcatMsg->volume_id = dmTester->TESTVOLID;
    TIMEDBLOCK("fill") {
        fds::UpdateBlobInfoNoData(updcatMsg, MAX_OBJECT_SIZE, BLOB_SIZE);
    }

    for (uint i = 0; i < NUM_BLOBS; i++) {
        DMCallback cb;
        txnId = dmTester->getNextTxnId();
        blobName = dmTester->getBlobName(i);
        updcatMsg->blob_name = blobName;
        updcatMsg->txId = txnId;
        startTxn(dmTester->TESTVOLID, dmTester->TESTBLOB, txnId);
        auto dmUpdCatReq = new DmIoUpdateCat(updcatMsg);
        dmUpdCatReq->dmio_updatecat_resp_cb = BIND_OBJ_CALLBACK(cb, DMCallback::handler, asyncHdr);
        TIMEDBLOCK("process") {
            dataMgr->updateCatalog(dmUpdCatReq);
            cb.wait();
        }
        EXPECT_EQ(ERR_OK, cb.e);
        commitTxn(dmTester->TESTVOLID, blobName, txnId);
    }
    printStats();
}

TEST_F(DmUnitTest, QueryCatalog) {
    DEFINE_SHARED_PTR(AsyncHdr, asyncHdr);

    for (uint i = 0; i < NUM_BLOBS; i++) {
        DMCallback cb;
        auto qryCat = SvcMsgFactory::newQueryCatalogMsg(
            dmTester->TESTVOLID, dmTester->getBlobName(i), 0);

        auto dmQryReq = new DmIoQueryCat(qryCat);
        dmQryReq->dmio_querycat_resp_cb = BIND_OBJ_CALLBACK(cb, DMCallback::handler, asyncHdr);
        TIMEDBLOCK("process") {
            dataMgr->queryCatalogBackendSvc(dmQryReq);
            cb.wait();
        }
        EXPECT_EQ(ERR_OK, cb.e);
    }
    printStats();
}

TEST_F(DmUnitTest, SetMeta) {
    DEFINE_SHARED_PTR(AsyncHdr, asyncHdr);
    std::string blobName;
    uint64_t txnId;

    fpi::FDSP_MetaDataPair metaData;
    metaData.key = "blobType";
    metaData.value = "test Blob S3";

    for (uint i = 0; i < NUM_BLOBS; i++) {
        DMCallback cb;
        txnId = dmTester->getNextTxnId();
        blobName = dmTester->getBlobName(i);

        // start tx
        startTxn(dmTester->TESTVOLID, blobName, txnId);

        // update
        auto setBlobMeta = SvcMsgFactory::newSetBlobMetaDataMsg(dmTester->TESTVOLID,
                                                                dmTester->TESTBLOB);
        setBlobMeta->txId  = txnId;
        setBlobMeta->metaDataList.push_back(metaData);
        auto dmSetMDReq = new DmIoSetBlobMetaData(setBlobMeta);
        dmSetMDReq->dmio_setmd_resp_cb = BIND_OBJ_CALLBACK(cb, DMCallback::handler, asyncHdr);

        TIMEDBLOCK("process") {
            dataMgr->setBlobMetaDataSvc(dmSetMDReq);
            cb.wait();
        }
        EXPECT_EQ(ERR_OK, cb.e);

        // commit
        commitTxn(dmTester->TESTVOLID, blobName, txnId);
    }
    printStats();
}

TEST_F(DmUnitTest, GetMeta) {
    DEFINE_SHARED_PTR(AsyncHdr, asyncHdr);

    std::string blobName;
    for (uint i = 0; i < NUM_BLOBS; i++) {
        DMCallback cb;
        blobName = dmTester->getBlobName(i);
        auto getBlobMeta = SvcMsgFactory::newGetBlobMetaDataMsg(
            dmTester->TESTVOLID, blobName);
        auto dmReq = new DmIoGetBlobMetaData(getBlobMeta->volume_id,
                                             getBlobMeta->blob_name,
                                             getBlobMeta->blob_version,
                                             getBlobMeta);
        dmReq->dmio_getmd_resp_cb = BIND_OBJ_CALLBACK(cb, DMCallback::handler, asyncHdr);
        TIMEDBLOCK("process") {
            dataMgr->scheduleGetBlobMetaDataSvc(dmReq);
            cb.wait();
        }
        EXPECT_EQ(ERR_OK, cb.e);
    }
    printStats();
}

TEST_F(DmUnitTest, GetDMStats) {
    DMCallback cb;
    DEFINE_SHARED_PTR(AsyncHdr, asyncHdr);

    auto getDmStats = SvcMsgFactory::newGetDmStatsMsg(dmTester->TESTVOLID);
    auto dmRequest = new DmIoGetSysStats(getDmStats);

    dmRequest->cb = BIND_OBJ_CALLBACK(cb, DMCallback::handler, asyncHdr);
    TIMEDBLOCK("process") {
        dataMgr->handlers[FDS_DM_SYS_STATS]->handleQueueItem(dmRequest);
        cb.wait();
    }
    EXPECT_EQ(ERR_OK, cb.e);
    printStats();
}

TEST_F(DmUnitTest, GetBucket) {
    DMCallback cb;
    DEFINE_SHARED_PTR(AsyncHdr, asyncHdr);

    auto message = SvcMsgFactory::newGetBucketMsg(dmTester->TESTVOLID, 0);
    auto dmRequest = new DmIoGetBucket(message);

    dmRequest->cb = BIND_OBJ_CALLBACK(cb, DMCallback::handler, asyncHdr);
    TIMEDBLOCK("process") {
        dataMgr->handlers[FDS_LIST_BLOB]->handleQueueItem(dmRequest);
        cb.wait();
    }
    EXPECT_EQ(ERR_OK, cb.e);
    printStats();
}

TEST_F(DmUnitTest, DeleteBlob) {
    DEFINE_SHARED_PTR(AsyncHdr, asyncHdr);

    std::string blobName;
    uint64_t txnId;

    for (uint i = 0; i < NUM_BLOBS; i++) {
        DMCallback cb;
        txnId = dmTester->getNextTxnId();
        blobName = dmTester->getBlobName(i);

        // start tx
        startTxn(dmTester->TESTVOLID, blobName, txnId);

        // delete
        auto message = SvcMsgFactory::newDeleteBlobMsg(dmTester->TESTVOLID, blobName);
        message->txId = txnId;
        auto dmRequest = new DmIoDeleteBlob(message);
        dmRequest->cb = BIND_OBJ_CALLBACK(cb, DMCallback::handler, asyncHdr);
        TIMEDBLOCK("process") {
            dataMgr->handlers[FDS_DELETE_BLOB]->handleQueueItem(dmRequest);
            cb.wait();
        }
        EXPECT_EQ(ERR_OK, cb.e);

        // commit
        commitTxn(dmTester->TESTVOLID, blobName, txnId);
    }
    printStats();
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
    // setup default volumes
    generateVolumes(dmTester->volumes);
    dmTester->addVolume(0);
    dmTester->TESTVOLID = dmTester->volumes[0]->volUUID;
    dmTester->TESTBLOB = "testblob";

    int retCode = RUN_ALL_TESTS();
    dmTester->stop();
    return retCode;
}
