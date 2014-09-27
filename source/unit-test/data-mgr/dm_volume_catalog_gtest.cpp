/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#define GTEST_USE_OWN_TR1_TUPLE 0

#include <unistd.h>
#include <stdlib.h>

#include <vector>
#include <string>

#include <boost/shared_ptr.hpp>

#include <leveldb/write_batch.h>

#include <fds_types.h>
#include <Catalog.h>
#include <ObjectId.h>
#include <fds_process.h>
#include <util/timeutils.h>
#include <dm-platform.h>
#include <DataMgr.h>
#include <dm-vol-cat/DmVolumeCatalog.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;  // NOLINT

static fds_uint32_t NUM_VOLUMES = 1;
static fds_uint32_t MAX_OBJECT_SIZE = 2097152;    // 2MB
static fds_uint64_t BLOB_SIZE = 1 * 1024 * 1024 * 1024;   // 1GB
static fds_uint32_t NUM_BLOBS = 1;

static fds_bool_t PUTS_ONLY = false;
static fds_bool_t NO_DELETE = false;

static std::atomic<fds_uint32_t> blobCount;
static std::atomic<fds_uint64_t> txCount;

static std::atomic<fds_uint32_t> taskCount;

namespace fds {
DataMgr * dataMgr = 0;
}

static Error expungeObjects(fds_volid_t volId, const std::vector<ObjectID> & oids) {
    for (auto i : oids) {
        std::cout << "Expunged volume: '" << volId << "' object: '"
                << i << "'" << std::endl;
    }
    return ERR_OK;
}

class MockDataMgr : public PlatformProcess {
  public:
    MockDataMgr(int argc, char *argv[]) {
        static Module *modVec[] = {};
        init(argc, argv, "fds.dm.", "dm.log", &gl_DmPlatform, modVec);
    }

    virtual int run() override {
        return 0;
    }
};

struct BlobDetails {
    std::string name;
    fds_uint64_t size;

    MetaDataList::ptr metaList;
    BlobObjList::ptr objList;

    BlobDetails() {
        name = "file_" + std::to_string(blobCount++);
        size = BLOB_SIZE;

        // metadata for blob
        metaList.reset(new MetaDataList());
        for (int i = 0; i < 10; i++) {
            std::string tag = "tag_" + std::to_string(i);
            std::string val = "value_" + std::to_string(i);

            (*metaList)[tag] = val;
        }

        // data (offset/ oid) for blob
        objList.reset(new BlobObjList());
        for (fds_uint64_t offset = 0; offset < size; offset += MAX_OBJECT_SIZE) {
            std::string data = name + std::to_string(offset) +
                    std::to_string(util::getTimeStampNanos());

            ObjectID objId = ObjIdGen::genObjectId(data.c_str(), data.size());
            // fds_uint64_t size = ((offset + MAX_OBJECT_SIZE) < size ? MAX_OBJECT_SIZE
            //         : (size - offset));
            fds_uint64_t size = MAX_OBJECT_SIZE;
            BlobObjInfo blobObj(objId, size);

            (*objList)[offset] = blobObj;
        }
        objList->setEndOfBlob();
    }
};

class DISABLED_DmVolumeCatalogTest : public ::testing::Test {
  public:
    virtual void SetUp() override;
    virtual void TearDown() override;

    void generateVolumes();

    void testPutBlob(fds_volid_t volId, boost::shared_ptr<const BlobDetails> blob);

    boost::shared_ptr<DmVolumeCatalog> volcat;

    std::vector<boost::shared_ptr<VolumeDesc> > volumes;

    boost::shared_ptr<LatencyCounter> putCounter;
};

void DISABLED_DmVolumeCatalogTest::SetUp() {
    volcat.reset(new DmVolumeCatalog("dm_volume_catallog_gtest.ldb"));
    ASSERT_NE(static_cast<DmVolumeCatalog*>(0), volcat.get());

    putCounter.reset(new LatencyCounter("put", 0, 0));

    volcat->registerExpungeObjectsCb(&expungeObjects);

    generateVolumes();
}

void DISABLED_DmVolumeCatalogTest::generateVolumes() {
    for (fds_uint32_t i = 1; i <= NUM_VOLUMES; ++i) {
        std::string name = "test" + std::to_string(i);

        boost::shared_ptr<VolumeDesc> vdesc(new VolumeDesc(name, i));

        vdesc->tennantId = i;
        vdesc->localDomainId = i;
        vdesc->globDomainId = i;

        vdesc->volType = fpi::FDSP_VOL_S3_TYPE;
        vdesc->capacity = 100 * 1024 * 1024;     // 100TB
        vdesc->maxQuota = 90;
        vdesc->replicaCnt = 1;
        vdesc->maxObjSizeInBytes = MAX_OBJECT_SIZE;

        vdesc->writeQuorum = 1;
        vdesc->readQuorum = 1;
        vdesc->consisProtocol = fpi::FDSP_CONS_PROTO_EVENTUAL;

        vdesc->volPolicyId = 50;
        vdesc->archivePolicyId = 1;
        vdesc->mediaPolicy = fpi::FDSP_MEDIA_POLICY_HYBRID;
        vdesc->placementPolicy = 1;
        vdesc->appWorkload = fpi::FDSP_APP_S3_OBJS;
        vdesc->backupVolume = invalid_vol_id;

        vdesc->iops_min = 1000;
        vdesc->iops_max = 5000;
        vdesc->relativePrio = 1;

        vdesc->fSnapshot = false;
        vdesc->srcVolumeId = invalid_vol_id;
        vdesc->lookupVolumeId = invalid_vol_id;
        vdesc->qosQueueId = invalid_vol_id;

        volumes.push_back(vdesc);

        volcat->addCatalog(*volumes[i - 1]);
        volcat->activateCatalog(i);
    }
}

void DISABLED_DmVolumeCatalogTest::testPutBlob(fds_volid_t volId,
        boost::shared_ptr<const BlobDetails> blob) {
    boost::shared_ptr<BlobTxId> txId(new BlobTxId(++txCount));
    fds_uint64_t startTs = util::getTimeStampNanos();
    Error rc = volcat->putBlob(volId, blob->name, blob->metaList, blob->objList, txId);
    fds_uint64_t endTs = util::getTimeStampNanos();
    putCounter->update(endTs - startTs);
    taskCount--;
    EXPECT_TRUE(rc.ok());
}

void DISABLED_DmVolumeCatalogTest::TearDown() {
    volcat.reset();

    const FdsRootDir* root = g_fdsprocess->proc_fdsroot();
    ASSERT_NE(static_cast<FdsRootDir*>(0), root);

    std::ostringstream oss;
    oss << "rm -rf " << root->dir_user_repo_dm() << "*";
    int rc = system(oss.str().c_str());
    ASSERT_EQ(0, rc);
}

TEST_F(DISABLED_DmVolumeCatalogTest, all_ops) {
    taskCount = NUM_BLOBS;
    for (fds_uint32_t i = 0; i < NUM_BLOBS; ++i) {
        fds_volid_t volId = volumes[i % volumes.size()]->volUUID;

        boost::shared_ptr<const BlobDetails> blob(new BlobDetails());
        g_fdsprocess->proc_thrpool()->schedule(&DISABLED_DmVolumeCatalogTest::testPutBlob,
                this, volId, blob);
    }

    while (taskCount) usleep(10 * 1024);

    if (PUTS_ONLY) goto done;

  done:
    std::cout << putCounter->latency() << " " << putCounter->count() << std::endl;
}

int main(int argc, char** argv) {
    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    ::testing::InitGoogleMock(&argc, argv);

    MockDataMgr mockDm(argc, argv);
    return RUN_ALL_TESTS();
}
