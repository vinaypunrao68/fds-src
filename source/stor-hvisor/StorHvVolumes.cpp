#include <atomic>

/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "StorHvisorNet.h"
#include "StorHvJournal.h"
#include "StorHvVolumes.h"
#include "StorHvisorCPP.h"
#include "StorHvQosCtrl.h"

extern StorHvCtrl *storHvisor;

namespace fds {

StorHvVolume::StorHvVolume(const VolumeDesc& vdesc, StorHvCtrl *sh_ctrl, fds_log *parent_log)
  : FDS_Volume(vdesc), parent_sh(sh_ctrl)
{
  journal_tbl = new StorHvJournal(FDS_READ_WRITE_LOG_ENTRIES);
  vol_catalog_cache = new VolumeCatalogCache(voldesc->volUUID, sh_ctrl, parent_log);  

  if ( voldesc->volType == FDSP_VOL_BLKDEV_TYPE && parent_sh->GetRunTimeMode() == StorHvCtrl::NORMAL) { 
      blkdev_minor = hvisor_create_blkdev(voldesc->volUUID, voldesc->capacity);
  }

  volQueue = new FDS_VolumeQueue(4096, vdesc.iops_max, vdesc.iops_min, vdesc.relativePrio);
  volQueue->activate();

  is_valid = true;
}

StorHvVolume::~StorHvVolume()
{
  parent_sh->qos_ctrl->deregisterVolume(voldesc->volUUID);
  if (journal_tbl)
    delete journal_tbl;
  if (vol_catalog_cache)
    delete vol_catalog_cache;
  if(volQueue) {
    delete volQueue;
  }
  
}

/* safely destroys journal table and volume catalog cache
 * after this, object is not valid */
void StorHvVolume::destroy()
{
  rwlock.write_lock();
  if (!is_valid) {
    rwlock.write_unlock();
    return;
  }

  if ( voldesc->volType == FDSP_VOL_BLKDEV_TYPE && parent_sh->GetRunTimeMode() == StorHvCtrl::NORMAL) { 
     hvisor_delete_blkdev(blkdev_minor);
  }
  /* destroy data */
  delete journal_tbl;
  journal_tbl = NULL;
  delete vol_catalog_cache;
  vol_catalog_cache = NULL;

  is_valid = false;
  if ( volQueue) {
     delete volQueue;
     volQueue = NULL;
  }
  rwlock.write_unlock();
}

bool StorHvVolume::isValidLocked() const
{
  return is_valid;
}

void StorHvVolume::readLock()
{
  rwlock.read_lock();
}

void StorHvVolume::readUnlock()
{
  rwlock.read_unlock();
}

StorHvVolumeLock::StorHvVolumeLock(StorHvVolume *vol)
{
  shvol = vol;
  if (shvol) shvol->readLock();
}

StorHvVolumeLock::~StorHvVolumeLock()
{
  if (shvol) shvol->readUnlock();
}

/***** StorHvVolumeTable methods ******/

std::atomic<unsigned int> next_io_req_id;

/* creates its own logger */
StorHvVolumeTable::StorHvVolumeTable(StorHvCtrl *sh_ctrl, fds_log *parent_log)
  : parent_sh(sh_ctrl)
{
  if (parent_log) {
    vt_log = parent_log;
    created_log = false;
  }
  else {
    vt_log = new fds_log("voltab", "logs");
    created_log = true;
  }

  /* register for volume-related control events from OM*/
  if (parent_sh->om_client) {
    parent_sh->om_client->registerEventHandlerForVolEvents(volumeEventHandler);
  }

  next_io_req_id = ATOMIC_VAR_INIT(0);

  /* setup 'admin' queue that will hold requests to OM 
   * such as 'get bucket stats' and 'get bucket' */
  {
    VolumeDesc admin_vdesc("admin_vol", admin_vol_id);
    admin_vdesc.iops_min = 10;
    admin_vdesc.iops_max = 500; 
    admin_vdesc.relativePrio = 9;
    admin_vdesc.capacity = 0; /* not really a volume, using volume struct to hold admin requests  */
    StorHvVolume *admin_vol = new StorHvVolume(admin_vdesc, parent_sh, vt_log);
    if (admin_vol) {
      volume_map[admin_vol_id] = admin_vol;
      sh_ctrl->qos_ctrl->registerVolume(admin_vol_id, admin_vol->volQueue);
      FDS_PLOG_SEV(vt_log, fds::fds_log::notification) << "StorHvVolumeTable -- constructor registered admin volume";
    }
    else {
      FDS_PLOG_SEV(vt_log, fds::fds_log::error) << "StorHvVolumeTable -- failed to allocate admin volume struct";
    }
  }

  if (sh_ctrl->GetRunTimeMode() == StorHvCtrl::TEST_BOTH) {
    VolumeDesc vdesc("default_vol", FDS_DEFAULT_VOL_UUID);
    vdesc.iops_min = 200;
    vdesc.iops_max = 500;
    vdesc.relativePrio = 8;
    vdesc.capacity = 100000;
    StorHvVolume *vol = 
    volume_map[FDS_DEFAULT_VOL_UUID] = new StorHvVolume(vdesc, parent_sh, vt_log);
    sh_ctrl->qos_ctrl->registerVolume(vdesc.volUUID,vol->volQueue);
    FDS_PLOG(vt_log) << "StorHvVolumeTable - constructor registered volume 1";  

#if 0 // Test  volumes
    VolumeDesc vdesc2("default_vol2", 2);
    volume_map[2] = new StorHvVolume(vdesc2, parent_sh, vt_log);
    FDS_PLOG(vt_log) << "StorHvVolumeTable - constructor registered volume 2";  

    VolumeDesc vdesc3("default_vol3", 3);
    volume_map[3] = new StorHvVolume(vdesc3, parent_sh, vt_log);
    FDS_PLOG(vt_log) << "StorHvVolumeTable - constructor registered volume 3";  

    VolumeDesc vdesc4("default_vol4", 4);
    volume_map[4] = new StorHvVolume(vdesc4, parent_sh, vt_log);
    FDS_PLOG(vt_log) << "StorHvVolumeTable - constructor registered volume 4";  

    VolumeDesc vdesc5("default_vol5", 5);
    volume_map[5] = new StorHvVolume(vdesc5, parent_sh, vt_log);
    FDS_PLOG(vt_log) << "StorHvVolumeTable - constructor registered volume 5";  

    VolumeDesc vdesc6("default_vol6", 6);
    volume_map[6] = new StorHvVolume(vdesc6, parent_sh, vt_log);
    FDS_PLOG(vt_log) << "StorHvVolumeTable - constructor registered volume 6";  

    VolumeDesc vdesc7("default_vol7", 7);
    volume_map[7] = new StorHvVolume(vdesc7, parent_sh, vt_log);
    FDS_PLOG(vt_log) << "StorHvVolumeTable - constructor registered volume 7";  

    VolumeDesc vdesc8("default_vol8", 8);
    volume_map[8] = new StorHvVolume(vdesc8, parent_sh, vt_log);
    FDS_PLOG(vt_log) << "StorHvVolumeTable - constructor registered volume 8";  

    VolumeDesc vdesc9("default_vol9", 9);
    volume_map[9] = new StorHvVolume(vdesc9, parent_sh, vt_log);
    FDS_PLOG(vt_log) << "StorHvVolumeTable - constructor registered volume 9";  

    VolumeDesc vdesc10("default_vol10", 10);
    volume_map[10] = new StorHvVolume(vdesc10, parent_sh, vt_log);
    FDS_PLOG(vt_log) << "StorHvVolumeTable - constructor registered volume 10";  
#endif
  }

}

StorHvVolumeTable::StorHvVolumeTable(StorHvCtrl *sh_ctrl)
  : StorHvVolumeTable(sh_ctrl, NULL) {
}

StorHvVolumeTable::~StorHvVolumeTable()
{
  /*
   * Iterate volume_map and free the volume pointers
   */

  map_rwlock.write_lock();
  for (std::unordered_map<fds_volid_t, StorHvVolume*>::iterator it = volume_map.begin();
       it != volume_map.end();
       ++it)
    {
      StorHvVolume *vol = it->second;
      vol->destroy();
      delete vol;
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
Error StorHvVolumeTable::registerVolume(const VolumeDesc& vdesc)
{
    Error err(ERR_OK);
    fds_volid_t vol_uuid = vdesc.GetID();

    map_rwlock.write_lock();
    if (volume_map.count(vol_uuid) == 0) {
        StorHvVolume *new_vol = new StorHvVolume(vdesc, parent_sh, vt_log);
        if (new_vol) {
            // Register this volume queue with the QOS ctrl
            err = parent_sh->qos_ctrl->registerVolume(vdesc.volUUID, new_vol->volQueue);
            if (err.ok()) {
                volume_map[vol_uuid] = new_vol;
            } else {
                new_vol->destroy();
                delete new_vol;
            }
        }
        else {
            err = ERR_INVALID_ARG; // TODO: need more error types
        }
    }
    map_rwlock.write_unlock();
    
    FDS_PLOG_SEV(vt_log, fds::fds_log::notification)
            << "StorHvVolumeTable - Register new volume " << vdesc.name << " "
            << std::hex << vol_uuid << std::dec << ", policy " << vdesc.volPolicyId
            << " (iops_min=" << vdesc.iops_min << ",iops_max=" << vdesc.iops_max <<",prio=" << vdesc.relativePrio << ")"
            << " result: " << err.GetErrstr();  
    
    /* check if any blobs are waiting for volume to be registered, and if so,
     * move them to appropriate qos queue  */
    if (err.ok())
        moveWaitBlobsToQosQueue(vol_uuid, vdesc.name, err);

    return err;
}

Error StorHvVolumeTable::modifyVolumePolicy(fds_volid_t vol_uuid,
					    const VolumeDesc& vdesc)
{
  Error err(ERR_OK);
  StorHvVolume *vol = getLockedVolume(vol_uuid);
  if (vol && vol->volQueue)
    {
      /* update volume descriptor */
      (vol->voldesc)->modifyPolicyInfo(vdesc.iops_min,
				      vdesc.iops_max,
				      vdesc.relativePrio);
 
      /* notify appropriate qos queue about the change in qos params*/
      err = storHvisor->qos_ctrl->modifyVolumeQosParams(vol_uuid,
							vdesc.iops_min,
							vdesc.iops_max,
							vdesc.relativePrio);
    }
  else {
    err = Error(ERR_NOT_FOUND);
  }
  vol->readUnlock();

  FDS_PLOG_SEV(vt_log, fds::fds_log::notification) << "StorHvVolumeTable - modify policy info for volume " 
						   << vdesc.name
						   << " (iops_min=" << vdesc.iops_min 
						   << ",iops_max=" << vdesc.iops_max 
						   << ",prio=" << vdesc.relativePrio << ")"
						   << " RESULT " << err.GetErrstr();

  return err;
}

/*
 * Removes volume from the map, returns error if volume does not exist
 */
Error StorHvVolumeTable::removeVolume(fds_volid_t vol_uuid)
{
  Error err(ERR_OK);
  StorHvVolume *vol = NULL;

  map_rwlock.write_lock();
  if (volume_map.count(vol_uuid) == 0) {
    FDS_PLOG_SEV(vt_log, fds::fds_log::warning) << "StorHvVolumeTable - removeVolume called for non-existing volume "
                     << vol_uuid;
    err = ERR_INVALID_ARG;
    map_rwlock.write_unlock();
    return err;
  }

  vol = volume_map[vol_uuid];
  volume_map.erase(vol_uuid);
  map_rwlock.write_unlock();

  vol->destroy();
  delete vol;
  
  FDS_PLOG_SEV(vt_log, fds::fds_log::notification) << "StorHvVolumeTable - Removed volume "
                   << vol_uuid;
 
  return err;
}

/*
 * Returns SH volume object or NULL if the volume has not been created
 * Not thread-safe, so use StorHvVolumeLock to lock the volume and check
 * if volume object is still valid via StorHvVolume::isValidLocked()
 * before using the volume object
 */
StorHvVolume* StorHvVolumeTable::getVolume(fds_volid_t vol_uuid)
{
  StorHvVolume *ret_vol = NULL;

  map_rwlock.read_lock();
  if (volume_map.count(vol_uuid) > 0) {
    ret_vol = volume_map[vol_uuid];
  }
  else {
    FDS_PLOG_SEV(vt_log, fds::fds_log::warning) << "StorHvVolumeTable::getVolume - Volume "
						<< std::hex << vol_uuid << std::dec
						<< " does not exist";    
  }
  map_rwlock.read_unlock();

  return ret_vol;
}

/*
 * Returns the locked volume object. Guarantees that returned volume
 * object is valid (i.e., can safely access journal table and volume catalog cache)
 * Call StorHvVolume::readUnlock() when finished using volume object
 */
StorHvVolume* StorHvVolumeTable::getLockedVolume(fds_volid_t vol_uuid)
{
  StorHvVolume *ret_vol = getVolume(vol_uuid);

  if (ret_vol) {
    ret_vol->readLock();
    if (!ret_vol->isValidLocked())
    {
      ret_vol->readUnlock();
      FDS_PLOG_SEV(vt_log, fds::fds_log::error) << "StorHvVolumeTable::getLockedVolume - Volume " << vol_uuid
                       << "does not exist anymore, must have been destroyed";
      return NULL;
    }    
  }

  return ret_vol;
}

/* returns true if volume is registered and is in volume_map, otherwise returns false */
fds_bool_t StorHvVolumeTable::volumeExists(const std::string& vol_name)
{
  return (getVolumeUUID(vol_name) != invalid_vol_id);
}

StorHvVolVec
StorHvVolumeTable::getVolumeIds() {
    StorHvVolVec volIds;
    map_rwlock.read_lock();
    for (std::unordered_map<fds_volid_t, StorHvVolume*>::const_iterator it = volume_map.cbegin(); 
         it != volume_map.cend(); 
         ++it) {
        volIds.push_back(it->first);
    }
    map_rwlock.read_unlock();

    return volIds;
}

/* returns volume UUID if found in volume map, otherwise returns invalid_vol_id */
fds_volid_t StorHvVolumeTable::getVolumeUUID(const std::string& vol_name)
{
  fds_volid_t ret_id = invalid_vol_id;
  map_rwlock.read_lock();
  /* we need to iterate to check names of volumes (or we could keep vol name -> uuid 
   * map, but we would need to synchronize it with volume_map, etc, can revisit this later) */
  for (std::unordered_map<fds_volid_t, StorHvVolume*>::iterator it = volume_map.begin(); 
       it != volume_map.end(); 
       ++it)
    {
      if (vol_name.compare((it->second)->voldesc->name) == 0) {
	/* we found the volume, however if we found it second time, not good  */
	fds_verify(ret_id == invalid_vol_id);
	ret_id = it->first;
      }
    }
  map_rwlock.read_unlock();
  return ret_id;
}


/*
 * Add blob request to wait queue because a blob is waiting for OM
 * to attach bucjets to AM; once vol table receives vol attach event,
 * it will move all requests waiting in the queue for that bucket 
 * to appropriate qos queue 
 */
void StorHvVolumeTable::addBlobToWaitQueue(const std::string& bucket_name,
					   FdsBlobReq* blob_req)
{
  fds_verify(blob_req->magicInUse() == true);

  /* we can check again that this table does not know about this bucket */
  fds_verify(volumeExists(bucket_name) == false);

  FDS_PLOG_SEV(vt_log, fds::fds_log::debug) << "VolumeTable -- adding blob to wait queue, waiting for "
					    << "bucket " << bucket_name;

  /*
   * Pack to qos req
   * TODO: We're hard coding 885 as the request ID to
   * pass to QoS. SHCtrl has a request counter but
   * we can't see if from here...Let's fix that eventually.
   */
  AmQosReq *qosReq = new AmQosReq(blob_req, 885);

  wait_rwlock.write_lock();
  wait_blobs_it_t it = wait_blobs.find(bucket_name);
  if (it != wait_blobs.end()) {
    /* we already have vector of waiting blobs for this bucket name */
    (it->second).push_back(qosReq);
  }
  else {
    /* we need to start a new vector */
    wait_blobs[bucket_name].push_back(qosReq);
  }
  wait_rwlock.write_unlock();
}

void StorHvVolumeTable::moveWaitBlobsToQosQueue(fds_volid_t vol_uuid, 
						const std::string& vol_name, 
						Error error)
{
  Error err = error;
  bucket_wait_vec_t blobs;

  wait_rwlock.read_lock();
  wait_blobs_it_t it = wait_blobs.find(vol_name);
  if (it != wait_blobs.end()) {
    /* we have a wait queue of blobs for this volume name */
    blobs.swap(it->second);
    wait_blobs.erase(it);
  }
  wait_rwlock.read_unlock();

  if (blobs.size() == 0) {
    FDS_PLOG_SEV(vt_log, fds::fds_log::debug) << "VolumeTable::moveWaitBlobsToQueue -- "
					      << "no blobs waiting for bucket " << vol_name;
    return; // no blobs waiting  
  }

  if ( err.ok() ) 
    {
      fds::StorHvVolume *shvol = getLockedVolume(vol_uuid);
      if (( shvol == NULL) || (shvol->volQueue == NULL)) {
	FDS_PLOG_SEV(vt_log, fds::fds_log::error) <<  "VolumeTable::moveWaitBlobsToQosQueue -- " 
						  << "Volume and  Queueus are NOT setup :" << vol_uuid;
	err = ERR_INVALID_ARG;
      }
      if ( err.ok()) {
	for (uint i = 0; i < blobs.size(); ++i) {
	  FDS_PLOG_SEV(vt_log, fds::fds_log::debug)
                  << "VolumeTable - moving blob to qos queue of vol  " << std::hex
                  << vol_uuid << std::dec << " vol_name " << vol_name;
          // since we did not know the volume id before, volid for this io is set to 0
          // so set its volume id now to actual volume id
          AmQosReq* req = blobs[i];
          fds_verify(req != NULL);
          req->setVolId(vol_uuid);
	  storHvisor->qos_ctrl->enqueueIO(vol_uuid, req);
	}
	blobs.clear();
      }
      shvol->readUnlock();
    }

  if ( !err.ok()) {
    /* we haven't pushed requests to qos queue either because we already 
     * got 'error' parameter with !err.ok() or we couldn't find volume
     * so complete blobs in 'blobs' vector with error (if there are any) */
    for (uint i = 0; i < blobs.size(); ++i) {
      AmQosReq* qosReq = blobs[i];
      blobs[i] = NULL;
      FdsBlobReq* blobReq = qosReq->getBlobReqPtr();
      FDS_NativeAPI::DoCallback(blobReq, err, 0, 0);  // Hard coding a result!
      FDS_PLOG_SEV(vt_log, fds::fds_log::debug) 
	<< "VolumeTable -- completion of blob request before moving it to QoS queue";
      delete qosReq;
    } 
    blobs.clear();
  }
}

/*
 * Handler for volume-related control message from OM
 */
void StorHvVolumeTable::volumeEventHandler(fds_volid_t vol_uuid,
                                           VolumeDesc *vdb,
                                           fds_vol_notify_t vol_action,
					   FDS_ProtocolInterface::FDSP_ResultType result)
{
    Error err(ERR_OK);
    switch (vol_action) {
        case fds_notify_vol_attatch:
            FDS_PLOG_SEV(storHvisor->GetLog(), fds::fds_log::notification)
                    << "StorHvVolumeTable - Received volume attach event from OM"
                    << " for volume " << std::hex << vol_uuid << std::dec;
            
            if (result == FDS_ProtocolInterface::FDSP_ERR_OK) {
                err = storHvisor->vol_table->registerVolume(vdb ? *vdb : VolumeDesc("", vol_uuid));
            }
            else if (result == FDS_ProtocolInterface::FDSP_ERR_VOLUME_DOES_NOT_EXIST) {
                /* complete all requests that are waiting on bucket to attach with error */
                if (vdb) {
                    FDS_PLOG_SEV(storHvisor->GetLog(), fds::fds_log::notification)
                            << "StorHvVolumeTable - Volume " << vdb->name << "does not exist.";
                    storHvisor->vol_table->moveWaitBlobsToQosQueue(vol_uuid, vdb->name, Error(ERR_NOT_FOUND));
                }
            }
            break;
        case fds_notify_vol_detach:
            FDS_PLOG_SEV(storHvisor->GetLog(), fds::fds_log::notification)
                    << "StorHvVolumeTable - Received volume detach event from OM"
                    << " for volume " << std::hex << vol_uuid << std::dec;
            err = storHvisor->vol_table->removeVolume(vol_uuid);
            break;
        case fds_notify_vol_mod:
            fds_verify(vdb != NULL);
            FDS_PLOG_SEV(storHvisor->GetLog(), fds::fds_log::notification)
                    << "StorHvVolumeTable - Received volume modify  event from OM"
                    << " for volume " << vdb->name << ":" << std::hex
                    << vol_uuid << std::dec;
            err = storHvisor->vol_table->modifyVolumePolicy(vol_uuid, *vdb);
            break;
        default:
            FDS_PLOG_SEV(storHvisor->GetLog(), fds::fds_log::warning)
                    << "StorHvVolumeTable - Received unexpected volume event from OM"
                    << " for volume " << std::hex << vol_uuid << std::dec;
    } 

    // TODO(Anna) We have to respond to OM with error, but since we don't do it
    // yet, we assert here on error for now
    fds_verify(err.ok());
}

/* print detailed info into log */
void StorHvVolumeTable::dump()
{
  map_rwlock.read_lock();
  for (std::unordered_map<fds_volid_t, StorHvVolume*>::iterator it = volume_map.begin(); 
       it != volume_map.end(); 
       ++it)
    {
      FDS_PLOG(vt_log) << "StorHvVolumeTable - existing volume" 
                       << "UUID " << it->second->voldesc->volUUID
                       << ", name " << it->second->voldesc->name
                       << ", type " << it->second->voldesc->volType;
    }
  map_rwlock.read_unlock();
}

BEGIN_C_DECLS
int pushFbdReq(fbd_request_t *blkReq) {

	if (blkReq->io_type == FDS_IO_READ)
		blkReq->io_type = FDS_GET_BLOB;
	else if ( blkReq->io_type == FDS_IO_WRITE)
		blkReq->io_type = FDS_PUT_BLOB;
  /*
   * Map this blk request into a blob request
   */
  FdsBlobReq *blobReq = new FdsBlobReq((fds::fds_io_op_t)blkReq->io_type,  // IO type
                                       blkReq->volUUID,  // Vol ID
                                       std::to_string((fds_uint64_t)blkReq->sec *
                                                      HVISOR_SECTOR_SIZE),  // Temp blob name is offset
                                       // "blkDev:vol" + std::to_string(blkReq->volUUID),  // Temp blob name is volId
                                       blkReq->sec * HVISOR_SECTOR_SIZE,  // Blob offset
                                       blkReq->len,  // Request buffer length
                                       blkReq->buf,  // Data buffer
                                       blkReq->cb_request, // Callback function
                                       blkReq->vbd,  // Callback arg1
                                       blkReq->vReq, // Callback arg2
                                       blkReq);  // Callback arg 3 (request struct itself)
  Error err = storHvisor->pushBlobReq(blobReq);
  fds_verify(err == ERR_OK);
  return (0);
}
END_C_DECLS

BEGIN_C_DECLS
int  pushVolQueue(void *req1)
{
  fds_volid_t vol_id;
  StorHvVolume *shvol;
  fbd_request_t *req = (fbd_request_t *)req1;

#ifdef FDS_TEST_SH_NOOP
  FDS_PLOG(storHvisor->GetLog()) << "pushVolQueue: FDS_TEST_SH_NOOP defined, returning before pushing IO to SH queue";
  if ((fds::fds_io_op_t)req->io_type == FDS_IO_READ) {
    memset(req->buf, 0, req->len);
  }
  return 8;
#endif

  FDS_IOType *io = new FDS_IOType();

  vol_id = req->volUUID;
  shvol = storHvisor->vol_table->getLockedVolume(vol_id);
  if (( shvol == NULL) || (shvol->volQueue == NULL)) {
	  FDS_PLOG(storHvisor->GetLog()) <<  " Volume and  Queueus are NOT setup :" << vol_id;
	  return -1;
  }

  // push request to the  per volume queue 
  
  io->fbd_req = req;
  io->io_type = (fds::fds_io_op_t)req->io_type;
  io->io_vol_id = req->volUUID;
  io->io_module = FDS_IOType::STOR_HV_IO;
  io->io_req_id = atomic_fetch_add(&next_io_req_id, (unsigned int)1);
  io->io_magic = FDS_SH_IO_MAGIC_IN_USE;

  FDS_PLOG(storHvisor->GetLog()) << " Queueing IO " << io->io_req_id << " for  vol_id:  " << vol_id;

  storHvisor->qos_ctrl->enqueueIO(vol_id, io);
  shvol->readUnlock();

  FDS_PLOG(storHvisor->GetLog()) << " Queueing the  IO done.  vol_id:  " << vol_id;

 return 0;
}


END_C_DECLS


} // namespace fds
