/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include "./dm_mocks.h"
#include "./dm_gtest.h"
#include "./dm_utils.h"
#include <vector>
#include <string>
#include <thread>

#include <dm-vol-cat/DmVolumeDirectory.h>
#include <PerfTrace.h>

#define DM_CATALOG_TYPE DmVolumeDirectory
// #define DM_CATALOG_TYPE DmVolumeCatalog

boost::shared_ptr<LatencyCounter> tpPutCounter(new LatencyCounter("threadpool put", 0, 0));
boost::shared_ptr<LatencyCounter> tpGetCounter(new LatencyCounter("threadpool get", 0, 0));
boost::shared_ptr<LatencyCounter> tpDeleteCounter(new LatencyCounter("threadpool delete", 0, 0));

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
    boost::shared_ptr<PerfContext> pctx = PerfTracer::tracePointEnd(blob->name);
    tpPutCounter->update(pctx->end_cycle - pctx->start_cycle);
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
    boost::shared_ptr<PerfContext> pctx = PerfTracer::tracePointEnd(blobName);
    tpGetCounter->update(pctx->end_cycle - pctx->start_cycle);
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
    boost::shared_ptr<PerfContext> pctx = PerfTracer::tracePointEnd(blobName);
    tpDeleteCounter->update(pctx->end_cycle - pctx->start_cycle);
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
        PerfTracer::tracePointBegin(blob->name, DM_VOL_CAT_WRITE, volId);
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
            PerfTracer::tracePointBegin(it.blob_name, DM_VOL_CAT_READ, vdesc->volUUID);
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

            PerfTracer::tracePointBegin(it.blob_name, DM_TX_OP, vdesc->volUUID);
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
    std::cout << "\033[33m[threadpool put latency]\033[39m " << std::fixed << std::setprecision(3)
            << (tpPutCounter->latency() / (1024 * 1024)) << "ms     \033[33m[count]\033[39m "
            << tpPutCounter->count() << std::endl;
    std::cout << "\033[33m[get latency]\033[39m " << std::fixed << std::setprecision(3)
            << (getCounter->latency() / (1024 * 1024)) << "ms     \033[33m[count]\033[39m "
            << getCounter->count() << std::endl;
    std::cout << "\033[33m[threadpool get latency]\033[39m " << std::fixed << std::setprecision(3)
            << (tpGetCounter->latency() / (1024 * 1024)) << "ms     \033[33m[count]\033[39m "
            << tpGetCounter->count() << std::endl;
    std::cout << "\033[33m[delete latency]\033[39m " << std::fixed << std::setprecision(3)
            << (deleteCounter->latency() / (1024 * 1024)) << "ms     \033[33m[count]\033[39m "
            << deleteCounter->count() << std::endl;
    std::cout << "\033[33m[threadpool delete latency]\033[39m " <<
            std::fixed << std::setprecision(3) << (tpDeleteCounter->latency() / (1024 * 1024))
            << "ms     \033[33m[count]\033[39m " << tpDeleteCounter->count() << std::endl;

    std::this_thread::yield();
}

int main(int argc, char** argv) {
    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    ::testing::InitGoogleMock(&argc, argv);

    MockDataMgr mockDm(argc, argv);

    // process command line options
    po::options_description desc("\nDM test Command line options");
    desc.add_options()
            ("help,h", "help/ usage message")
            ("num-volumes,v",
            po::value<fds_uint32_t>(&NUM_VOLUMES)->default_value(NUM_VOLUMES),
            "number of volumes")
            ("obj-size,o",
            po::value<fds_uint32_t>(&MAX_OBJECT_SIZE)->default_value(MAX_OBJECT_SIZE),
            "max object size in bytes")
            ("blob-size,b",
            po::value<fds_uint64_t>(&BLOB_SIZE)->default_value(BLOB_SIZE),
            "blob size in bytes")
            ("num-blobs,n",
            po::value<fds_uint32_t>(&NUM_BLOBS)->default_value(NUM_BLOBS),
            "number of blobs")
            ("puts-only", "do put operations only")
            ("no-delete", "do put & get operations only");

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }

    PUTS_ONLY = 0 != vm.count("puts-only");
    NO_DELETE = 0 != vm.count("no-delete");

    return RUN_ALL_TESTS();
}
