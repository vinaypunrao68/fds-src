/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include "./dm_mocks.h"
#include "./dm_gtest.h"
#include "./dm_utils.h"
#include <ObjectRefScanner.h>

#include <testlib/SvcMsgFactory.h>
#include <vector>
#include <string>
#include <thread>
#include <google/profiler.h>
#include <iostream>
fds::DMTester* dmTester = NULL;
fds::concurrency::TaskStatus taskCount(0);

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

    startBlbTx->volume_id = volId.get();
    startBlbTx->blob_name = blobName;
    startBlbTx->txId = txnNum;
    startBlbTx->blob_mode = blobMode;
    startBlbTx->dmt_version = 1;
    auto dmBlobTxReq = new DmIoStartBlobTx(volId,
                                           startBlbTx->blob_name,
                                           startBlbTx->blob_version,
                                           startBlbTx->blob_mode,
                                           startBlbTx->dmt_version);
    dmBlobTxReq->ioBlobTxDesc = BlobTxId::ptr(new BlobTxId(startBlbTx->txId));
    dmBlobTxReq->cb = BIND_OBJ_CALLBACK(cb, DMCallback::handler, asyncHdr);
    TIMEDBLOCK("start") {
        dataMgr->handlers[FDS_START_BLOB_TX]->handleQueueItem(dmBlobTxReq);
        cb.wait();
    }
    EXPECT_EQ(ERR_OK, cb.e);
}

void commitTxn(fds_volid_t volId, std::string blobName, int txnNum = 1) {
    DMCallback cb;
    static sequence_id_t seq_id = 0;
    DEFINE_SHARED_PTR(AsyncHdr, asyncHdr);
    DEFINE_SHARED_PTR(CommitBlobTxMsg, commitBlbTx);
    commitBlbTx->volume_id = dmTester->TESTVOLID.get();
    commitBlbTx->blob_name = blobName;
    commitBlbTx->txId = txnNum;

    auto dmBlobTxReq1 = new DmIoCommitBlobTx(dmTester->TESTVOLID,
                                             commitBlbTx->blob_name,
                                             commitBlbTx->blob_version,
                                             commitBlbTx->dmt_version,
                                             ++seq_id);
    dmBlobTxReq1->ioBlobTxDesc = BlobTxId::ptr(new BlobTxId(commitBlbTx->txId));
    dmBlobTxReq1->cb =
            BIND_OBJ_CALLBACK(cb, DMCallback::handler, asyncHdr);

    TIMEDBLOCK("commit") {
        dataMgr->handlers[FDS_COMMIT_BLOB_TX]->handleQueueItem(dmBlobTxReq1);
        cb.wait();
    }
    EXPECT_EQ(ERR_OK, cb.e);
}

static void testPutBlobOnce(boost::shared_ptr<DMCallback> & cb, DmIoUpdateCatOnce * dmUpdCatReq) {
    TIMEDBLOCK("process") {
        dataMgr->handlers[FDS_CAT_UPD_ONCE]->handleQueueItem(dmUpdCatReq);
        cb->wait();
    }
    EXPECT_EQ(ERR_OK, cb->e);
    taskCount.done();
}

TEST_F(DmUnitTest, AddVolume) {
    for (fds_uint32_t i = 1; i < dmTester->volumes.size(); i++) {
        EXPECT_EQ(ERR_OK, dmTester->addVolume(i));
    }
    printStats();
}

TEST_F(DmUnitTest, PutBlobOnce) {
    DEFINE_SHARED_PTR(AsyncHdr, asyncHdr);
    sequence_id_t seq_id = 0;

    if (profile)
        ProfilerStart("/tmp/dm_direct.prof");
    // start tx
    DEFINE_SHARED_PTR(UpdateCatalogOnceMsg, putBlobOnce);

    putBlobOnce->volume_id = dmTester->TESTVOLID.get();
    putBlobOnce->dmt_version = 1;
    TIMEDBLOCK("fill") {
        fds::UpdateBlobInfoNoData(putBlobOnce, MAX_OBJECT_SIZE, BLOB_SIZE);
    }

    TIMEDBLOCK("total putBlobOnce") {
        taskCount.reset(NUM_BLOBS);
        uint64_t txnId;
        for (uint i = 0; i < NUM_BLOBS; i++) {
            boost::shared_ptr<DMCallback> cb(new DMCallback());
            txnId = dmTester->getNextTxnId();
            putBlobOnce->blob_name = dmTester->getBlobName(i);
            putBlobOnce->txId = txnId;


            auto dmCommitBlobOnceReq = new DmIoCommitBlobOnce<DmIoUpdateCatOnce>(dmTester->TESTVOLID,
                                                              putBlobOnce->blob_name,
                                                              putBlobOnce->blob_version,
                                                              putBlobOnce->dmt_version,
                                                              ++seq_id);
            dmCommitBlobOnceReq->ioBlobTxDesc = BlobTxId::ptr(new BlobTxId(putBlobOnce->txId));
            dmCommitBlobOnceReq->cb =
                    BIND_OBJ_CALLBACK(*cb.get(), DMCallback::handler, asyncHdr);


            auto dmUpdCatReq = new DmIoUpdateCatOnce(putBlobOnce, dmCommitBlobOnceReq);
            dmCommitBlobOnceReq->parent = dmUpdCatReq;

            g_fdsprocess->proc_thrpool()->schedule(&testPutBlobOnce, cb, dmUpdCatReq);
        }
        taskCount.await();
    }

    if (profile)
        ProfilerStop();
    printStats();
}

TEST_F(DmUnitTest, PutBlob) {
    uint64_t txnId;
    std::string blobName;
    DEFINE_SHARED_PTR(AsyncHdr, asyncHdr);

    // update
    DEFINE_SHARED_PTR(UpdateCatalogMsg, updcatMsg);
    updcatMsg->volume_id = dmTester->TESTVOLID.get();
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
        // FIXME(DAC): Memory leak.
        auto dmUpdCatReq = new DmIoUpdateCat(updcatMsg);
        dmUpdCatReq->cb = BIND_OBJ_CALLBACK(cb, DMCallback::handler, asyncHdr);
        TIMEDBLOCK("process") {
            dataMgr->handlers[FDS_CAT_UPD]->handleQueueItem(dmUpdCatReq);
            cb.wait();
        }
        EXPECT_EQ(ERR_OK, cb.e);
        commitTxn(dmTester->TESTVOLID, blobName, txnId);
    }
    printStats();
}

TEST_F(DmUnitTest, ObjectRefMg) {
    dataMgr->features.setQosEnabled(true);

    concurrency::TaskStatus waiter;

    ObjectRefMgr refMgr(dmTester, dataMgr);
    refMgr.mod_startup();
    refMgr.scanOnce([&waiter](ObjectRefMgr *refMgr) {
        waiter.done();
    });

    EXPECT_TRUE(waiter.await(5000));
    refMgr.dumpStats();

    dataMgr->features.setQosEnabled(false);
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
    // setup default volumes
    generateVolumes(dmTester->volumes);
    dmTester->addVolume(0);
    dmTester->TESTVOLID = dmTester->volumes[0]->volUUID;
    dmTester->TESTBLOB = "testblob";

    int retCode = RUN_ALL_TESTS();
    dmTester->stop();
    dmTester->clean();
    return retCode;
}
