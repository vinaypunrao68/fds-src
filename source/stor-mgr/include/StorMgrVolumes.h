/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_STORMGRVOLUMES_H_
#define SOURCE_STOR_MGR_INCLUDE_STORMGRVOLUMES_H_

#include <boost/shared_ptr.hpp>
#include <list>
#include <map>
#include <memory>
#include <functional>
#include <unordered_map>

#include "fdsp/FDSP_types.h"
#include "fds_error.h"
#include "fds_types.h"
#include "fds_volume.h"
#include "qos_ctrl.h"
#include "util/Log.h"
#include "concurrency/RwLock.h"
#include "odb.h"
#include "util/counter.h"
#include "SmIo.h"

/* defaults */
#define FDS_DEFAULT_VOL_UUID 1

/*
 * Forward declaration of SM control class
 */
namespace fds {

class ObjectStorMgr;

class SmVolQueue : public FDS_VolumeQueue {
    private:
     fds_volid_t  volUuid;
     fds_uint32_t qDepth;

    public:
     SmVolQueue(fds_volid_t  _volUuid,
                fds_uint32_t _q_cap,
                fds_uint32_t _iops_max,
                fds_uint32_t _iops_min,
                fds_uint32_t _priority) :
         FDS_VolumeQueue(_q_cap,
                         _iops_max,
                         _iops_min,
                         _priority) {
             volUuid = _volUuid;
             /*
              * TODO: The queue depth should be computed
              * from the volume's parameters. Just make
              * something up for now for testing.
              */
             qDepth  = 10;

             FDS_VolumeQueue::activate();
         }

     fds_volid_t getVolUuid() const {
         return volUuid;
     }
     fds_uint32_t getQDepth() const {
         return qDepth;
     }
};

class StorMgrVolume : public FDS_Volume, public HasLogger {
     /*
      * Put SM specific volume info here.
      */

     /*
      * Queue for handling requests to this
      * volume. This queue is used by SM's
      * QoS manager.
      */
     SmVolQueue *volQueue;
     bool volQueueOwner_;

    public:
     /*
      * perf stats 
      */
     std::atomic<fds_uint64_t> ssdByteCnt;
     std::atomic<fds_uint64_t> hddByteCnt;
     /**
      * Bytes deduped since the service was started
      * TODO(xxx) make it actual deduped bytes for this volume
      * -- need to load from DB
      */
     double dedupBytes_;

     /*
      *  per volume stats 
      */
     CounterHist8bit  objStats;
     fds_uint64_t   averageObjectsRead;
     boost::posix_time::ptime lastAccessTimeR;

     inline fds_bool_t isSnapshot() const {
         return voldesc->isSnapshot();
     }

     fds_volid_t getVolId() const {
         return voldesc->volUUID;
     }

     SmVolQueue* getQueue() const {
         return volQueue;
     }

     bool isQosQueueOwner() const {
         return volQueueOwner_;
     }

     StorMgrVolume(const VolumeDesc& vdb,
                   fds_log *parent_log);
     ~StorMgrVolume();
     Error createVolIndexEntry(fds_volid_t vol_uuid,
                               fds_uint64_t vol_offset,
                               FDS_ObjectIdType objId,
                               fds_uint32_t data_obj_len);
     Error deleteVolIndexEntry(fds_volid_t vol_uuid,
                               fds_uint64_t vol_offset,
                               FDS_ObjectIdType objId);

     void updateDedupBytes(double dedup_bytes_added) {
         dedupBytes_ += dedup_bytes_added;
     }
     // at the moment, dedup bytes could be < 0 because
     // we may have SM coming from persistent state we are
     // not loading yet
     fds_uint64_t getDedupBytes() const {
         if (dedupBytes_ < 0) return 0;
         return dedupBytes_;
     }

     osm::ObjectDB  *volumeIndexDB;
     // VolumeObjectCache *vol_obj_cache;
};

class StorMgrVolumeTable : public HasLogger {
    typedef std::unordered_map<fds_volid_t, StorMgrVolume*> volume_map_t;

    public:
     /* A logger is created if not passed in */
     explicit StorMgrVolumeTable(ObjectStorMgr *sm);
     /* Use logger that passed in to the constructor */
     StorMgrVolumeTable(ObjectStorMgr *sm, fds_log *parent_log);
     /* Exposed for mock testing */
     StorMgrVolumeTable();
     ~StorMgrVolumeTable();
     Error updateVolStats(fds_volid_t vol_uuid);
     fds_uint32_t getVolAccessStats(fds_volid_t vol_uuid);
     void updateDupObj(fds_volid_t volid,
                       const ObjectID &objId,
                       fds_uint32_t obj_size,
                       fds_bool_t incr,
                       std::map<fds_volid_t, fds_uint32_t>& vol_refcnt);
     double getDedupBytes(fds_volid_t volid);

     inline fds_bool_t isSnapshot(fds_volid_t volid) {
         SCOPEDREAD(map_rwlock);
         volume_map_t::const_iterator iter = volume_map.find(volid);
         return volume_map.end() != iter && iter->second->isSnapshot();
     }

     Error registerVolume(const VolumeDesc& vdb);
     Error deregisterVolume(fds_volid_t vol_uuid);
     StorMgrVolume* getVolume(fds_volid_t vol_uuid);

     Error createVolIndexEntry(fds_volid_t      vol_uuid,
                               fds_uint64_t     vol_offset,
                               FDS_ObjectIdType objId,
                               fds_uint32_t     data_obj_len);
     Error deleteVolIndexEntry(fds_volid_t vol_uuid,
                               fds_uint64_t vol_offset,
                               FDS_ObjectIdType objId);

     std::list<fds_volid_t> getVolList() {
         std::list<fds_volid_t> volIds;
         map_rwlock.read_lock();
         for (volume_map_t::const_iterator cit = volume_map.cbegin();
              cit != volume_map.cend();
              cit++) {
             volIds.push_back((*cit).first);
         }
         map_rwlock.read_unlock();

         return volIds;
     }

    private:
     /* Reference to parent SM instance */
     ObjectStorMgr *parent_sm;

     /* volume uuid -> StorMgrVolume map */
     volume_map_t volume_map;

     /* Protects volume_map */
     fds_rwlock map_rwlock;

     /*
      * Pointer to logger to use 
      */
     fds_bool_t created_log;
};

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_STORMGRVOLUMES_H_
