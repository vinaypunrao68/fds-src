/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include "StorMgr.h"
#include <odb.h>
#include "StorMgrVolumes.h"

namespace fds {

extern ObjectStorMgr *objStorMgr;

StorMgrVolume::StorMgrVolume(const VolumeDesc&  vdb,
                             ObjectStorMgr     *sm,
                             fds_log           *parent_log)
  : FDS_Volume(vdb),
    parent_sm(sm) {
  /*
   * Setup storage prefix.
   * All other values are default for now
   */
  std::string filename;
  filename= sm->getStorPrefix() + "SNodeVolIndex" + std::to_string(vdb.volUUID);

  /*
   * Create volume queue with parameters based on
   * the volume descriptor.
   * TODO: The queue capacity is still hard coded. We
   * should calculate this somehow.
   */
  volQueue = new SmVolQueue(voldesc->GetID(),
                            50,
                            voldesc->getIopsMax(),
                            voldesc->getIopsMax(),
                            voldesc->getPriority());

  volumeIndexDB  = new ObjectDB(filename);
}

StorMgrVolume::~StorMgrVolume() {
  /*
   * TODO: Should do some sort of checking/cleanup
   * if the volume queue isn't empty.
   */
  delete volQueue;
  delete volumeIndexDB;
}

/**
 * Stores a mapping from a volume's storage location to the
 * object at that location.
 */
Error StorMgrVolume::createVolIndexEntry(fds_volid_t      vol_uuid, 
                                         fds_uint64_t     vol_offset, 
                                         FDS_ObjectIdType objId, 
                                         fds_uint32_t     data_obj_len) {
  Error err(ERR_OK);
  DiskLoc diskloc;
  diskloc.vol_id = vol_uuid;
  diskloc.file_id = 0;
  diskloc.offset = vol_offset;
  ObjectID oid(objId.hash_high,
               objId.hash_low);

  FDS_PLOG(objStorMgr->GetLog()) << "createVolIndexEntry Obj ID:" << objId.hash_high
                                 << ":"<< objId.hash_low  << "glob_vol_id:"
                                 << vol_uuid << "offset" << vol_offset;

   err = volumeIndexDB->Put(diskloc, oid);
   return err;
}

  

Error StorMgrVolume::deleteVolIndexEntry(fds_volid_t      vol_uuid, 
                                         fds_uint64_t     vol_offset,
                                         FDS_ObjectIdType objId) {
Error err(ERR_OK);
  DiskLoc diskloc;
  diskloc.vol_id = vol_uuid;
  diskloc.file_id = 0;
  diskloc.offset = vol_offset;
  ObjectID oid(objId.hash_high,
               objId.hash_low);

  FDS_PLOG(objStorMgr->GetLog()) << "deleteVolIndexEntry Obj ID:" << objId.hash_high << ":" << objId.hash_low << "glob_vol_id:" << vol_uuid << "offset" << vol_offset;
   err = volumeIndexDB->Delete(diskloc);
   return err;
}


/***** StorMgrVolumeTable methods ******/

/* creates its own logger */
StorMgrVolumeTable::StorMgrVolumeTable(ObjectStorMgr *sm, fds_log *parent_log)
  : parent_sm(sm) {
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

StorMgrVolumeTable::~StorMgrVolumeTable() {
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
Error
StorMgrVolumeTable::registerVolume(const VolumeDesc& vdb) {
  Error       err(ERR_OK);
  fds_volid_t volUuid = vdb.GetID();

  map_rwlock.write_lock();
  if (volume_map.count(volUuid) == 0) {
    FDS_PLOG(vt_log) << "Registering new volume " << volUuid;
    StorMgrVolume* vol = new StorMgrVolume(vdb,
                                           parent_sm,
                                           vt_log);
    fds_assert(vol != NULL);
    volume_map[volUuid] = vol;
  } else {
    /*
     * TODO: Should probably compare the known volume's details with
     * the one we're being told about.
     */
    FDS_PLOG(vt_log) << "Register already known volume " << volUuid;
  }
  map_rwlock.write_unlock();

  return err;
}

/*
 * TODO: another thread may delete the volume since we release the read lock,
 * so we should revisit this when we implement volume delete
 */
StorMgrVolume* StorMgrVolumeTable::getVolume(fds_volid_t vol_uuid)
{
  StorMgrVolume *ret_vol = NULL;

  map_rwlock.read_lock();
  if (volume_map.count(vol_uuid) > 0) {
    ret_vol = volume_map[vol_uuid];
  }  else {
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


Error StorMgrVolumeTable::createVolIndexEntry(fds_volid_t vol_uuid, 
                                              fds_uint64_t vol_offset, 
                                              FDS_ObjectIdType objId, 
                                              fds_uint32_t data_obj_len) {
  Error err(ERR_OK);
  StorMgrVolume *vol;
  map_rwlock.write_lock();
  if (volume_map.count(vol_uuid) == 0) {
    FDS_PLOG(vt_log) << "StorMgrVolumeTable - createVolIndexEntry volume "
                     << "called for non-existing volume " << vol_uuid;
    err = ERR_INVALID_ARG;
    map_rwlock.write_unlock();

    return err;
  }

  vol = volume_map[vol_uuid];
  vol->createVolIndexEntry(vol_uuid, vol_offset, objId, data_obj_len);
  map_rwlock.write_unlock();

  return err;     
}


Error StorMgrVolumeTable::deleteVolIndexEntry(fds_volid_t vol_uuid,
                                              fds_uint64_t vol_offset, 
                                              FDS_ObjectIdType objId) {
  Error err(ERR_OK);
  StorMgrVolume *vol;
  map_rwlock.write_lock();
  if (volume_map.count(vol_uuid) == 0) {
    FDS_PLOG(vt_log) << "StorMgrVolumeTable - deleteVolIndexEntry volume "
                     << "called for non-existing volume " << vol_uuid;
    err = ERR_INVALID_ARG;
    map_rwlock.write_unlock();
    return err;
  }

  vol = volume_map[vol_uuid];
  vol->deleteVolIndexEntry(vol_uuid, vol_offset, objId);
  map_rwlock.write_unlock();

  return err;
}
}  // namespace fds
