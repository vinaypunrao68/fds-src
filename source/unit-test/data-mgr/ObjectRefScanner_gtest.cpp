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

static void testPutBlobOnce(boost::shared_ptr<DMCallback> & cb, DmIoUpdateCatOnce * dmUpdCatReq) {
    dataMgr->handlers[FDS_CAT_UPD_ONCE]->handleQueueItem(dmUpdCatReq);
    cb->wait();
    EXPECT_EQ(ERR_OK, cb->e);
    taskCount.done();
}

void addVolumes() {
    for (fds_uint32_t i = 1; i < dmTester->volumes.size(); i++) {
        EXPECT_EQ(ERR_OK, dmTester->addVolume(i));
    }
}

void issuePutBlobs(const fds_volid_t &volId, uint32_t numBlobs, std::list<ObjectID> &addedObjs) {
    DEFINE_SHARED_PTR(AsyncHdr, asyncHdr);
    sequence_id_t seq_id = 0;

    // start tx
    DEFINE_SHARED_PTR(UpdateCatalogOnceMsg, putBlobOnce);

    putBlobOnce->volume_id = volId.get();
    putBlobOnce->dmt_version = 1;
    fds::UpdateBlobInfoNoData(putBlobOnce, MAX_OBJECT_SIZE, BLOB_SIZE);
    for (const auto &obj : putBlobOnce->obj_list) {
        addedObjs.push_back(ObjectID(obj.data_obj_id.digest));
    }

    taskCount.reset(numBlobs);
    uint64_t txnId;
    for (uint i = 0; i < numBlobs; i++) {
        boost::shared_ptr<DMCallback> cb(new DMCallback());
        txnId = dmTester->getNextTxnId();
        putBlobOnce->blob_name = dmTester->getBlobName(i);
        putBlobOnce->txId = txnId;

        auto dmCommitBlobOnceReq = new DmIoCommitBlobOnce<DmIoUpdateCatOnce>(volId,
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

TEST_F(DmUnitTest, ObjectRefMg) {
    std::list<ObjectID> addedObjs;

    addVolumes();
    issuePutBlobs(dmTester->TESTVOLID, NUM_BLOBS, addedObjs);
    
    /* We disable to qos because issuePutBlobs() and addVolumes() don't use qos to schedule
     * their work
     */
    dataMgr->features.setQosEnabled(true);

    concurrency::TaskStatus waiter;

    ObjectRefMgr refMgr(dmTester, dataMgr);
    refMgr.mod_startup();
    refMgr.setScanDoneCb([&waiter](ObjectRefMgr *refMgr) {
        waiter.done();
    });
    refMgr.scanOnce();

    ASSERT_TRUE(waiter.await(5000));
    refMgr.dumpStats();

    const auto dlt = dmTester->getSvcMgr()->getCurrentDLT();
    // std::sort(addedObjs.begin(), addedObjs.end());
    for (const auto &obj : addedObjs) {
        auto token = dlt->getToken(obj);
        GLOGDEBUG << "mytest calculated objid: " << obj << " token: " << token;
        auto bf = refMgr.getTokenBloomFilter(token);
        ASSERT_TRUE(bf.get() != nullptr);
        ASSERT_TRUE(bf->lookup(obj));
    }

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
