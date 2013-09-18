/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef __STOR_HV_VOLS_H_
#define __STOR_HV_VOLS_H_

#include <lib/Ice-3.5.0/cpp/include/Ice/Ice.h>
#include <lib/Ice-3.5.0/cpp/include/IceUtil/IceUtil.h>

#include <unordered_map>

#include "fdsp/FDSP.h"
#include "include/fds_err.h"
#include "include/fds_types.h"
#include "include/fds_volume.h"
#include "util/Log.h"
#include "util/concurrency/RwLock.h"
#include "odb.h"
#include "lib/OMgrClient.h"


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
  StorMgrVolume(VolumeDesc&  vdb, ObjectStorMgr *sm, fds_log *parent_log);
  ~StorMgrVolume();


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
