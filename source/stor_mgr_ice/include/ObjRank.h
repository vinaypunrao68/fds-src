/*
 *  * Copyright 2013 Formation Data Systems, Inc.
 *
 */
#ifndef OBJ_RANK_ENGINE_H_
#define OBJ_RANK_ENGINE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fdsp/FDSP.h>
#include "stor_mgr_err.h"
#include <fds_volume.h>
#include <fds_types.h>
#include "ObjLoc.h"
#include <odb.h>
#include <unistd.h>
#include <assert.h>
#include <iostream>
#include <Ice/Ice.h>
#include <util/Log.h>
#include "DiskMgr.h"
#include "StorMgrVolumes.h"
#include <disk-mgr/dm_service.h>
#include <disk-mgr/dm_io.h>

#include <include/fds_qos.h>
#include <include/qos_ctrl.h>
#include <include/fds_assert.h>
#include <utility>
#include <atomic>
#include <unordered_map>


// ObjectRankEngine : A class that keeps track of a rank table of all objects that "Needs" to be in the SSD tier. 
// It is the job of the migrator/placement-tiering engine to make sure the rank table is reflected in the persistent-Layer
class ObjectRankEngine {
 public: 
   ObjectRankEngine(fds_uint32_t tbl_size, fds_log *log);
   ~ObjectRankEngine();
   typedef enum rankOperType {
     OBJ_RANK_PROMOTION, // To Flash/SSD
     OBJ_RANK_DEMOTION // To disk
   };
   
   fds_log *ranklog;
   ObjectDB *rankDB;
   fds_bool_t   rankingStatus; // True if ranking process is enabled
   
   typedef std::unordered_map<ObjectID, rankOperType> RankDeltaTbl;

   RankDeltaTbl *rankDeltaChgTbl; //Delta Change table from the last migrator snapshot

   // The main Rank Table
   std::ordered_map<ObjectID, fds_uint32_t> rankTbl;   

  // rankAndInsertObject() is called by placement engine on a new objectID
   fds_uint32_t rankAndInsertObject(ObjectID objid, StorMgrVolume *volume);

  // Delete an objectID from the rank list since it is no longer valid
   void deleteObject(ObjectID objid);

   // Obtain the rank assigned to an object
   fds_uint32_t getRank(ObjectID *objid);

   // Start the ranking process
   void  startObjRanking() {
    rankingStatus = true;
   }

   // Stop ranking process - called for by administrator controlled config, 
   // Admins can provide a scheduled time when ranking proces can run, without interfering with the ingest traffic
   void stopObjRanking() {
      rankingStatus = false;
   }

   // Migrator/Tiering Engine will periodically get the changes to the rankTbl and apply the migrations as needed 
   // consulting the indexTbl
   void getRankDeltaChgTbl(RankDeltaTbl &tbl);

   // Persist the Rank Tbl via level-DB for restartability, & restart rank-engine without loosing the prev context
   void  persistRankTbl();
   
   // getNextTblSegment returns the num_objs in the array provided by caller, return false if there are no more to return
   fds_bool_t  getNextTblSegment(fds_uint32_t num_objs, ObjectID *objIdArray);

};


ObjectRankEngine::ObjectRankEngine(fds_uint32_t tbl_size, fds_log *log, ObjectStorMgr *sm) : ranklog(log) { 
    std::string filename;
    filename= sm->getStorPrefix() + "ObjRankDB";
    rankDB = new ObjectDB(filename);
}

ObjectRankEngine:~ObjectRankEngine() {
   delete rankDB;
}

#endif
