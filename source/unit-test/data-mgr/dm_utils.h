/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_UNIT_TEST_DATA_MGR_DM_UTILS_H_
#define SOURCE_UNIT_TEST_DATA_MGR_DM_UTILS_H_
#include <boost/shared_ptr.hpp>
#include <leveldb/write_batch.h>

#include <fds_types.h>
#include <ObjectId.h>
#include <fds_process.h>
#include <util/timeutils.h>
#include <dm-platform.h>
#include <DataMgr.h>
#include <fdsp/fds_service_types.h>
#include <string>
#include <vector>

static Error expungeObjects(fds_volid_t volId, const std::vector<ObjectID> & oids) {
    /*
    for (auto i : oids) {
        std::cout << "Expunged volume: '" << volId << "' object: '"
                << i << "'" << std::endl;
    }
    */
    return ERR_OK;
}

#define BIND_OBJ_CALLBACK(obj, func, header , ...)                       \
    std::bind(&func, &obj , header, ##__VA_ARGS__ , std::placeholders::_1, \
              std::placeholders::_2);

struct DMCallback {
    boost::shared_ptr<fpi::AsyncHdr> asyncHdr;
    Error e;
    fds::dmCatReq *req;
    void handler(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr, const fds::Error &e, fds::dmCatReq *req) { //NOLINT
        this->asyncHdr = asyncHdr;
        this->e = e;
        this->req = req;
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

        boost::shared_ptr<VolumeDesc> vdesc(new VolumeDesc(name, ++volCount));

        vdesc->tennantId = i;
        vdesc->localDomainId = i;
        vdesc->globDomainId = i;

        vdesc->volType = fpi::FDSP_VOL_S3_TYPE;
        // vdesc->volType = fpi::FDSP_VOL_BLKDEV_TYPE;
        vdesc->capacity = 10 * 1024;  // 10GB
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

#endif  // SOURCE_UNIT_TEST_DATA_MGR_DM_UTILS_H_
