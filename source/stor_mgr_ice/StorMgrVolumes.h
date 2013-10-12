/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_ICE_STORMGRVOLUMES_H_
#define SOURCE_STOR_MGR_ICE_STORMGRVOLUMES_H_

#include <Ice/Ice.h>
#include <IceUtil/IceUtil.h>

/* TODO: move this to interface file in include dir */
#include <lib/OMgrClient.h>

#include <unordered_map>

#include <fdsp/FDSP.h>
#include <include/fds_err.h>
#include <include/fds_types.h>
#include <include/fds_volume.h>
#include <util/Log.h>
#include <concurrency/RwLock.h>
#include "stor_mgr_ice/odb.h"

/* defaults */
#define FDS_DEFAULT_VOL_UUID 1

/*
 * Forward declaration of SM control class
 */
namespace fds {

  class ObjectStorMgr;

  class StorMgrVolume : public FDS_Volume {
 public:
    fds_volid_t vol_uuid;
    VolumeDesc *vol_desc;
    StorMgrVolume(const VolumeDesc&  vdb,
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

    /* Reference to parent SM instance */
    ObjectStorMgr *parent_sm;
  };

  class StorMgrVolumeTable {
 public:
    /* A logger is created if not passed in */
    explicit StorMgrVolumeTable(ObjectStorMgr *sm);
    /* Use logger that passed in to the constructor */
    StorMgrVolumeTable(ObjectStorMgr *sm, fds_log *parent_log);
    ~StorMgrVolumeTable();

    Error registerVolume(VolumeDesc vdb);
    Error deregisterVolume(fds_volid_t vol_uuid);
    StorMgrVolume* getVolume(fds_volid_t vol_uuid);

    Error createVolIndexEntry(fds_volid_t      vol_uuid,
                              fds_uint64_t     vol_offset,
                              FDS_ObjectIdType objId,
                              fds_uint32_t     data_obj_len);
    Error deleteVolIndexEntry(fds_volid_t vol_uuid,
                              fds_uint64_t vol_offset,
                              FDS_ObjectIdType objId);

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
}  // namespace fds

#endif  // SOURCE_STOR_MGR_ICE_STORMGRVOLUMES_H_
