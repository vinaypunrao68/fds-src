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

    Error addVolume(uint num) {
        const auto& volumes = dmTester->volumes;
        Error err(ERR_OK);
        std::cout << "adding volume: " << volumes[num]->volUUID
                  << ":" << volumes[num]->name
                  << ":" << dataMgr->getPrefix()
                  << std::endl;
        return dataMgr->_process_add_vol(dataMgr->getPrefix() +
                                        std::to_string(volumes[num]->volUUID),
                                        volumes[num]->volUUID, volumes[num].get(),
                                        false);
    }
};

TEST_F(DmUnitTest, AddVolume) {
    for (fds_uint32_t i = 0; i < dmTester->volumes.size(); i++) {
        EXPECT_EQ(ERR_OK, addVolume(i));
    }
}

TEST_F(DmUnitTest, PutBlob) {
    DMCallback cb;
    DEFINE_SHARED_PTR(AsyncHdr, asyncHdr);

    // start tx
    DEFINE_SHARED_PTR(StartBlobTxMsg, startBlbTx);

    startBlbTx->volume_id = dmTester->volumes[0]->volUUID;
    startBlbTx->blob_name = "testblob";
    startBlbTx->txId = 1;
    startBlbTx->dmt_version = 1;
    auto dmBlobTxReq = new DmIoStartBlobTx(startBlbTx->volume_id,
                                           startBlbTx->blob_name,
                                           startBlbTx->blob_version,
                                           startBlbTx->blob_mode,
                                           startBlbTx->dmt_version);
    dmBlobTxReq->ioBlobTxDesc = BlobTxId::ptr(new BlobTxId(startBlbTx->txId));
    dmBlobTxReq->dmio_start_blob_tx_resp_cb = BIND_OBJ_CALLBACK(cb, DMCallback::handler, asyncHdr);
    dataMgr->startBlobTx(dmBlobTxReq);
    cb.wait();
    EXPECT_EQ(ERR_OK, cb.e);

    // update
    DEFINE_SHARED_PTR(UpdateCatalogMsg, updcatMsg);
    updcatMsg->volume_id = dmTester->volumes[0]->volUUID;
    updcatMsg->blob_name = "testblob";
    updcatMsg->txId = 1;
    updcatMsg->obj_list;
    fds::UpdateBlobInfoNoData(updcatMsg, MAX_OBJECT_SIZE, BLOB_SIZE);

    auto dmUpdCatReq = new DmIoUpdateCat(updcatMsg);
    cb.reset();
    dmUpdCatReq->dmio_updatecat_resp_cb = BIND_OBJ_CALLBACK(cb, DMCallback::handler, asyncHdr);
    dataMgr->updateCatalog(dmUpdCatReq);
    cb.wait();
    EXPECT_EQ(ERR_OK, cb.e);

    // commit
    DEFINE_SHARED_PTR(CommitBlobTxMsg, commitBlbTx);
    commitBlbTx->volume_id = dmTester->volumes[0]->volUUID;
    commitBlbTx->blob_name = "testblob";
    commitBlbTx->txId = 1;

    auto dmBlobTxReq1 = new DmIoCommitBlobTx(commitBlbTx->volume_id,
                                            commitBlbTx->blob_name,
                                            commitBlbTx->blob_version,
                                            commitBlbTx->dmt_version);
    cb.reset();
    dmBlobTxReq1->dmio_commit_blob_tx_resp_cb =
            BIND_OBJ_CALLBACK(cb, DMCallback::handler, asyncHdr);

    dmBlobTxReq1->ioBlobTxDesc = dmBlobTxReq->ioBlobTxDesc;
    dataMgr->commitBlobTx(dmBlobTxReq1);
    cb.wait();
    EXPECT_EQ(ERR_OK, cb.e);
}


int main(int argc, char** argv) {
    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    ::testing::InitGoogleMock(&argc, argv);

    dmTester = new fds::DMTester(argc, argv);

    // process command line options
    po::options_description desc("\nDM test Command line options");
    desc.add_options()
            ("help,h"       , "help/ usage message")  // NOLINT
            ("num-volumes,v", po::value<fds_uint32_t>(&NUM_VOLUMES)->default_value(NUM_VOLUMES)        , "number of volumes")  // NOLINT
            ("obj-size,o"   , po::value<fds_uint32_t>(&MAX_OBJECT_SIZE)->default_value(MAX_OBJECT_SIZE), "max object size in bytes")  // NOLINT
            ("blob-size,b"  , po::value<fds_uint64_t>(&BLOB_SIZE)->default_value(BLOB_SIZE)            , "blob size in bytes")  // NOLINT
            ("num-blobs,n"  , po::value<fds_uint32_t>(&NUM_BLOBS)->default_value(NUM_BLOBS)            , "number of blobs")  // NOLINT
            ("puts-only"    , "do put operations only")
            ("no-delete"    , "do put & get operations only");

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }

    dmTester->start();
    generateVolumes(dmTester->volumes);
    int retCode = RUN_ALL_TESTS();
    dmTester->stop();
    return retCode;
}
