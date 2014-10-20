/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <dm_gtest.h>

#include <vector>
#include <string>
#include <thread>

#include <dm-vol-cat/DmVolumeDirectory.h>

#define DM_CATALOG_TYPE DmVolumeDirectory
// #define DM_CATALOG_TYPE DmVolumeCatalog

static std::atomic<fds_uint32_t> taskCount;

class DmVolumeCatalogTest : public ::testing::Test {
  public:
    virtual void SetUp() override;
    virtual void TearDown() override;

    void testPutBlob(fds_volid_t volId, boost::shared_ptr<const BlobDetails> blob);

    void testGetBlob(fds_volid_t volId, const std::string blobName);

    void testDeleteBlob(fds_volid_t volId, const std::string blobName, blob_version_t version);

    boost::shared_ptr<DM_CATALOG_TYPE> volcat;

    std::vector<boost::shared_ptr<VolumeDesc> > volumes;
};

void DmVolumeCatalogTest::SetUp() {
    volcat.reset(new DM_CATALOG_TYPE("dm_volume_catallog_gtest.ldb"));
    ASSERT_NE(static_cast<DM_CATALOG_TYPE*>(0), volcat.get());

    volcat->registerExpungeObjectsCb(&expungeObjects);

    generateVolumes(volumes);

    for (auto it : volumes) {
        volcat->addCatalog(*it);
        volcat->activateCatalog(it->volUUID);
    }
}

void DmVolumeCatalogTest::testPutBlob(fds_volid_t volId,
        boost::shared_ptr<const BlobDetails> blob) {
    boost::shared_ptr<BlobTxId> txId(new BlobTxId(++txCount));
    // Put
    fds_uint64_t startTs = util::getTimeStampNanos();
    Error rc = volcat->putBlob(volId, blob->name, blob->metaList, blob->objList, txId);
    fds_uint64_t endTs = util::getTimeStampNanos();
    putCounter->update(endTs - startTs);
    if (taskCount) taskCount--;
    EXPECT_TRUE(rc.ok());
}

void DmVolumeCatalogTest::testGetBlob(fds_volid_t volId, const std::string blobName) {
    blob_version_t version;
    fpi::FDSP_MetaDataList metaList;
    fpi::FDSP_BlobObjectList objList;
    // Get
    fds_uint64_t startTs = util::getTimeStampNanos();
    // TODO(Andrew/Umesh): Get a meaningful offset
    Error rc = volcat->getBlob(volId, blobName, 0, -1, &version, &metaList, &objList);
    fds_uint64_t endTs = util::getTimeStampNanos();
    getCounter->update(endTs - startTs);
    if (taskCount) taskCount--;
    EXPECT_TRUE(rc.ok());
}

void DmVolumeCatalogTest::testDeleteBlob(fds_volid_t volId, const std::string blobName,
        blob_version_t version) {
    // Delete
    fds_uint64_t startTs = util::getTimeStampNanos();
    Error rc = volcat->deleteBlob(volId, blobName, version);
    fds_uint64_t endTs = util::getTimeStampNanos();
    deleteCounter->update(endTs - startTs);
    EXPECT_TRUE(rc.ok());

    // Get
    fpi::FDSP_MetaDataList metaList;
    fpi::FDSP_BlobObjectList objList;
    rc = volcat->getBlob(volId, blobName, 0, -1, &version, &metaList, &objList);
    EXPECT_FALSE(rc.ok());

    if (taskCount) taskCount--;
}

void DmVolumeCatalogTest::TearDown() {
    volcat.reset();

    volumes.clear();
    putCounter->reset();
    getCounter->reset();
    deleteCounter->reset();

    if (!NO_DELETE) {
        const FdsRootDir* root = g_fdsprocess->proc_fdsroot();
        ASSERT_NE(static_cast<FdsRootDir*>(0), root);

        std::ostringstream oss;
        oss << "rm -rf " << root->dir_user_repo_dm() << "*";
        int rc = system(oss.str().c_str());
        ASSERT_EQ(0, rc);
    }
}

TEST_F(DmVolumeCatalogTest, copy_volume) {
    std::vector<boost::shared_ptr<VolumeDesc> > snapshots;
    generateVolumes(snapshots);

    for (fds_uint32_t i = 0; i < NUM_VOLUMES; ++i) {
        boost::shared_ptr<const BlobDetails> blob(new BlobDetails());
        testPutBlob(volumes[i]->volUUID, blob);

        snapshots[i]->fSnapshot = true;
        snapshots[i]->srcVolumeId = volumes[i]->volUUID;

        Error rc = volcat->copyVolume(*snapshots[i]);
        EXPECT_TRUE(rc.ok());

        rc = volcat->activateCatalog(snapshots[i]->volUUID);
        EXPECT_TRUE(rc.ok());

        fds_uint64_t size = 0;
        fds_uint64_t blobCount = 0;
        fds_uint64_t objCount = 0;
        rc = volcat->getVolumeMeta(snapshots[i]->volUUID, &size, &blobCount, &objCount);
        EXPECT_TRUE(rc.ok());
        EXPECT_EQ(blobCount, 1);
        EXPECT_EQ(size, blobCount * BLOB_SIZE);
    }
}

TEST_F(DmVolumeCatalogTest, all_ops) {
    taskCount = NUM_BLOBS;
    for (fds_uint32_t i = 0; i < NUM_BLOBS; ++i) {
        fds_volid_t volId = volumes[i % volumes.size()]->volUUID;

        boost::shared_ptr<const BlobDetails> blob(new BlobDetails());
        g_fdsprocess->proc_thrpool()->schedule(&DmVolumeCatalogTest::testPutBlob,
                this, volId, blob);
    }

    while (taskCount) usleep(10 * 1024);

    if (PUTS_ONLY) goto done;

    // get volume details
    for (auto vdesc : volumes) {
        fds_uint64_t size = 0, blobCount = 0, objCount = 0;
        Error rc = volcat->getVolumeMeta(vdesc->volUUID, &size, &blobCount, &objCount);
        EXPECT_TRUE(rc.ok());
        EXPECT_EQ(size, static_cast<fds_uint64_t>(blobCount) * BLOB_SIZE);

        // get list of blobs for volume
        fpi::BlobInfoListType blobList;
        rc = volcat->listBlobs(vdesc->volUUID, &blobList);
        EXPECT_TRUE(rc.ok());
        EXPECT_EQ(blobList.size(), blobCount);

//        if (blobCount) {
//            rc = volcat->markVolumeDeleted(vdesc->volUUID);
//            EXPECT_EQ(rc.GetErrno(), ERR_VOL_NOT_EMPTY);
//        }

        taskCount += blobCount;
        for (auto it : blobList) {
            g_fdsprocess->proc_thrpool()->schedule(&DmVolumeCatalogTest::testGetBlob,
                    this, vdesc->volUUID, it.blob_name);
        }
        while (taskCount) usleep(10 * 1024);

        taskCount += blobCount;
        for (auto it : blobList) {
            blob_version_t version = 0;
            fds_uint64_t blobSize = 0;
            fpi::FDSP_MetaDataList metaList;

            rc = volcat->getBlobMeta(vdesc->volUUID, it.blob_name, &version, &blobSize, &metaList);
            EXPECT_EQ(blobSize, BLOB_SIZE);
            EXPECT_EQ(metaList.size(), NUM_TAGS);

            if (NO_DELETE) {
                taskCount--;
                continue;
            }

            g_fdsprocess->proc_thrpool()->schedule(&DmVolumeCatalogTest::testDeleteBlob,
                    this, vdesc->volUUID, it.blob_name, version);
        }
        while (taskCount) usleep(10 * 1024);

        if (NO_DELETE) continue;

        rc = volcat->markVolumeDeleted(vdesc->volUUID);
        EXPECT_TRUE(rc.ok());

        rc = volcat->deleteEmptyCatalog(vdesc->volUUID);
        EXPECT_TRUE(rc.ok());
    }

  done:
    std::cout << "\033[33m[put latency]\033[39m " << std::fixed << std::setprecision(3)
            << (putCounter->latency() / (1024 * 1024)) << "ms     \033[33m[count]\033[39m "
            << putCounter->count() << std::endl;
    std::cout << "\033[33m[get latency]\033[39m " << std::fixed << std::setprecision(3)
            << (getCounter->latency() / (1024 * 1024)) << "ms     \033[33m[count]\033[39m "
            << getCounter->count() << std::endl;
    std::cout << "\033[33m[delete latency]\033[39m " << std::fixed << std::setprecision(3)
            << (deleteCounter->latency() / (1024 * 1024)) << "ms     \033[33m[count]\033[39m "
            << deleteCounter->count() << std::endl;
    std::this_thread::yield();
}

