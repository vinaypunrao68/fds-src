/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include "StorMgr.h"
#include "odb.h"
#include "StorMgrVolumes.h"

extern ObjectStorMgr *objStorMgr;

namespace fds {


StorMgrVolume::StorMgrVolume(VolumeDesc &vdb, ObjectStorMgr *sm, fds_log *parent_log)
  : FDS_Volume(), parent_sm(sm)
{
std::string filename;
  volUUID = vdb.GetID();
  /* all other values are default for now */
  filename= sm->stor_prefix + "SNodeVolIndex";
  volumeIndexDB  = new ObjectDB(filename);
}

StorMgrVolume::~StorMgrVolume()
{
  delete volumeIndexDB;
}


/***** StorMgrVolumeTable methods ******/

/* creates its own logger */
StorMgrVolumeTable::StorMgrVolumeTable(ObjectStorMgr *sm, fds_log *parent_log)
  : parent_sm(sm)
{
  if (parent_log) {
    vt_log = parent_log;
    created_log = false;
  }
  else {
    vt_log = new fds_log("sm_voltab", "logs");
    created_log = true;
  }

  /* register for volume-related control events from OM*/
  if (parent_sm->omClient) {
    parent_sm->omClient->registerEventHandlerForVolEvents((volume_event_handler_t)volumeEventHandler);
  }

}

StorMgrVolumeTable::StorMgrVolumeTable(ObjectStorMgr *sm)
  : StorMgrVolumeTable(sm, NULL) {
}

StorMgrVolumeTable::~StorMgrVolumeTable()
{
  /*
   * Iterate volume_map and free the volume pointers
   */
  map_rwlock.write_lock();
  for (std::unordered_map<fds_volid_t, StorMgrVolume*>::iterator it = volume_map.begin();
       it != volume_map.end();
       ++it)
    {
      delete it->second;
    }
  volume_map.clear();
  map_rwlock.write_unlock();

  /*
   * Delete log if we created one
   */
  if (created_log) {
    delete vt_log;
  }
}


/*
 * Creates volume if it has not been created yet
 * Does nothing if volume is already registered
 */
Error StorMgrVolumeTable::registerVolume(VolumeDesc vdb)
{
  Error err(ERR_OK);
  fds_volid_t vol_uuid = vdb.GetID();
  map_rwlock.write_lock();
  if (volume_map.count(vol_uuid) < 1) {
    StorMgrVolume* new_vol = new StorMgrVolume( vdb, parent_sm, vt_log);
    if (new_vol) {
      volume_map[vol_uuid] = new_vol;
    }
    else {
      err = ERR_INVALID_ARG; // TODO: need more error types
    }
  }
  map_rwlock.write_unlock();

  FDS_PLOG(vt_log) << "StorMgrVolumeTable - Register new volume " << vol_uuid
		   << " result: " << err.GetErrstr();  

  return err;
}

/*
 * Returns SH volume object or NULL if the volume has not been created
 * TODO: another thread may delete the volume since we release the read lock,
 * so we should revisit this when we implement volume delete
 */
StorMgrVolume* StorMgrVolumeTable::getVolume(fds_volid_t vol_uuid)
{
   StorMgrVolume *ret_vol = NULL;

   map_rwlock.read_lock();
   if (volume_map.count(vol_uuid) > 0) {
     ret_vol = volume_map[vol_uuid];
   }
   else {
     FDS_PLOG(vt_log) << "StorMgrVolumeTable::getVolume - Volume " << vol_uuid
		      << " does not exist";    
   }
   map_rwlock.read_unlock();
 
   return ret_vol;
}

Error  StorMgrVolumeTable::deregisterVolume(fds_volid_t vol_uuid) {
Error err(ERR_OK);
StorMgrVolume *vol = NULL;

   map_rwlock.write_lock();
   if (volume_map.count(vol_uuid) == 0) {
       FDS_PLOG(vt_log) << "StorMgrVolumeTable - deregistering volume called for non-existing volume "
                        << vol_uuid;
       err = ERR_INVALID_ARG;
       map_rwlock.write_unlock();
      return err;
   }
         
   vol = volume_map[vol_uuid];
   volume_map.erase(vol_uuid);
   map_rwlock.write_unlock();
           
   delete vol;
           
   FDS_PLOG(vt_log) << "StorMgrVolumeTable - Removed volume "
                   << vol_uuid;
 
  return err;
}

/*
 * Handler for volume-related control message from OM
 */
void StorMgrVolumeTable::volumeEventHandler( fds_volid_t vol_uuid, VolumeDesc *vdb, fds_vol_notify_t vol_action)
{
Error err(ERR_OK);
  switch (vol_action) {
      case fds_notify_vol_add:
        FDS_PLOG(objStorMgr->GetLog()) << "StorMgrVolumeTable - Received volume attach event from OM"
					       << " for volume " << vol_uuid;
        err = objStorMgr->volTbl->registerVolume(vdb ? *vdb : VolumeDesc("", vol_uuid));
        break;

     case fds_notify_vol_rm:
        FDS_PLOG(objStorMgr->GetLog()) << "StorMgrVolumeTable - Received volume detach event from OM"
				       << " for volume " << vol_uuid;
        err = objStorMgr->volTbl->deregisterVolume( vdb->GetID());
        break;

    default:
        FDS_PLOG(objStorMgr->GetLog()) << "StorMgrVolumeTable - Received unexpected volume event from OM"
                                      << " for volume " << vol_uuid;
        break;
  } 
}


} // namespace fds
