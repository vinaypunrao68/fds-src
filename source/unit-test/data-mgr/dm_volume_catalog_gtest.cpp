/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include "./dm_mocks.h"
#include "./dm_gtest.h"
#include "./dm_utils.h"
#include <vector>
#include <string>
#include <thread>

#include <dm-vol-cat/DmVolumeCatalog.h>
#include <util/color.h>
#include <PerfTrace.h>

boost::shared_ptr<LatencyCounter> tpPutCounter(new LatencyCounter("threadpool put", invalid_vol_id, 0));
boost::shared_ptr<LatencyCounter> tpGetCounter(new LatencyCounter("threadpool get", invalid_vol_id, 0));
boost::shared_ptr<LatencyCounter> tpDeleteCounter(new LatencyCounter("threadpool delete", invalid_vol_id, 0));

boost::shared_ptr<LatencyCounter> totalPutCounter(new LatencyCounter("end to end put", invalid_vol_id, 0));
boost::shared_ptr<LatencyCounter> totalGetCounter(new LatencyCounter("end to end get", invalid_vol_id, 0));
boost::shared_ptr<LatencyCounter> totalDeleteCounter(
        new LatencyCounter("end to end delete", invalid_vol_id, 0));

fds::concurrency::TaskStatus taskCount(0);

class DmVolumeCatalogTest : public ::testing::Test {
  public:
    virtual void SetUp() override;
    virtual void TearDown() override;

    void testPutBlob(fds_volid_t volId, boost::shared_ptr<const BlobDetails> blob);

    void testGetBlob(fds_volid_t volId, const std::string blobName);

    void testDeleteBlob(fds_volid_t volId, const std::string blobName, blob_version_t version);

    boost::shared_ptr<DmVolumeCatalog> volcat;

    std::vector<boost::shared_ptr<VolumeDesc> > volumes;
};

void DmVolumeCatalogTest::SetUp() {
    volcat.reset(new DmVolumeCatalog("dm_volume_catallog_gtest.ldb"));
    ASSERT_NE(static_cast<DmVolumeCatalog*>(0), volcat.get());

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
    static sequence_id_t sequence_id = 0;
    // Put
    fds_uint64_t startTs = util::getTimeStampNanos();
    Error rc = volcat->putBlob(volId, blob->name, blob->metaList, blob->objList, txId, ++sequence_id);
    fds_uint64_t endTs = util::getTimeStampNanos();
    putCounter->update(endTs - startTs);
    boost::shared_ptr<PerfContext> pctx = PerfTracer::tracePointEnd(blob->name);
    tpPutCounter->update(pctx->end_cycle - pctx->start_cycle);
    taskCount.done();
    EXPECT_TRUE(rc.ok());
}

void DmVolumeCatalogTest::testGetBlob(fds_volid_t volId, const std::string blobName) {
    blob_version_t version;
    fds_uint64_t blobSize;
    fpi::FDSP_MetaDataList metaList;
    fpi::FDSP_BlobObjectList objList;
    // Get
    fds_uint64_t startTs = util::getTimeStampNanos();
    // TODO(Andrew/Umesh): Get a meaningful offset
    Error rc = volcat->getBlob(volId, blobName, 0, -1, &version, &metaList, &objList, &blobSize);
    fds_uint64_t endTs = util::getTimeStampNanos();
    getCounter->update(endTs - startTs);
    boost::shared_ptr<PerfContext> pctx = PerfTracer::tracePointEnd(blobName);
    tpGetCounter->update(pctx->end_cycle - pctx->start_cycle);
    taskCount.done();
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
    fds_uint64_t blobSize;
    fpi::FDSP_MetaDataList metaList;
    fpi::FDSP_BlobObjectList objList;
    rc = volcat->getBlob(volId, blobName, 0, -1, &version, &metaList, &objList, &blobSize);
    EXPECT_FALSE(rc.ok());

    taskCount.done();
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
        PerfTracer::tracePointBegin(blob->name, PerfEventType::DM_VOL_CAT_WRITE, volumes[i]->volUUID);
        testPutBlob(volumes[i]->volUUID, blob);

        snapshots[i]->fSnapshot = true;
        snapshots[i]->srcVolumeId = volumes[i]->volUUID;

        Error rc = volcat->copyVolume(*snapshots[i]);
        std::cout << "[copyVolume returned: ] " << rc << std::endl;
        EXPECT_TRUE(rc.ok());

        rc = volcat->activateCatalog(snapshots[i]->volUUID);
        std::cout << "[activateCatalog returned: ] " << rc << std::endl;
        EXPECT_TRUE(rc.ok());

        fds_uint64_t size = 0;
        fds_uint64_t blobCount = 0;
        fds_uint64_t objCount = 0;
        rc = volcat->statVolume(snapshots[i]->volUUID, &size, &blobCount, &objCount);
        std::cout << "[statVolume returned: ] " << rc << std::endl;
        EXPECT_TRUE(rc.ok());
        EXPECT_EQ(blobCount, 1);
        EXPECT_EQ(size, blobCount * BLOB_SIZE);
    }
}

TEST_F(DmVolumeCatalogTest, all_ops) {
    taskCount.reset(NUM_BLOBS);
    fds_uint64_t e2eStatTs = util::getTimeStampNanos();
    for (fds_uint32_t i = 0; i < NUM_BLOBS; ++i) {
        fds_volid_t volId = volumes[i % volumes.size()]->volUUID;

        boost::shared_ptr<const BlobDetails> blob(new BlobDetails());
        PerfTracer::tracePointBegin(blob->name, PerfEventType::DM_VOL_CAT_WRITE, volId);
        g_fdsprocess->proc_thrpool()->schedule(&DmVolumeCatalogTest::testPutBlob,
                this, volId, blob);
    }

    taskCount.await();
    fds_uint64_t e2eEndTs = util::getTimeStampNanos();
    totalPutCounter->update(e2eEndTs - e2eStatTs);

    if (PUTS_ONLY) goto done;

    // get volume details
    for (auto vdesc : volumes) {
        fds_uint64_t size = 0, blobCount = 0, objCount = 0;
        Error rc = volcat->statVolume(vdesc->volUUID, &size, &blobCount, &objCount);
        EXPECT_TRUE(rc.ok());
        EXPECT_EQ(size, static_cast<fds_uint64_t>(blobCount) * BLOB_SIZE);

        // Set/get/check some metadata for the volume
        fpi::FDSP_MetaDataPair metadataPair;
        metadataPair.key = "Volume Name";
        metadataPair.value = vdesc->name;
        fpi::FDSP_MetaDataList setMetadataList;
        setMetadataList.push_back(metadataPair);
        rc = volcat->setVolumeMetadata(vdesc->volUUID, setMetadataList, 0);
        EXPECT_TRUE(rc.ok());

        fpi::FDSP_MetaDataList getMetadataList;
        rc = volcat->getVolumeMetadata(vdesc->volUUID, getMetadataList);
        EXPECT_TRUE(rc.ok());
        EXPECT_TRUE(getMetadataList == setMetadataList);

        // get list of blobs for volume
        fpi::BlobDescriptorListType blobList;
        rc = volcat->listBlobs(vdesc->volUUID, &blobList);
        EXPECT_TRUE(rc.ok());
        EXPECT_EQ(blobList.size(), blobCount);

//        if (blobCount) {
//            rc = volcat->markVolumeDeleted(vdesc->volUUID);
//            EXPECT_EQ(rc.GetErrno(), ERR_VOL_NOT_EMPTY);
//        }

        taskCount.reset(taskCount.getNumTasks() + blobCount);
        fds_uint64_t e2eStatTs = util::getTimeStampNanos();
        for (auto it : blobList) {
            PerfTracer::tracePointBegin(it.name, PerfEventType::DM_QUERY_REQ, vdesc->volUUID);
            g_fdsprocess->proc_thrpool()->schedule(&DmVolumeCatalogTest::testGetBlob,
                    this, vdesc->volUUID, it.name);
        }
        taskCount.await();
        fds_uint64_t e2eEndTs = util::getTimeStampNanos();
        totalGetCounter->update(e2eEndTs - e2eStatTs);

        taskCount.reset(taskCount.getNumTasks() + blobCount);
        e2eStatTs = util::getTimeStampNanos();
        for (auto it : blobList) {
            blob_version_t version = 0;
            fds_uint64_t blobSize = 0;
            fpi::FDSP_MetaDataList metaList;

            rc = volcat->getBlobMeta(vdesc->volUUID, it.name, &version, &blobSize, &metaList);
            EXPECT_EQ(blobSize, BLOB_SIZE);
            EXPECT_EQ(metaList.size(), NUM_TAGS);

            if (NO_DELETE) {
                taskCount.done();
                continue;
            }

            PerfTracer::tracePointBegin(it.name, PerfEventType::DM_DELETE_BLOB_REQ, vdesc->volUUID);
            g_fdsprocess->proc_thrpool()->schedule(&DmVolumeCatalogTest::testDeleteBlob,
                    this, vdesc->volUUID, it.name, version);
        }
        taskCount.await();
        e2eEndTs = util::getTimeStampNanos();
        totalDeleteCounter->update(e2eEndTs - e2eStatTs);

        if (NO_DELETE) continue;

        rc = volcat->markVolumeDeleted(vdesc->volUUID);
        EXPECT_TRUE(rc.ok());

        rc = volcat->deleteEmptyCatalog(vdesc->volUUID);
        EXPECT_TRUE(rc.ok());
    }

  done:
    std::cout << Color::Yellow << "[put latency] " << std::fixed << std::setprecision(3) <<
            (putCounter->latency() / (1024 * 1024)) << "ms     [count] " << putCounter->count() <<
            Color::End << std::endl;
    std::cout << Color::Yellow << "[threadpool put latency] " << std::fixed <<
            std::setprecision(3) << (tpPutCounter->latency() / (1024 * 1024)) << "ms     [count] "
            << tpPutCounter->count() << Color::End << std::endl;
    std::cout << Color::Yellow << "[get latency] " << std::fixed << std::setprecision(3) <<
            (getCounter->latency() / (1024 * 1024)) << "ms     [count] " << getCounter->count() <<
            Color::End << std::endl;
    std::cout << Color::Yellow << "[threadpool get latency] " << std::fixed <<
            std::setprecision(3) << (tpGetCounter->latency() / (1024 * 1024)) << "ms     [count] "
            << tpGetCounter->count() << Color::End << std::endl;
    std::cout << Color::Yellow << "[delete latency] " << std::fixed << std::setprecision(3)
            << (deleteCounter->latency() / (1024 * 1024)) << "ms     [count] " <<
            deleteCounter->count() << Color::End << std::endl;
    std::cout << Color::Yellow << "[threadpool delete latency] " << std::fixed <<
            std::setprecision(3) << (tpDeleteCounter->latency() / (1024 * 1024))
            << "ms     [count] " << tpDeleteCounter->count() << Color::End << std::endl;

    std::cout << std::endl;
    std::cout << Color::Yellow << "[total put latency] " << std::fixed << std::setprecision(3)
            << (totalPutCounter->latency() / (1024 * 1024)) << "ms     [count] "
            << totalPutCounter->count() << Color::End << std::endl;
    std::cout << Color::Yellow << "[total get latency] " << std::fixed << std::setprecision(3)
            << (totalGetCounter->latency() / (1024 * 1024)) << "ms     [count] "
            << totalGetCounter->count() << Color::End << std::endl;
    std::cout << Color::Yellow << "[total delete latency] " << std::fixed << std::setprecision(3)
            << (totalDeleteCounter->latency() / (1024 * 1024)) << "ms     [count] " <<
            totalDeleteCounter->count() << Color::End << std::endl;

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
