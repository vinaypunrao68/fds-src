
/*
 * Copyright 2013 Formation Data Systems, Inc.
 * Object tracker statistics 
 */

#include <include/ObjStats.h>
#include <StorMgr.h>

namespace fds {

extern ObjectStorMgr  *objStorMgr;

ObjStatsTracker::ObjStatsTracker(fds_log *parent_log) {

   std::string  filename = "/fds/objStats";

    /*
     * init the  log 
     */
    if (parent_log)
	stats_log = parent_log;

    objStatsMapLock = new fds_mutex("Added object Stats lock");

    /*
     * get set the  start time 
     */
  startTime  = CounterHist8bit::getFirstSlotTimestamp();
  FDS_PLOG(stats_log) << "STATS:Start TIME: " << startTime;


  // Create leveldb
  leveldb::Options options;
  options.create_if_missing = true;
  leveldb::Status status = leveldb::DB::Open(options, filename, &db);
//  assert(status.ok());
  FDS_PLOG(stats_log) << "STATS:LevelDB status is " << status.ToString();
}

ObjStatsTracker::~ObjStatsTracker() {
    delete objStatsMapLock;
    delete  objStats;

}

fds_bool_t ObjStatsTracker::objStatsExists(const ObjectID& objId) {
  if (ioPathStatsObj_map.count(objId) != 0) {
    return true;
  }
  return false;
}

fds_bool_t ObjStatsTracker::objExists(const ObjectID& objId) {
  fds_bool_t result;

  objStatsMapLock->lock();
  result = objStatsExists(objId);
  objStatsMapLock->unlock();

  return result;
}

Error ObjStatsTracker::updateIOpathStats(fds_volid_t vol_uuid,const ObjectID& objId) {

   Error err(ERR_OK);
   fds_bool_t  slotChange;
   ioPathStats   *oStats;
//   StorMgrVolumeTable *vol_tab = objStorMgr->sm_getVolTables();
 
  FDS_PLOG(stats_log) << "STATS: inside update IOPathStats:"  << objId;
   /*
    * update the map stats 
    */
   objStatsMapLock->lock();

   if(objStatsExists(objId) == true) {
  FDS_PLOG(stats_log) << "STATS: obj stats  map EXISTS:"  << objId << "startTime: "<< startTime;
    /* update  the stats  */
    oStats = ioPathStatsObj_map[objId];
    oStats->lastAccessTimeR =  oStats->objStats.last_access_ts;
    slotChange = oStats->objStats.increment(startTime, COUNTER_UPDATE_SLOT_TIME);
    if (slotChange == true) {
	oStats->averageObjectsRead += oStats->objStats.getWeightedCount(startTime,COUNTER_UPDATE_SLOT_TIME);
    	FDS_PLOG(stats_log) << "STATS-DB: Average Objects  per slot :"  << oStats->averageObjectsRead;
    }
    ioPathStatsObj_map[objId] = oStats;
  }else {

  FDS_PLOG(stats_log) << "STATS: obj stats   Does not map Exists:"  << objId << "startTime: " << startTime;
   oStats = new ioPathStats();
   slotChange = oStats->objStats.increment(startTime, COUNTER_UPDATE_SLOT_TIME);
   oStats->lastAccessTimeR =  boost::posix_time::second_clock::universal_time();
   ioPathStatsObj_map[objId] = oStats;
  }

  objStatsMapLock->unlock(); 

   /* update per volume stats */
//    volTbl->updateVolStats(vol_uuid);

   if (slotChange == true) {
 	/* update the stats to level DB */
     FDS_PLOG(stats_log) << "STATS-DB: Invoking the level DB  update function:"  << objId;
     err = updateDbStats(objId,oStats); 
   }

  return err;
}


void ObjStatsTracker::setAccessSlotTime(fds_uint64_t slotTime)  {
  /*
   * start the timer based on the  slottime requested by the invoker
   */


}



void ObjStatsTracker::lastObjectReadAccessTime(fds_volid_t vol_uuid,ObjectID& objId, fds_uint64_t accessTime) {

}



void ObjStatsTracker::lastObjectWriteAccessTime(fds_volid_t vol_uuid,ObjectID& objId, fds_uint64_t accessTime) {



}



fds_uint32_t ObjStatsTracker::getObjectAccess( const ObjectID& objId) {

   ioPathStats   *oStats;
   fds_uint32_t  AveNumObjAccess = -1;

   objStatsMapLock->lock();

   if(objStatsExists(objId) == true) {
    /* update  the stats  */
    oStats = ioPathStatsObj_map[objId];
    AveNumObjAccess = oStats->objStats.getWeightedCount(startTime, COUNTER_UPDATE_SLOT_TIME);
    objStatsMapLock->unlock();
    FDS_PLOG(stats_log) << "STATS: Objects  recived  on SLOT :"  << AveNumObjAccess;
    return AveNumObjAccess;
   }

  FDS_PLOG(stats_log) << "STATS:Objects  does not  exists in the map:"  << objId;
  objStatsMapLock->unlock();

  return AveNumObjAccess;
}


/*
 * private  functions 
 */

Error ObjStatsTracker::queryDbStats(const ObjectID &oid,ioPathStats *qryStats)  {



}


Error ObjStatsTracker::updateDbStats(const ObjectID& oid,ioPathStats *updStats)  {
   Error err(ERR_OK);

 /*
  *  search the level db for the existing record. if the record exisits  update and wtite it 
  *  back. if not create a  new record  write  it 
  */

  leveldb::WriteOptions writeopts;
  writeopts.sync = true;

  levelDbStats  *stats  = new levelDbStats();
  leveldb::Slice value((char*)stats, sizeof(levelDbStats));
  leveldb::Slice key((const char *)&oid, sizeof(oid));
  std::string valueR = "";

 stats->lastAccessTimeR = updStats->lastAccessTimeR ; 

  leveldb::Status status = db->Get(leveldb::ReadOptions(), key, &valueR);
  valueR.copy((char *)stats, sizeof(levelDbStats), 0);
  if (!status.ok()) {

      stats->numObjsRead += updStats->objStats.getWeightedCount(startTime,COUNTER_UPDATE_SLOT_TIME);

      leveldb::Status status = db->Put(writeopts, key, value);

     if (! status.ok()) {
       FDS_PLOG(stats_log) << "STATS-LVL:Failed to  put   the object stats "
	       << status.ToString();
     } else {
       FDS_PLOG(stats_log) << "STATS-LVL:Successfully added the per objects stats  to level DB: "
	       << oid;
     }

   } else {
      FDS_PLOG(stats_log) << "STATS-LVL:Objects  exists in levelDB:" << oid << "numObjs:" << stats->numObjsRead ;
      /*
       * we already have the key and value in level DB, update the stats 
       */

    stats->numObjsRead += updStats->objStats.getWeightedCount(startTime,COUNTER_UPDATE_SLOT_TIME);

    leveldb::Status status = db->Put(writeopts, key, value);

     if (! status.ok()) {
       FDS_PLOG(stats_log) << "STATS-LVL:Failed to  update  the object stats  "
	       << status.ToString();
     } else {
       FDS_PLOG(stats_log) << "STATS-LVL:Successfully updated the per objects stats  to level DB "
	       << oid;

     }
   }
  return err;
}



Error ObjStatsTracker::deleteDbStats(const ObjectID &oid,ioPathStats *delStats)  {




}

} /* fds namespace */
