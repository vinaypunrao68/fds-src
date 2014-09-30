/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_UNIT_TEST_DATA_MGR_DM_GTEST_H_
#define SOURCE_UNIT_TEST_DATA_MGR_DM_GTEST_H_

#define GTEST_USE_OWN_TR1_TUPLE 0

#include <unistd.h>
#include <stdlib.h>

#include <vector>
#include <string>

#include <boost/shared_ptr.hpp>

#include <leveldb/write_batch.h>

#include <fds_types.h>
#include <ObjectId.h>
#include <fds_process.h>
#include <util/timeutils.h>
#include <dm-platform.h>
#include <DataMgr.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;  // NOLINT
namespace po = boost::program_options;

static fds_uint32_t MAX_OBJECT_SIZE = 2097152;    // 2MB
static fds_uint64_t BLOB_SIZE = 1 * 1024 * 1024 * 1024;   // 1GB
static fds_uint32_t NUM_VOLUMES = 1;
static fds_uint32_t NUM_BLOBS = 1;

static const fds_uint32_t NUM_TAGS = 10;

static fds_bool_t PUTS_ONLY = false;
static fds_bool_t NO_DELETE = false;

static std::atomic<fds_uint32_t> blobCount;
static std::atomic<fds_uint64_t> txCount;

namespace fds {
DataMgr * dataMgr = 0;
}

static Error expungeObjects(fds_volid_t volId, const std::vector<ObjectID> & oids) {
    /*
    for (auto i : oids) {
        std::cout << "Expunged volume: '" << volId << "' object: '"
                << i << "'" << std::endl;
    }
    */
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
        for (fds_uint32_t i = 0; i < NUM_TAGS; i++) {
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

boost::shared_ptr<LatencyCounter> putCounter(new LatencyCounter("put", 0, 0));
boost::shared_ptr<LatencyCounter> getCounter(new LatencyCounter("get", 0, 0));
boost::shared_ptr<LatencyCounter> deleteCounter(new LatencyCounter("delete", 0, 0));

void generateVolumes(std::vector<boost::shared_ptr<VolumeDesc> > & volumes) {
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
    }
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

#endif  // SOURCE_UNIT_TEST_DATA_MGR_DM_GTEST_H_
