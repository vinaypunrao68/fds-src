/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include "StorHvisorNet.h"
#include "StorHvJournal.h"

#include "StorHvVolumes.h"

namespace fds {


/* default values 
 * TODO: currently just some random numbers, assuming we are using vol id and 
 * nothing else; need some meaningful defaults */
FDS_Volume::FDS_Volume()
{
  tennantId = 1;
  localDomainId = 1;
  globDomainId = 1;
  volUUID = FDS_DEFAULT_VOL_UUID;
  volType = FDS_VOL_BLKDEV_TYPE;

  capacity = 0;
  maxQuota = 0;

  replicaCnt = 1;
  writeQuorum = 1;
  readQuorum = 1;
  consisProtocol = FDS_CONS_PROTO_STRONG;
  
  volPolicyId = 1;
  archivePolicyId = 1;
  placementPolicy = 1;
  appWorkload = FDS_APP_WKLD_JOURNAL_FILESYS;
  
  backupVolume = 1;
}

FDS_Volume::~FDS_Volume()
{
}

StorHvVolume::StorHvVolume(fds_volid_t vol_uuid, StorHvCtrl *sh_ctrl, fds_log *parent_log)
  : FDS_Volume(), parent_sh(sh_ctrl)
{
  volUUID = vol_uuid;
  /* all other values are default for now */

  journal_tbl = new StorHvJournal(FDS_READ_WRITE_LOG_ENTRIES);
  vol_catalog_cache = new VolumeCatalogCache(vol_uuid, sh_ctrl, parent_log);  
}

StorHvVolume::~StorHvVolume()
{
  delete journal_tbl;
  delete vol_catalog_cache;
}


/***** StorHvVolumeTable methods ******/

/* creates its own logger */
StorHvVolumeTable::StorHvVolumeTable(StorHvCtrl *sh_ctrl)
  : parent_sh(sh_ctrl)
{
  vt_log = new fds_log("shvt", "logs");
  created_log = true;

  /* 
   * Hardcoding creation of volume 1
   * TODO: do not hardcode creation of volume 1, should
   * explicitly call register volume for all volumes 
   */
  volume_map[FDS_DEFAULT_VOL_UUID] = new StorHvVolume(FDS_DEFAULT_VOL_UUID, parent_sh, vt_log);
  FDS_PLOG(vt_log) << "StorHvVolumeTable - constructor registered volume 1";  
}

StorHvVolumeTable::StorHvVolumeTable(StorHvCtrl *sh_ctrl, fds_log *parent_log)
  : parent_sh(sh_ctrl),
    vt_log(parent_log),
    created_log(false)
{
  /* 
   * Hardcoding creation of volume 1
   * TODO: do not hardcode creation of volume 1, should
   * explicitly call register volume for all volumes 
   */
  volume_map[FDS_DEFAULT_VOL_UUID] = new StorHvVolume(FDS_DEFAULT_VOL_UUID, parent_sh, vt_log);
  FDS_PLOG(vt_log) << "StorHvVolumeTable - constructor registered volume 1";  
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
Error StorHvVolumeTable::registerVolume(fds_volid_t vol_uuid)
{
  Error err(ERR_OK);

  map_rwlock.write_lock();
  if (volume_map.count(vol_uuid) < 1) {
    StorHvVolume* new_vol = new StorHvVolume(vol_uuid, parent_sh, vt_log);
    if (new_vol) {
      volume_map[vol_uuid] = new_vol;
    }
    else {
      err = ERR_INVALID_ARG; // TODO: need more error types
    }
  }
  map_rwlock.write_unlock();

  FDS_PLOG(vt_log) << "StorHvVolumeTable - Register new volume " << vol_uuid
                   << " result: " << err.GetErrstr();  

  return err;
}

/*
 * Returns SH volume object or NULL if the volume has not been created
 * TODO: another thread may delete the volume since we release the read lock,
 * so we should revisit this when we implement volume delete
 */
StorHvVolume* StorHvVolumeTable::getVolume(fds_volid_t vol_uuid)
{
  StorHvVolume *ret_vol = NULL;

  map_rwlock.read_lock();
  if (volume_map.count(vol_uuid) > 0) {
    ret_vol = volume_map[vol_uuid];
  }
  else {
    FDS_PLOG(vt_log) << "StorHvVolumeTable::getVolume - Volume " << vol_uuid
                     << " does not exist";    
  }
  map_rwlock.read_unlock();

  return ret_vol;
}

} // namespace fds
