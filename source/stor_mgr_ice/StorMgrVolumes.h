/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_ICE_STORMGRVOLUMES_H_
#define SOURCE_STOR_MGR_ICE_STORMGRVOLUMES_H_

#include <Ice/Ice.h>
#include <IceUtil/IceUtil.h>

/* TODO: move this to interface file in include dir */
#include <lib/OMgrClient.h>

#include <list>
#include <unordered_map>

#include <fdsp/FDSP.h>
#include <include/fds_err.h>
#include <include/fds_types.h>
#include <include/fds_volume.h>
#include <util/Log.h>
#include <concurrency/RwLock.h>
#include <odb.h>
#include <include/qos_ctrl.h>

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

  class StorMgrVolume : public FDS_Volume {
 private:
    /*
     * Put SM specific volume info here.
     */

    /*
     * Queue for handling requests to this
     * volume. This queue is used by SM's
     * QoS manager.
     */
    SmVolQueue *volQueue;

    /*
     * Reference to parent SM instance
     */
    ObjectStorMgr *parent_sm;
 public:
    fds_volid_t getVolId() const {
      return voldesc->volUUID;
    }

    SmVolQueue* getQueue() const {
      return volQueue;
    }

    StorMgrVolume(const VolumeDesc& vdb,
                  ObjectStorMgr *sm,
                  fds_log *parent_log);
    ~StorMgrVolume();
    Error createVolIndexEntry(fds_volid_t vol_uuid,
                              fds_uint64_t vol_offset,
                              FDS_ObjectIdType objId,
                              fds_uint32_t data_obj_len);
    Error deleteVolIndexEntry(fds_volid_t vol_uuid,
                              fds_uint64_t vol_offset,
                              FDS_ObjectIdType objId);

    osm::ObjectDB  *volumeIndexDB;
    // VolumeObjectCache *vol_obj_cache;
  };

  class StorMgrVolumeTable {
 public:
    /* A logger is created if not passed in */
    explicit StorMgrVolumeTable(ObjectStorMgr *sm);
    /* Use logger that passed in to the constructor */
    StorMgrVolumeTable(ObjectStorMgr *sm, fds_log *parent_log);
    ~StorMgrVolumeTable();

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
      for (std::unordered_map<fds_volid_t, StorMgrVolume*>::const_iterator cit = volume_map.cbegin();
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

    /* handler for volume-related control message from OM */
    static void volumeEventHandler(fds_volid_t vol_uuid,
                                   VolumeDesc *vdb,
                                   fds_vol_notify_t vol_action);

    /* volume uuid -> StorMgrVolume map */
    std::unordered_map<fds_volid_t, StorMgrVolume*> volume_map;

    /* Protects volume_map */
    fds_rwlock map_rwlock;

    /*
     * Pointer to logger to use 
     */
    fds_log *vt_log;
    fds_bool_t created_log;
  };
  
  class SmIoReq : public FDS_IOType {
 private:
    ObjectID     objId;
    ObjectBuf    objData;
    fds_volid_t  volUuid;
    fds_uint64_t volOffset;

 public:
    /*
     * This constructor is generally used for
     * write since it accepts an objBuf.
     * TODO: Wrap this up in a clear interface.
     */
    SmIoReq(fds_uint64_t       _objIdHigh,
            fds_uint64_t       _objIdLow,
            const std::string& _dataStr,
            fds_volid_t        _volUuid,
            fds_io_op_t        _ioType) {
      objId = ObjectID(_objIdHigh, _objIdLow);
      objData.size        = _dataStr.size();
      objData.data        = _dataStr;
      volUuid             = _volUuid;
      io_vol_id           = volUuid;
      FDS_IOType::io_type = _ioType;
    }

    /*
     * This constructor is generally used for
     * read since it takes a request tracking
     * ID.
     * TODO: Wrap this up in a clear interface.
     */
    SmIoReq(fds_uint64_t       _objIdHigh,
            fds_uint64_t       _objIdLow,
            const std::string& _dataStr,
            fds_volid_t        _volUuid,
            fds_io_op_t        _ioType,
            fds_uint32_t       _ioReqId) {
      objId = ObjectID(_objIdHigh, _objIdLow);
      objData.size        = _dataStr.size();
      objData.data        = _dataStr;
      volUuid             = _volUuid;
      io_vol_id           = volUuid;
      FDS_IOType::io_type = _ioType;
      io_req_id           = _ioReqId;
    }
    ~SmIoReq() {
    }

    const ObjectID& getObjId() const {
      return objId;
    }

    const ObjectBuf& getObjData() const {
      return objData;
    }

    fds_volid_t getVolId() const {
      return volUuid;
    }
  };
}  // namespace fds

#endif  // SOURCE_STOR_MGR_ICE_STORMGRVOLUMES_H_
