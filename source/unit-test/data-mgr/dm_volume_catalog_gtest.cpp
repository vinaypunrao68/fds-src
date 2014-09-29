/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#define GTEST_USE_OWN_TR1_TUPLE 0

#include <unistd.h>
#include <stdlib.h>

#include <vector>
#include <string>
#include <thread>

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
namespace po = boost::program_options;

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
            fds_uint64_t sz = ((offset + MAX_OBJECT_SIZE) < size ? MAX_OBJECT_SIZE
                    : (size - offset));
            // fds_uint64_t size = MAX_OBJECT_SIZE;
            BlobObjInfo blobObj(objId, sz);

            (*objList)[offset] = blobObj;
        }
        objList->setEndOfBlob();
    }
};

class DmVolumeCatalogTest : public ::testing::Test {
  public:
    virtual void SetUp() override;
    virtual void TearDown() override;

    void generateVolumes();

    void testPutBlob(fds_volid_t volId, boost::shared_ptr<const BlobDetails> blob);

    void testGetBlob(fds_volid_t volId, const std::string blobName);

    boost::shared_ptr<DmVolumeCatalog> volcat;

    std::vector<boost::shared_ptr<VolumeDesc> > volumes;

    boost::shared_ptr<LatencyCounter> putCounter;
    boost::shared_ptr<LatencyCounter> getCounter;
};

void DmVolumeCatalogTest::SetUp() {
    volcat.reset(new DmVolumeCatalog("dm_volume_catallog_gtest.ldb"));
    ASSERT_NE(static_cast<DmVolumeCatalog*>(0), volcat.get());

    putCounter.reset(new LatencyCounter("put", 0, 0));
    getCounter.reset(new LatencyCounter("get", 0, 0));

    volcat->registerExpungeObjectsCb(&expungeObjects);

    generateVolumes();
}

void DmVolumeCatalogTest::generateVolumes() {
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

void DmVolumeCatalogTest::testPutBlob(fds_volid_t volId,
        boost::shared_ptr<const BlobDetails> blob) {
    boost::shared_ptr<BlobTxId> txId(new BlobTxId(++txCount));
    // Put
    fds_uint64_t startTs = util::getTimeStampNanos();
    Error rc = volcat->putBlob(volId, blob->name, blob->metaList, blob->objList, txId);
    fds_uint64_t endTs = util::getTimeStampNanos();
    putCounter->update(endTs - startTs);
    taskCount--;
    EXPECT_TRUE(rc.ok());
}

void DmVolumeCatalogTest::testGetBlob(fds_volid_t volId, const std::string blobName) {
    blob_version_t version;
    fpi::FDSP_MetaDataList metaList;
    fpi::FDSP_BlobObjectList objList;
    // Get
    fds_uint64_t startTs = util::getTimeStampNanos();
    Error rc = volcat->getBlob(volId, blobName, &version, &metaList, &objList);
    fds_uint64_t endTs = util::getTimeStampNanos();
    getCounter->update(endTs - startTs);
    taskCount--;
    EXPECT_TRUE(rc.ok());
}

void DmVolumeCatalogTest::TearDown() {
    volcat.reset();

    const FdsRootDir* root = g_fdsprocess->proc_fdsroot();
    ASSERT_NE(static_cast<FdsRootDir*>(0), root);

    std::ostringstream oss;
    oss << "rm -rf " << root->dir_user_repo_dm() << "*";
    int rc = system(oss.str().c_str());
    ASSERT_EQ(0, rc);
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
        EXPECT_EQ(size, blobCount * BLOB_SIZE);

        // get list of blobs for volume
        fpi::BlobInfoListType blobList;
        rc = volcat->listBlobs(vdesc->volUUID, &blobList);
        EXPECT_TRUE(rc.ok());
        EXPECT_EQ(blobList.size(), blobCount);

        taskCount += blobCount;
        for (auto it : blobList) {
            g_fdsprocess->proc_thrpool()->schedule(&DmVolumeCatalogTest::testGetBlob,
                    this, vdesc->volUUID, it.blob_name);
        }
        while (taskCount) usleep(10 * 1024);
    }

  done:
    std::cout << "\033[33m[put latency]\033[39m " << std::fixed << std::setprecision(3)
            << (putCounter->latency() / (1024 * 1024)) << "ms     \033[33m[count]\033[39m "
            << putCounter->count() << std::endl;
    std::cout << "\033[33m[get latency]\033[39m " << std::fixed << std::setprecision(3)
            << (getCounter->latency() / (1024 * 1024)) << "ms     \033[33m[count]\033[39m "
            << getCounter->count() << std::endl;
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
