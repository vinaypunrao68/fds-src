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
#include <DataMgr.h>
#include <fdsp/svc_types_types.h>
#include <string>
#include <vector>
#include <concurrency/taskstatus.h>
#include <util/color.h>
#include <map>
#include <util/stats.h>

using fds::util::Color;
std::map<std::string, fds::util::Stats> statsMap;
fds_mutex mtx;

extern fds::concurrency::TaskStatus taskCount;

void printStats() {
    FDSGUARD(mtx);
    for (auto& iter : statsMap) {
        auto& stats = iter.second;
        stats.calculate();
        if (stats.getCount() > 1) {
            std::cout << Color::Yellow << "[" << std::setw(10) << iter.first << "] " << Color::End
                      << std::fixed << std::setprecision(3)
                      << Color::Green << "["
                      << " count:" << stats.getCount()
                      << " median:" << stats.getMedian()
                      << " avg:" << stats.getAverage()
                      << " stddev:" << stats.getStdDev()
                      << " min:" << stats.getMin()
                      << " max:" << stats.getMax()
                      << " ]" << Color::End << std::endl;
        } else {
            std::cout << Color::Yellow << "[" << std::setw(10) << iter.first << "] " << Color::End
                      << std::fixed << std::setprecision(3)
                      << Color::Green << stats.getAverage() << Color::End << std::endl;
        }
    }
    statsMap.clear();
}

struct TimePrinter {
    std::string name;
    fds::util::StopWatch stopwatch;
    bool done = false;
    TimePrinter() {}
    explicit TimePrinter(std::string name) :  name(name){
        stopwatch.start();
    }
    void summary() {
        std::cout << Color::Yellow << "[" << std::setw(10) << name << "] " << Color::End
                  << std::fixed << std::setprecision(3)
                  << Color::Red
                  << (stopwatch.getElapsedNanos()/(1000.0*1000)) << " ms"
                  << Color::End
                  << std::endl;
    }

    ~TimePrinter() {
        if (done) {
            // summary();
            FDSGUARD(mtx);
            statsMap[name].add(stopwatch.getElapsedNanos()/(1000000.0));
        }
    }
};

std::map<std::string, TimePrinter> timeMap;

#define TIMEDSECTION(NAME, ...) timeMap[NAME].stopwatch.reset();  timeMap[NAME].name = NAME; \
    __VA_ARGS__ ; timeMap[NAME].summary()

#define TIMEDBLOCK(NAME) for (TimePrinter __tp__(NAME); !__tp__.done ; __tp__.done = true)
#define TIMEDOUTERBLOCK(NAME) for (TimePrinter __otp__(NAME); !__otp__.done ; __otp__.done = true)

static Error expungeObjects(fds_volid_t volId, const std::vector<ObjectID> & oids, bool force) {
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

#define DEFINE_SHARED_PTR(TYPE, NAME) \
    boost::shared_ptr<fpi::TYPE> NAME(new fpi::TYPE());

struct DMCallback {
    boost::shared_ptr<fpi::AsyncHdr> asyncHdr;
    Error e;
    fds::dmCatReq *req;
    fds::concurrency::TaskStatus taskStatus;

    void reset() {
        taskStatus.reset(1);
    }

    bool wait(ulong timeout = 0) {
        if (timeout > 0) {
            return taskStatus.await(timeout);
        } else {
            taskStatus.await();
            return true;
        }
    }

    void handler(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr, const fds::Error &e, fds::dmCatReq *req) { //NOLINT
        this->asyncHdr = asyncHdr;
        this->e = e;
        this->req = req;
        taskStatus.done();
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

boost::shared_ptr<LatencyCounter> putCounter(new LatencyCounter("put", invalid_vol_id, 0));
boost::shared_ptr<LatencyCounter> getCounter(new LatencyCounter("get", invalid_vol_id, 0));
boost::shared_ptr<LatencyCounter> deleteCounter(new LatencyCounter("delete", invalid_vol_id, 0));

void generateVolumes(std::vector<boost::shared_ptr<VolumeDesc> > & volumes) {
    for (fds_uint32_t i = 1; i <= NUM_VOLUMES; ++i) {
        std::string name = "test" + std::to_string(i);

        boost::shared_ptr<VolumeDesc> vdesc(new VolumeDesc(name, fds_volid_t(++volCount)));

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
        vdesc->backupVolume = invalid_vol_id.get();

        vdesc->iops_assured = 1000;
        vdesc->iops_throttle = 5000;
        vdesc->relativePrio = 1;

        vdesc->fSnapshot = false;
        vdesc->srcVolumeId = invalid_vol_id;
        vdesc->lookupVolumeId = invalid_vol_id;
        vdesc->qosQueueId = invalid_vol_id;

        volumes.push_back(vdesc);
    }
}

#endif  // SOURCE_UNIT_TEST_DATA_MGR_DM_UTILS_H_
