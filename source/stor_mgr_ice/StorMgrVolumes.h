/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef __STOR_HV_VOLS_H_
#define __STOR_HV_VOLS_H_

#include <Ice/Ice.h>
#include <IceUtil/IceUtil.h>

#include <unordered_map>

#include <fdsp/FDSP.h>
#include <fds_err.h>
#include <fds_types.h>
#include <fds_volume.h>
#include <util/Log.h>
#include <concurrency/RwLock.h>
#include "odb.h"

/* TODO: move this to interface file in include dir */
#include <lib/OMgrClient.h>
#include "StorMgrVolumes.h"


/* defaults */
#define FDS_DEFAULT_VOL_UUID 1


/*
 * Forward declaration of SM control class
 */
class ObjectStorMgr;
using namespace osm;

namespace fds {

class StorMgrVolume : public FDS_Volume
{
public:
  fds_volid_t vol_uuid;
  VolumeDesc *vol_desc;
  StorMgrVolume(VolumeDesc&  vdb, ObjectStorMgr *sm, fds_log *parent_log);
  ~StorMgrVolume();
  Error createVolIndexEntry(fds_volid_t& vol_uuid, 
                      fds_uint64_t vol_offset, 
                      FDS_ObjectIdType& objId, 
                     fds_uint32_t data_obj_len);
  Error deleteVolIndexEntry(fds_volid_t& vol_uuid, 
                      fds_uint64_t vol_offset,
                      FDS_ObjectIdType& objId);


  ObjectDB  *volumeIndexDB;
  //VolumeObjectCache *vol_obj_cache;

  /* Reference to parent SM instance */
  ObjectStorMgr *parent_sm;
};

class StorMgrVolumeTable
{
 public:
  /* A logger is created if not passed in */
  StorMgrVolumeTable(ObjectStorMgr *sm);
  /* Use logger that passed in to the constructor */
  StorMgrVolumeTable(ObjectStorMgr *sm, fds_log *parent_log);
  ~StorMgrVolumeTable();

  Error registerVolume(VolumeDesc vdb);
  Error deregisterVolume(fds_volid_t vol_uuid);
  StorMgrVolume* getVolume(fds_volid_t vol_uuid);
  
   Error createVolIndexEntry(fds_volid_t      vol_uuid,
                             fds_uint64_t      vol_offset,
                             FDS_ObjectIdType objId,
                             fds_uint32_t      data_obj_len);
   Error deleteVolIndexEntry(fds_volid_t vol_uuid, 
                             fds_uint64_t vol_offset,
                             FDS_ObjectIdType objId);

 private: /* methods */ 
  ObjectStorMgr *parent_sm;

  /* handler for volume-related control message from OM */
  static void volumeEventHandler(fds_volid_t vol_uuid, VolumeDesc *vdb, fds_vol_notify_t vol_action);
  

  /* volume uuid -> StorMgrVolume map */
  std::unordered_map<fds_volid_t, StorMgrVolume*> volume_map;   

  /* Protects volume_map */
  fds_rwlock map_rwlock;
  
  /* Reference to parent SM instance */

  /*
   * Pointer to logger to use 
   */
  fds_log *vt_log;
  fds_bool_t created_log;
};

} // namespace fds

#endif // __STOR_HV_VOLS_H_
