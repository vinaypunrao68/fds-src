/*
 * Copyright 2013 Formation Data Systems, Inc.
 * Object tracker statistics 
 */
#include <set>
#include <string>
#include <fds_process.h>
#include <ObjStats.h>

namespace fds {

ObjStatsTracker gl_objStats;

ObjStatsTracker::ObjStatsTracker()
        : Module("SM Obj Stats Track") {
    /*
     * init the  log 
     */
    stats_log = NULL;

    objStatsMapLock = new fds_mutex("Added object Stats lock");
    fds_verify(objStatsMapLock != NULL);

    /*
     * get set the  start time 
     */
    startTime  = CounterHist8bit::getFirstSlotTimestamp();

    hotObjThreshold  = 100;
    coldObjThreshold = 20;
}

ObjStatsTracker::~ObjStatsTracker() {
    delete objStatsMapLock;
    delete  objStats;
}

int
ObjStatsTracker::mod_init(fds::SysParams const *const param) {
    Module::mod_init(param);
    stats_log = g_fdslog;
    fds_assert(stats_log != NULL);
    FDS_PLOG(stats_log) << "STATS:Start TIME: " << startTime;

    FdsConfigAccessor conf_helper(g_fdsprocess->get_conf_helper());
    const FdsRootDir *fdsroot = g_fdsprocess->proc_fdsroot();

    fdsroot->fds_mkdir(fdsroot->dir_fds_var_stats().c_str());
    root  = fdsroot->dir_fds_var_stats();
    root += conf_helper.get<std::string>("prefix");
    root += "objStats";

    // Create leveldb
    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, root, &db);
    fds_verify(status.ok() == true);
    return 0;
}

void
ObjStatsTracker::mod_startup() {
    Module::mod_startup();
}

void
ObjStatsTracker::mod_shutdown() {
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

Error ObjStatsTracker::updateIOpathStats(fds_volid_t vol_uuid, const ObjectID& objId) {
    Error err(ERR_OK);
    fds_bool_t  slotChange;
    ioPathStats   *oStats;

    FDS_PLOG(stats_log) << "STATS: inside update IOPathStats:"  << objId;
    /*
     * update the map stats 
     */
    objStatsMapLock->lock();

    if (objStatsExists(objId) == true) {
        FDS_PLOG(stats_log) << "STATS: obj stats  map EXISTS:"
                            << objId << "startTime: " << startTime;
        /* update  the stats  */
        oStats = ioPathStatsObj_map[objId];
        oStats->lastAccessTimeR =  oStats->objStats.last_access_ts;
        slotChange = oStats->objStats.increment(startTime, COUNTER_UPDATE_SLOT_TIME);
        if (slotChange == true) {
            oStats->averageObjectsRead =
                    oStats->objStats.getWeightedCount(startTime,
                                                      COUNTER_UPDATE_SLOT_TIME);
            FDS_PLOG(stats_log) << "STATS-DB: Average Objects  per slot :"
                                << oStats->averageObjectsRead;

            /* classify the  objects  for tiering */
            if (oStats->averageObjectsRead > hotObjThreshold) {
                /* check  if this object is in cold list  and delete  before adding to  Hot list */
                coldObjList.erase(objId);
                FDS_PLOG(stats_log) << "STATS-DB: Object classified  as HOT :" <<objId;
                hotObjList.insert(objId);
            }

            if (oStats->averageObjectsRead  < coldObjThreshold) {
                FDS_PLOG(stats_log) << "STATS-DB: Object classified  as COLD :" <<objId;
                coldObjList.insert(objId);
            }

            if ((oStats->averageObjectsRead  > coldObjThreshold) &&
                (oStats->averageObjectsRead < hotObjThreshold)) {
                FDS_PLOG(stats_log) << "STATS-DB: Object classification list CLEAN :" <<objId;
                coldObjList.erase(objId);
                hotObjList.erase(objId);
            }
        }
        ioPathStatsObj_map[objId] = oStats;
    } else {
        FDS_PLOG(stats_log) << "STATS: obj stats   Does not map Exists:"
                            << objId << "startTime: " << startTime;
        oStats = new ioPathStats();
        slotChange = oStats->objStats.increment(startTime, COUNTER_UPDATE_SLOT_TIME);
        oStats->lastAccessTimeR =  boost::posix_time::second_clock::universal_time();
        oStats->vol_uuid = vol_uuid;
        ioPathStatsObj_map[objId] = oStats;
    }
    objStatsMapLock->unlock();

    // if (slotChange == true) {
    //     /* update the stats to level DB */
    //     FDS_PLOG(stats_log) << "STATS-DB: Invoking the level DB  update function:"  << objId;
    //     err = updateDbStats(objId, oStats);
    // }

    return err;
}

void ObjStatsTracker::setHotObjectThreshold(fds_uint32_t hotObjLevel) {
    hotObjThreshold = hotObjLevel;
}

void ObjStatsTracker::setColdObjectThreshold(fds_uint32_t coldObjLevel) {
    coldObjThreshold = coldObjLevel;
}


void ObjStatsTracker::setAccessSlotTime(fds_uint64_t slotTime)  {
  /*
   * start the timer based on the  slottime requested by the invoker
   */
}

void ObjStatsTracker::lastObjectReadAccessTime(fds_volid_t vol_uuid,
                                               ObjectID& objId,
                                               fds_uint64_t accessTime) {
}

void ObjStatsTracker::lastObjectWriteAccessTime(fds_volid_t vol_uuid,
                                                ObjectID& objId,
                                                fds_uint64_t accessTime) {
}

fds_volid_t ObjStatsTracker::getVolId(const ObjectID& objId) {
    ioPathStats   *oStats;

    objStatsMapLock->lock();
    if (objStatsExists(objId) == true) {
        oStats = ioPathStatsObj_map[objId];
        objStatsMapLock->unlock();
        return (oStats->vol_uuid);
    }
    objStatsMapLock->unlock();
    return -1;
}

fds_uint32_t ObjStatsTracker::getObjectAccess(const ObjectID& objId) {
    ioPathStats   *oStats;
    fds_uint32_t  AveNumObjAccess = 0; /* if we don't have the stats, this is likely a cold obj */

    objStatsMapLock->lock();

    if (objStatsExists(objId) == true) {
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

void ObjStatsTracker::getHotObjectList(std::set<ObjectID, ObjectLess>& ret_list) {
    objStatsMapLock->lock();
    ret_list.swap(hotObjList); /* fast op, constant time */
    hotObjList.clear();
    objStatsMapLock->unlock();
}

/*
 * private  functions 
 */

Error ObjStatsTracker::queryDbStats(const ObjectID &oid, ioPathStats *qryStats) {
    return Error(ERR_OK);
}

Error ObjStatsTracker::updateDbStats(const ObjectID& oid, ioPathStats *updStats) {
    Error err(ERR_OK);

    /*
     *  search the level db for the existing record. if the record exisits  update and wtite it 
     *  back. if not create a  new record  write  it 
     */
    leveldb::WriteOptions writeopts;
    writeopts.sync = true;

    levelDbStats  *stats  = new levelDbStats();
    leveldb::Slice value(reinterpret_cast<char*>(stats), sizeof(levelDbStats));
    leveldb::Slice key(reinterpret_cast<const char *>(oid.GetId()), oid.getDigestLength());
    std::string valueR = "";

    stats->lastAccessTimeR = updStats->lastAccessTimeR;

    leveldb::Status status = db->Get(leveldb::ReadOptions(), key, &valueR);
    valueR.copy(reinterpret_cast<char *>(stats), sizeof(levelDbStats), 0);
    if (!status.ok()) {
        stats->numObjsRead += updStats->objStats.getWeightedCount(startTime,
                                                                  COUNTER_UPDATE_SLOT_TIME);

        leveldb::Status status = db->Put(writeopts, key, value);
        if (!status.ok()) {
            FDS_PLOG(stats_log) << "STATS-LVL:Failed to  put   the object stats "
                                << status.ToString();
        } else {
            FDS_PLOG(stats_log) << "STATS-LVL:Successfully added the per "
                                << "objects stats to level DB: "
                                << oid;
        }
    } else {
        FDS_PLOG(stats_log) << "STATS-LVL:Objects  exists in levelDB:"
                            << oid << "numObjs:" << stats->numObjsRead;
        /*
         * we already have the key and value in level DB, update the stats 
         */
        stats->numObjsRead += updStats->objStats.getWeightedCount(startTime,
                                                                  COUNTER_UPDATE_SLOT_TIME);

        leveldb::Status status = db->Put(writeopts, key, value);
        if (!status.ok()) {
            FDS_PLOG(stats_log) << "STATS-LVL:Failed to  update  the object stats  "
                                << status.ToString();
        } else {
            FDS_PLOG(stats_log) << "STATS-LVL:Successfully updated the per "
                                << "objects stats to level DB "
                                << oid;
        }
    }
    return err;
}

Error ObjStatsTracker::deleteDbStats(const ObjectID &oid, ioPathStats *delStats) {
    return Error(ERR_OK);
}

}  // namespace fds
