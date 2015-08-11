/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include "./dm_mocks.h"
#include "./dm_gtest.h"
#include "./dm_utils.h"

#include <testlib/SvcMsgFactory.h>
#include <vector>
#include <string>
#include <thread>
#include <google/profiler.h>

fds::DMTester* dmTester = NULL;
fds::concurrency::TaskStatus taskCount(0);
static sequence_id_t _global_seq_id;

struct SeqIdTest : ::testing::Test {
    virtual void SetUp() override {
        // XXX: kinda assumes one volume per test :(
        _global_seq_id = 0;
        static int i = 0;
        dmTester->addVolume(i);
        dmTester->TESTVOLID = dmTester->volumes[i]->volUUID;

        ++i;
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
    DEFINE_SHARED_PTR(AsyncHdr, asyncHdr);
    DEFINE_SHARED_PTR(CommitBlobTxMsg, commitBlbTx);
    commitBlbTx->volume_id = dmTester->TESTVOLID.get();
    commitBlbTx->blob_name = blobName;
    commitBlbTx->txId = txnNum;

    auto dmBlobTxReq1 = new DmIoCommitBlobTx(dmTester->TESTVOLID,
                                             commitBlbTx->blob_name,
                                             commitBlbTx->blob_version,
                                             commitBlbTx->dmt_version,
                                             ++_global_seq_id);
    dmBlobTxReq1->ioBlobTxDesc = BlobTxId::ptr(new BlobTxId(commitBlbTx->txId));
    dmBlobTxReq1->cb =
            BIND_OBJ_CALLBACK(cb, DMCallback::handler, asyncHdr);

    TIMEDBLOCK("commit") {
        dataMgr->handlers[FDS_COMMIT_BLOB_TX]->handleQueueItem(dmBlobTxReq1);
        cb.wait();
    }
    EXPECT_EQ(ERR_OK, cb.e);
}

void putVolMeta() {
    DMCallback cb;
    DEFINE_SHARED_PTR(AsyncHdr, asyncHdr);
    DEFINE_SHARED_PTR(SetVolumeMetadataMsg, setVolMeta);

    // Set/get/check some metadata for the volume
    fpi::FDSP_MetaDataPair metadataPair;
    metadataPair.key = "Volume Name";
    metadataPair.value = dmTester->volumes.back()->name;
    fpi::FDSP_MetaDataList setMetadataList;
    setMetadataList.push_back(metadataPair);

    setVolMeta->volumeId = dmTester->TESTVOLID.get();
    setVolMeta->sequence_id = ++_global_seq_id;
    setVolMeta->metadataList = setMetadataList;

    auto dmReq = new DmIoSetVolumeMetaData(setVolMeta);

    dmReq->cb = BIND_OBJ_CALLBACK(cb, DMCallback::handler, asyncHdr);

    TIMEDBLOCK("set vol meta") {
        dataMgr->handlers[FDS_SET_VOLUME_METADATA]->handleQueueItem(dmReq);
        cb.wait();
    }
    EXPECT_EQ(ERR_OK, cb.e);
}

void putBlob(){
    uint64_t txnId;
    std::string blobName;
    DEFINE_SHARED_PTR(AsyncHdr, asyncHdr);

    for (uint i = 0; i < NUM_BLOBS; i++) {
        // update
        DEFINE_SHARED_PTR(UpdateCatalogMsg, updcatMsg);
        updcatMsg->volume_id = dmTester->TESTVOLID.get();

        DMCallback cb;
        txnId = dmTester->getNextTxnId();
        blobName = dmTester->getBlobName(i);
        updcatMsg->blob_name = blobName;
        updcatMsg->txId = txnId;

        fds::UpdateBlobInfoNoData(updcatMsg, MAX_OBJECT_SIZE, BLOB_SIZE);

        startTxn(dmTester->TESTVOLID, blobName, txnId);
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
}

static void doPutBlobOnce(boost::shared_ptr<DMCallback> & cb, DmIoUpdateCatOnce * dmUpdCatReq) {
    TIMEDBLOCK("process") {
        dataMgr->handlers[FDS_CAT_UPD_ONCE]->handleQueueItem(dmUpdCatReq);
        cb->wait();
    }
    EXPECT_EQ(ERR_OK, cb->e);
    taskCount.done();
}

void putBlobOnce(){
    uint64_t txnId;
    std::string blobName;
    DEFINE_SHARED_PTR(AsyncHdr, asyncHdr);

    taskCount.reset(NUM_BLOBS);

    for (uint i = 0; i < NUM_BLOBS; i++) {
        // update
        DEFINE_SHARED_PTR(UpdateCatalogOnceMsg, updcatMsg);
        updcatMsg->volume_id = dmTester->TESTVOLID.get();
        updcatMsg->dmt_version = 1;

        boost::shared_ptr<DMCallback> cb(new DMCallback());

        txnId = dmTester->getNextTxnId();
        blobName = dmTester->getBlobName(i);
        updcatMsg->blob_name = blobName;
        updcatMsg->txId = txnId;
        updcatMsg->sequence_id = ++_global_seq_id;

        fds::UpdateBlobInfoNoData(updcatMsg, MAX_OBJECT_SIZE, BLOB_SIZE);

        auto dmCommitBlobOnceReq = new DmIoCommitBlobOnce<DmIoUpdateCatOnce>(dmTester->TESTVOLID,
                                                          updcatMsg->blob_name,
                                                          updcatMsg->blob_version,
                                                          updcatMsg->dmt_version,
                                                          _global_seq_id);
        dmCommitBlobOnceReq->ioBlobTxDesc = BlobTxId::ptr(new BlobTxId(updcatMsg->txId));
        dmCommitBlobOnceReq->cb =
            BIND_OBJ_CALLBACK(*cb.get(), DMCallback::handler, asyncHdr);

        auto dmUpdCatReq = new DmIoUpdateCatOnce(updcatMsg, dmCommitBlobOnceReq);
        dmCommitBlobOnceReq->parent = dmUpdCatReq;

        g_fdsprocess->proc_thrpool()->schedule(&doPutBlobOnce, cb, dmUpdCatReq);
    }
    taskCount.await();
}

TEST_F(SeqIdTest, SequenceIncrementPutOnce){
    sequence_id_t tmp_seq_id;

    MAX_OBJECT_SIZE = 2 * 1024 * 1024;    // 2MB
    BLOB_SIZE = 4 * 1024 * 1024;   // 4 MB
    NUM_BLOBS = 1024;

    TIMEDBLOCK("Scan For Max SeqId - Empty") {
        dataMgr->timeVolCat_->queryIface()->getVolumeSequenceId(dmTester->TESTVOLID, tmp_seq_id);
        EXPECT_EQ(0, tmp_seq_id);
    }

    putBlobOnce();

    TIMEDBLOCK("Scan For Max SeqId - Confirmation") {
        dataMgr->timeVolCat_->queryIface()->getVolumeSequenceId(dmTester->TESTVOLID, tmp_seq_id);
        EXPECT_EQ(NUM_BLOBS, tmp_seq_id);
    }

    printStats();
}

TEST_F(SeqIdTest, SequenceIncrementCommitTx){
    sequence_id_t tmp_seq_id;

    MAX_OBJECT_SIZE = 2 * 1024 * 1024;    // 2MB
    BLOB_SIZE = 4 * 1024 * 1024;   // 4 MB
    NUM_BLOBS = 1024;

    TIMEDBLOCK("Scan For Max SeqId - Empty") {
        dataMgr->timeVolCat_->queryIface()->getVolumeSequenceId(dmTester->TESTVOLID, tmp_seq_id);
        EXPECT_EQ(0, tmp_seq_id);
    }

    putBlob();

    TIMEDBLOCK("Scan For Max SeqId - Confirmation") {
        dataMgr->timeVolCat_->queryIface()->getVolumeSequenceId(dmTester->TESTVOLID, tmp_seq_id);
        EXPECT_EQ(NUM_BLOBS, tmp_seq_id);
    }

    printStats();
}

TEST_F(SeqIdTest, SequenceIncrementVolMetaLast){
    sequence_id_t tmp_seq_id;

    MAX_OBJECT_SIZE = 2 * 1024 * 1024;    // 2MB
    BLOB_SIZE = 4 * 1024 * 1024;   // 4 MB
    NUM_BLOBS = 1024;

    TIMEDBLOCK("Scan For Max SeqId - Empty") {
        dataMgr->timeVolCat_->queryIface()->getVolumeSequenceId(dmTester->TESTVOLID, tmp_seq_id);
        EXPECT_EQ(0, tmp_seq_id);
    }

    putBlobOnce();

    putVolMeta();

    TIMEDBLOCK("Scan For Max SeqId - Confirmation") {
        dataMgr->timeVolCat_->queryIface()->getVolumeSequenceId(dmTester->TESTVOLID, tmp_seq_id);
        EXPECT_EQ(NUM_BLOBS+1, tmp_seq_id);
    }

    printStats();
}

TEST_F(SeqIdTest, SequenceIncrementVolMetaFirst){
    sequence_id_t tmp_seq_id;

    MAX_OBJECT_SIZE = 2 * 1024 * 1024;    // 2MB
    BLOB_SIZE = 4 * 1024 * 1024;   // 4 MB
    NUM_BLOBS = 1024;

    TIMEDBLOCK("Scan For Max SeqId - Empty") {
        dataMgr->timeVolCat_->queryIface()->getVolumeSequenceId(dmTester->TESTVOLID, tmp_seq_id);
        EXPECT_EQ(0, tmp_seq_id);
    }

    putVolMeta();

    putBlobOnce();

    TIMEDBLOCK("Scan For Max SeqId - Confirmation") {
        dataMgr->timeVolCat_->queryIface()->getVolumeSequenceId(dmTester->TESTVOLID, tmp_seq_id);
        EXPECT_EQ(NUM_BLOBS+1, tmp_seq_id);
    }

    printStats();
}

TEST_F(SeqIdTest, BlobDiffIdentical){
    MAX_OBJECT_SIZE = 2 * 1024 * 1024;    // 2MB
    BLOB_SIZE = 4 * 1024 * 1024;   // 4 MB
    NUM_BLOBS = 1024;

    auto volId1 =  dmTester->TESTVOLID;

    putBlobOnce();

    SetUp();
    auto volId2 =  dmTester->TESTVOLID;

    putBlobOnce();

    auto update_list = std::vector<std::string>();
    auto delete_list = std::vector<std::string>();

    auto dest = std::map<std::string, long int>();
    auto source = std::map<std::string, long int>();

    dataMgr->timeVolCat_->queryIface()->getAllBlobsWithSequenceId(volId1, source);
    dataMgr->timeVolCat_->queryIface()->getAllBlobsWithSequenceId(volId2, dest);

    TIMEDBLOCK("diff") {
        Error err = DmMigrationClient::diffBlobLists(dest, source, update_list, delete_list);
    }

    EXPECT_EQ(update_list.size(), 0);
    EXPECT_EQ(delete_list.size(), 0);

    printStats();
}

TEST_F(SeqIdTest, BlobDiffUpdate){
    MAX_OBJECT_SIZE = 2 * 1024 * 1024;    // 2MB
    BLOB_SIZE = 4 * 1024 * 1024;   // 4 MB
    NUM_BLOBS = 1024;

    auto volId1 =  dmTester->TESTVOLID;
    putBlobOnce();

    SetUp();
    --NUM_BLOBS;

    auto volId2 =  dmTester->TESTVOLID;
    putBlobOnce();

    auto update_list = std::vector<std::string>();
    auto delete_list = std::vector<std::string>();

    auto dest = std::map<std::string, long int>();
    auto source = std::map<std::string, long int>();

    dataMgr->timeVolCat_->queryIface()->getAllBlobsWithSequenceId(volId1, source);
    dataMgr->timeVolCat_->queryIface()->getAllBlobsWithSequenceId(volId2, dest);

    TIMEDBLOCK("diff") {
        Error err = DmMigrationClient::diffBlobLists(dest, source, update_list, delete_list);
    }

    EXPECT_EQ(update_list.size(), 1);
    EXPECT_EQ(delete_list.size(), 0);

    printStats();
}

TEST_F(SeqIdTest, BlobDiffDelete){
    MAX_OBJECT_SIZE = 2 * 1024 * 1024;    // 2MB
    BLOB_SIZE = 4 * 1024 * 1024;   // 4 MB
    NUM_BLOBS = 1024;

    auto volId1 =  dmTester->TESTVOLID;
    putBlobOnce();

    SetUp();
    ++NUM_BLOBS;

    auto volId2 =  dmTester->TESTVOLID;
    putBlobOnce();

    auto update_list = std::vector<std::string>();
    auto delete_list = std::vector<std::string>();

    auto dest = std::map<std::string, long int>();
    auto source = std::map<std::string, long int>();

    dataMgr->timeVolCat_->queryIface()->getAllBlobsWithSequenceId(volId1, source);
    dataMgr->timeVolCat_->queryIface()->getAllBlobsWithSequenceId(volId2, dest);

    TIMEDBLOCK("diff") {
        Error err = DmMigrationClient::diffBlobLists(dest, source, update_list, delete_list);
    }

    EXPECT_EQ(update_list.size(), 0);
    EXPECT_EQ(delete_list.size(), 1);

    printStats();
}

TEST_F(SeqIdTest, BlobDiffOverwrite){
    MAX_OBJECT_SIZE = 2 * 1024 * 1024;    // 2MB
    BLOB_SIZE = 4 * 1024 * 1024;   // 4 MB
    NUM_BLOBS = 1024;

    auto volId1 =  dmTester->TESTVOLID;

    putBlobOnce();

    SetUp();
    auto volId2 =  dmTester->TESTVOLID;

    putBlobOnce();
    putBlobOnce();

    auto update_list = std::vector<std::string>();
    auto delete_list = std::vector<std::string>();

    auto dest = std::map<std::string, int64_t>();
    auto source = std::map<std::string, int64_t>();

    dataMgr->timeVolCat_->queryIface()->getAllBlobsWithSequenceId(volId1, source);
    dataMgr->timeVolCat_->queryIface()->getAllBlobsWithSequenceId(volId2, dest);

    TIMEDBLOCK("diff") {
        Error err = DmMigrationClient::diffBlobLists(dest, source, update_list, delete_list);
    }

    EXPECT_EQ(update_list.size(), NUM_BLOBS);
    EXPECT_EQ(delete_list.size(), 0);

    printStats();
}

TEST_F(SeqIdTest, MigrateVolDesc){
    MAX_OBJECT_SIZE = 2 * 1024 * 1024;    // 2MB
    BLOB_SIZE = 4 * 1024 * 1024;   // 4 MB
    NUM_BLOBS = 1024;
    sequence_id_t tmp_seq_id;
    auto volId =  dmTester->TESTVOLID;
    putVolMeta();
    putBlobOnce();


    // confirm volume descriptor contains the expected metadata
    fpi::FDSP_MetaDataList metadataList;
    Error err = dataMgr->timeVolCat_->queryIface()->getVolumeMetadata(volId, metadataList);
    EXPECT_EQ(true, err.ok());

    bool foundSomething = false;

    for (auto it : metadataList) {
        if (it.key == "Volume Name") {
            EXPECT_EQ(dmTester->volumes.back()->name, it.value);
            foundSomething = true;
        }
    }

    EXPECT_EQ(true, foundSomething);


    // use the interface under test to update the volume descriptor
    VolumeMetaDesc volDesc(metadataList, ++_global_seq_id);

    std::string blobName{""}, data;

    err = volDesc.getSerialized(data);
    EXPECT_EQ(true, err.ok());

    err = dataMgr->timeVolCat_->migrateDescriptor(volId, blobName, data);
    EXPECT_EQ(true, err.ok());

    // confirm seqeunce id changed
    err = dataMgr->timeVolCat_->queryIface()->getVolumeSequenceId(volId, tmp_seq_id);
    EXPECT_EQ(true, err.ok());
    EXPECT_EQ(NUM_BLOBS+2, tmp_seq_id);

    // get the new metadata list
    fpi::FDSP_MetaDataList metadataList2;
    err = dataMgr->timeVolCat_->queryIface()->getVolumeMetadata(volId, metadataList2);
    EXPECT_EQ(true, err.ok());

    // confirm old metadata list and new metadata list match
    EXPECT_EQ(metadataList, metadataList2);

    // create mutated metadata list
    fpi::FDSP_MetaDataList metadataList3{metadataList};

    fpi::FDSP_MetaDataPair metadataPair;
    metadataPair.key = "avocado";
    metadataPair.value = "delicious";
    metadataList3.push_back(metadataPair);

    foundSomething = false;

    // ensure mutated ist contains old metadata
    for (auto it : metadataList3) {
        if (it.key == "Volume Name") {
            EXPECT_EQ(dmTester->volumes.back()->name, it.value);
            foundSomething = true;
        }
    }

    EXPECT_EQ(true, foundSomething);

    foundSomething = false;

    // ensure mutated list contains mutated data
    for (auto it : metadataList3) {
        if (it.key == "avocado") {
            EXPECT_EQ("delicious", it.value);
            foundSomething = true;
        }
    }

    EXPECT_EQ(true, foundSomething);


    // write a new volume descriptor with the mutated metadata list
    VolumeMetaDesc volDesc2{metadataList3, ++_global_seq_id};

    std::string data2;
    err = volDesc2.getSerialized(data2);
    EXPECT_EQ(true, err.ok());

    err = dataMgr->timeVolCat_->migrateDescriptor(volId, blobName, data2);
    EXPECT_EQ(true, err.ok());

    // read the descriptor and confirm both old and new metadata are there
    err = dataMgr->timeVolCat_->queryIface()->getVolumeMetadata(volId, metadataList2);
    EXPECT_EQ(true, err.ok());

    foundSomething = false;

    for (auto it : metadataList2) {
        if (it.key == "Volume Name") {
            EXPECT_EQ(dmTester->volumes.back()->name, it.value);
            foundSomething = true;
        }
    }

    EXPECT_EQ(true, foundSomething);

    foundSomething = false;

    for (auto it : metadataList2) {
        if (it.key == "avocado") {
            EXPECT_EQ("delicious", it.value);
            foundSomething = true;
        }
    }

    EXPECT_EQ(true, foundSomething);
}

TEST_F(SeqIdTest, MigrateBlobDelete){
    MAX_OBJECT_SIZE = 2 * 1024 * 1024;    // 2MB
    BLOB_SIZE = 4 * 1024 * 1024;   // 4 MB
    NUM_BLOBS = 1024;

    auto volId =  dmTester->TESTVOLID;

    putBlobOnce();

    // confirm the sequence id of the volume is as expected
    sequence_id_t tmp_seq_id;
    Error err = dataMgr->timeVolCat_->queryIface()->getVolumeSequenceId(volId, tmp_seq_id);
    EXPECT_EQ(true, err.ok());
    EXPECT_EQ(NUM_BLOBS, tmp_seq_id);

    std::string data{""};

    // delete the blob with the largest sequence id and use get seq id to confirm sequence id has dropped
    err = dataMgr->timeVolCat_->migrateDescriptor(volId, dmTester->getBlobName(NUM_BLOBS-1),
                                                  data);
    EXPECT_EQ(true, err.ok());

    err = dataMgr->timeVolCat_->queryIface()->getVolumeSequenceId(volId, tmp_seq_id);
    EXPECT_EQ(true, err.ok());
    EXPECT_EQ(NUM_BLOBS-1, tmp_seq_id);
}

TEST_F(SeqIdTest, MigrateBlobPutNew){
    MAX_OBJECT_SIZE = 2 * 1024 * 1024;    // 2MB
    BLOB_SIZE = 4 * 1024 * 1024;   // 4 MB
    NUM_BLOBS = 1024;

    auto volId =  dmTester->TESTVOLID;

    putBlobOnce();

    // confirm we wrote the blob descriptors
    sequence_id_t tmp_seq_id;
    Error err = dataMgr->timeVolCat_->queryIface()->getVolumeSequenceId(volId, tmp_seq_id);
    EXPECT_EQ(true, err.ok());
    EXPECT_EQ(NUM_BLOBS, tmp_seq_id);


    // fake up a new blob and write it
    BlobMetaDesc blobDesc;
    blobDesc.desc.blob_name = dmTester->getBlobName(NUM_BLOBS);
    blobDesc.desc.version = 1;
    blobDesc.desc.sequence_id = ++_global_seq_id;
    blobDesc.desc.blob_size = 0;

    std::string blobString;
    err= blobDesc.getSerialized(blobString);
    EXPECT_EQ(true, err.ok());

    err = dataMgr->timeVolCat_->migrateDescriptor(volId, blobDesc.desc.blob_name,
                                                        blobString);
    EXPECT_EQ(true, err.ok());

    // check that the new blob's sequence id is returned
    err= dataMgr->timeVolCat_->queryIface()->getVolumeSequenceId(volId, tmp_seq_id);
    EXPECT_EQ(true, err.ok());
    EXPECT_EQ(NUM_BLOBS+1, tmp_seq_id);
}

TEST_F(SeqIdTest, MigrateBlobPutGrow){
    MAX_OBJECT_SIZE = 2 * 1024 * 1024;    // 2MB
    BLOB_SIZE = 4 * 1024 * 1024;   // 4 MB
    NUM_BLOBS = 1024;

    auto volId =  dmTester->TESTVOLID;

    putBlobOnce();

    // confirm sequence id
    sequence_id_t tmp_seq_id;
    Error err = dataMgr->timeVolCat_->queryIface()->getVolumeSequenceId(volId, tmp_seq_id);
    EXPECT_EQ(true, err.ok());
    EXPECT_EQ(NUM_BLOBS, tmp_seq_id);

    // make a new blob descriptor for the last blob and write it
    BlobMetaDesc blobDesc;
    std::string blobName = dmTester->getBlobName(NUM_BLOBS-1);

    blobDesc.desc.blob_name = blobName;
    blobDesc.desc.version = 2;
    blobDesc.desc.sequence_id = ++_global_seq_id;
    blobDesc.desc.blob_size = BLOB_SIZE*2;

    std::string blobString;
    err= blobDesc.getSerialized(blobString);
    EXPECT_EQ(true, err.ok());

    err = dataMgr->timeVolCat_->migrateDescriptor(volId, blobName,
                                                        blobString);
    EXPECT_EQ(true, err.ok());

    // confirm updated blob's sequence id is returned
    err= dataMgr->timeVolCat_->queryIface()->getVolumeSequenceId(volId, tmp_seq_id);
    EXPECT_EQ(true, err.ok());
    EXPECT_EQ(NUM_BLOBS+1, tmp_seq_id);

    // confirm blob size increased
    BlobMetaDesc blobDesc2;
    fpi::FDSP_BlobObjectList objList;

    err = dataMgr->timeVolCat_->queryIface()->
        getBlobAndMetaFromSnapshot(volId, blobName, blobDesc2, objList, NULL);
     EXPECT_EQ(true, err.ok());

    EXPECT_EQ(BLOB_SIZE*2, blobDesc2.desc.blob_size);
}

TEST_F(SeqIdTest, MigrateBlobPutTruncate){
    MAX_OBJECT_SIZE = 2 * 1024 * 1024;    // 2MB
    BLOB_SIZE = 16 * 1024 * 1024;   // 16 MB
    NUM_BLOBS = 1024;

    auto volId1 =  dmTester->TESTVOLID;

    putBlobOnce();

    // read the final blob
    std::string blobName = dmTester->getBlobName(NUM_BLOBS-1);
    BlobMetaDesc blobDesc;
    fpi::FDSP_BlobObjectList objList;

    Error err = dataMgr->timeVolCat_->queryIface()->
        getBlobAndMetaFromSnapshot(volId1, blobName, blobDesc, objList, NULL);
    EXPECT_EQ(true, err.ok());

    EXPECT_EQ(BLOB_SIZE, blobDesc.desc.blob_size);

    // trim the size of the blob and write it
    blobDesc.desc.blob_name = blobName;
    blobDesc.desc.version = 2;
    blobDesc.desc.sequence_id = ++_global_seq_id;
    blobDesc.desc.blob_size = blobDesc.desc.blob_size/2;

    std::string blobString;
    err= blobDesc.getSerialized(blobString);
    EXPECT_EQ(true, err.ok());

    err = dataMgr->timeVolCat_->migrateDescriptor(volId1, blobName,
                                                  blobString);
    EXPECT_EQ(true, err.ok());

    // confirm the blob offsets were truncated
    BlobMetaDesc blobDesc2;
    fpi::FDSP_BlobObjectList objList2;

    err = dataMgr->timeVolCat_->queryIface()->
        getBlobAndMetaFromSnapshot(volId1, blobName, blobDesc2, objList2, NULL);
    EXPECT_EQ(true, err.ok());

    // ensure descriptor is identical to the one we wrote
    EXPECT_EQ(blobDesc, blobDesc2);

    // ensure size field and object list count are both halved
    EXPECT_EQ(blobDesc.desc.blob_size, blobDesc2.desc.blob_size);
    EXPECT_EQ(objList.size()/2, objList2.size());

    // manually truncate the original object list so we can compare to the new one
    objList.erase(objList.begin() + (objList.size()/2), objList.end());

    // ensure object list is identical to the first half of the old list
    for(auto it = objList.cbegin(), it2 = objList2.cbegin(); it == objList.cend(); ++it, ++it2){
        EXPECT_EQ(it->offset, it2->offset);
        EXPECT_EQ(it->data_obj_id.digest, it2->data_obj_id.digest);
        EXPECT_EQ(it->size, it2->size);
    }
}

int main(int argc, char** argv) {
    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    ::testing::InitGoogleMock(&argc, argv);

    // process command line options
    po::options_description desc("\nDM test Command line options");
    desc.add_options()
            ("help,h", "help/ usage message");  // NOLINT

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }

    // give each test two volumes
    NUM_VOLUMES = ::testing::UnitTest::GetInstance()->total_test_count()*2;

    dmTester = new fds::DMTester(argc, argv);
    dmTester->start();
    // setup default volumes
    generateVolumes(dmTester->volumes);
    dmTester->TESTBLOB = "testblob";

    int retCode = RUN_ALL_TESTS();

    dmTester->stop();
    dmTester->clean();

    return retCode;
}
