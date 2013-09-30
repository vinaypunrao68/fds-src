/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "StorHvisorNet.h"
#include "StorHvJournal.h"
#include "StorHvVolumes.h"

extern StorHvCtrl *storHvisor;

namespace fds {

StorHvVolume::StorHvVolume(const VolumeDesc& vdesc, StorHvCtrl *sh_ctrl, fds_log *parent_log)
  : FDS_Volume(vdesc), parent_sh(sh_ctrl)
{
  journal_tbl = new StorHvJournal(FDS_READ_WRITE_LOG_ENTRIES);
  vol_catalog_cache = new VolumeCatalogCache(volUUID, sh_ctrl, parent_log);  
  stat_history = new StatHistory(volUUID);

  if ( volType == FDS_VOL_BLKDEV_TYPE && parent_sh->GetRunTimeMode() == StorHvCtrl::NORMAL) { 
      blkdev_minor = (*parent_sh->cr_blkdev)(volUUID);
  }

  is_valid = true;
}

StorHvVolume::~StorHvVolume()
{
  if (journal_tbl)
    delete journal_tbl;
  if (vol_catalog_cache)
    delete vol_catalog_cache;
  if (stat_history)
    delete stat_history;
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

  if ( volType == FDS_VOL_BLKDEV_TYPE && parent_sh->GetRunTimeMode() == StorHvCtrl::NORMAL) { 
     (*parent_sh->del_blkdev)(blkdev_minor);
  }
  /* destroy data */
  delete journal_tbl;
  journal_tbl = NULL;
  delete vol_catalog_cache;
  vol_catalog_cache = NULL;
  delete stat_history;
  stat_history = NULL;

  is_valid = false;
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

/* creates its own logger */
StorHvVolumeTable::StorHvVolumeTable(StorHvCtrl *sh_ctrl, fds_log *parent_log)
  : parent_sh(sh_ctrl), 
    statTimer(new IceUtil::Timer),
    statTimerTask(new StorHvStatTimerTask(this))
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

  if (sh_ctrl->GetRunTimeMode() == StorHvCtrl::TEST_BOTH) { 
    VolumeDesc vdesc("default_vol", FDS_DEFAULT_VOL_UUID);
    volume_map[FDS_DEFAULT_VOL_UUID] = new StorHvVolume(vdesc, parent_sh, vt_log);
    FDS_PLOG(vt_log) << "StorHvVolumeTable - constructor registered volume 1";  
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

  statTimer->destroy();
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
    StorHvVolume* new_vol = new StorHvVolume(vdesc, parent_sh, vt_log);
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
 * Removes volume from the map, returns error if volume does not exist
 */
Error StorHvVolumeTable::removeVolume(fds_volid_t vol_uuid)
{
  Error err(ERR_OK);
  StorHvVolume *vol = NULL;

  map_rwlock.write_lock();
  if (volume_map.count(vol_uuid) == 0) {
    FDS_PLOG(vt_log) << "StorHvVolumeTable - removeVolume called for non-existing volume "
                     << vol_uuid;
    err = ERR_INVALID_ARG;
    map_rwlock.write_unlock();
    return err;
  }

  vol = volume_map[vol_uuid];
  volume_map.erase(vol_uuid);
  map_rwlock.write_unlock();

  /* call destroy first to ensure that no readers accessing volume
   * when destroying its data */
  vol->destroy();
  delete vol;
  
  FDS_PLOG(vt_log) << "StorHvVolumeTable - Removed volume "
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
    FDS_PLOG(vt_log) << "StorHvVolumeTable::getVolume - Volume " << vol_uuid
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
      FDS_PLOG(vt_log) << "StorHvVolumeTable::getLockedVolume - Volume " << vol_uuid
                       << "does not exist anymore, must have beed destroyed";
      return NULL;
    }    
  }

  return ret_vol;
}

/*
 * Handler for volume-related control message from OM
 */
void StorHvVolumeTable::volumeEventHandler(fds_volid_t vol_uuid,
                                           VolumeDesc *vdb,
                                           fds_vol_notify_t vol_action)
{
  switch (vol_action) {
  case fds_notify_vol_attatch:
    FDS_PLOG(storHvisor->GetLog()) << "StorHvVolumeTable - Received volume attach event from OM"
                                   << " for volume " << vol_uuid;

    storHvisor->vol_table->registerVolume(vdb ? *vdb : VolumeDesc("", vol_uuid));
    break;
  case fds_notify_vol_detach:
    FDS_PLOG(storHvisor->GetLog()) << "StorHvVolumeTable - Received volume detach event from OM"
                                   << " for volume " << vol_uuid;
    storHvisor->vol_table->removeVolume(vol_uuid);
    break;
  default:
    FDS_PLOG(storHvisor->GetLog()) << "StorHvVolumeTable - Received unexpected volume event from OM"
                                   << " for volume " << vol_uuid;
  } 
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
                       << "UUID " << it->second->volUUID
                       << ", name " << it->second->vol_name
                       << ", type " << it->second->volType;
    }
  map_rwlock.read_unlock();
}


/* Start dumping performance stats to log file*/
void StorHvVolumeTable::startPerfStats()
{
  if (statTimer)
    {
      IceUtil::Time interval = IceUtil::Time::seconds(FDS_STAT_DEFAULT_SLOT_LENGTH * (FDS_STAT_DEFAULT_HIST_SLOTS-2));
      try {
	statTimer->scheduleRepeated(statTimerTask, interval);
	FDS_PLOG(vt_log) << "StorHvVolumeTable: started logging perf stats";
      } catch (const IceUtil::IllegalArgumentException&) {
	/* ok */
	FDS_PLOG(vt_log) << "StorHvVolumeTable::startPerfStats: already dumping stats";
      } catch (...) {
	FDS_PLOG(vt_log) << "StorHvVolumeTable::startPerfStats: failed to start dumpting stats";
      }
    }
}

/* Stop dumping performace stats to log file */
void StorHvVolumeTable::stopPerfStats()
{
  if (statTimer)
    {
      statTimer->cancel(statTimerTask);
      FDS_PLOG(vt_log) << "StorHvVolumeTable: stopped logging perf stats";
    }
}

void StorHvVolumeTable::printPerfStats(std::ofstream& dumpFile)
{
  boost::posix_time::ptime tnow = boost::posix_time::microsec_clock::local_time();
  FDS_PLOG(vt_log) << "StorHvVolumeTable: printing perf stats";

  map_rwlock.read_lock();
  for (std::unordered_map<fds_volid_t, StorHvVolume*>::iterator it = volume_map.begin();
       it != volume_map.end();
       ++it)
    {
      it->second->stat_history->print(dumpFile, tnow);
    }
  map_rwlock.read_unlock();
}

StorHvStatTimerTask::StorHvStatTimerTask(StorHvVolumeTable* voltab) 
  :volTable(voltab)
{
  boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();

  std::string dirname("stats");
  std::string nowstr = to_simple_string(now);
  std::size_t i = nowstr.find_first_of(" ");
  std::size_t k = nowstr.find_first_of(".");
  std::string ts_str("");
  if (i != std::string::npos) {
    ts_str = nowstr.substr(0, i);
    if (k == std::string::npos) {
      ts_str.append("-");
      ts_str.append(nowstr.substr(i+1));
    }	     	     
  }

  /*make sure stats directory exists */
  if ( !boost::filesystem::exists(dirname.c_str()) )
    {
      boost::filesystem::create_directory(dirname.c_str());
    }

  /* use timestamp in the filename */
  std::string fname =dirname + std::string("//perf_") + ts_str + std::string(".stat"); 
  statfile.open(fname.c_str(), ios::out | ios::app );
}

/* timer task to print performance stats */
void StorHvStatTimerTask::runTimerTask()
{
  volTable->printPerfStats(statfile);
}

} // namespace fds
