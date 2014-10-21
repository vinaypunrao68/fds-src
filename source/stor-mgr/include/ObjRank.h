/*
 *  Copyright 2013-2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_OBJRANK_H_
#define SOURCE_STOR_MGR_INCLUDE_OBJRANK_H_

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <atomic>
#include <string>
#include <map>
#include <unordered_map>
#include <utility>

#include "fds_volume.h"
#include "fds_timer.h"
#include "lib/Catalog.h"
#include "StorMgrVolumes.h"

#include "include/fds_assert.h"
#include "concurrency/Mutex.h"
#include "concurrency/Thread.h"
#include "concurrency/Synchronization.h"

#include "ObjStats.h"


#define DEFAULT_RANK_FREQUENCY 1800           /* frequency in seconds */
#define DEFAULT_RANK_TBL_SIZE  100000         /* size of rank table */
#define DEFAULT_HOT_THRESHOLD  3              /* access obj about 3 times to make it hot */
#define DEFAULT_COLD_THRESHOLD 0              /* access obj about 0 times to make it cold */

#define MAX_RANK_CACHE_SIZE    10485760      /* x 20bytes = 200MB */
#define LOWRANK_OBJ_CACHE_SIZE 100000        /* x 20bytes ~ 2MB  */
#define OBJECT_RANK_ALL_SSD    0x00000000    /* highest rank */
#define OBJECT_RANK_ALL_DISK   0xFFFFFF00    /* lowest rank that we can calculate */
#define OBJECT_RANK_LOWEST     0xFFFFFFE0    /* lowest rank sentinel */
#define RANK_PROMOTION_MASK    0x00000004    /* mask for promotion operation */
#define RANK_DEMOTION_MASK     0x00000002    /* check if rank specifies demotion operation */
#define RANK_INVALID_MASK      0x00000001    /* check if rank is invalid (used to mark deletion) */

namespace fds {

/* ObjectRankEngine : A class that keeps track of a rank table of all objects that 
 * "Needs" to be in the SSD tier. It is the job of the migrator/placement-tiering engine 
 * to make sure the rank table is reflected in the persistent-Layer
 *
 */ 
class ObjectRankEngine {
    public:
     ObjectRankEngine(const std::string& _sm_prefix,
                      fds_uint32_t tbl_size,
                      fds_uint32_t rank_freq_sec,
                      fds_uint32_t _hot_threshold,
                      fds_uint32_t _cold_threshold,
                      StorMgrVolumeTable *_sm_volTbl,
                      ObjStatsTracker *_obj_stats);
     ~ObjectRankEngine();

     /* 'initialization' state means we accept insert and delete ops (on data path)
      * but do not do any ranking/creating chg table /etc.
      * */
     typedef enum {
         RANK_ENG_INITIALIZING,
         RANK_ENG_ACTIVE,
         RANK_ENG_EXITING
     } rankEngineState;

     typedef enum {
         OBJ_RANK_PROMOTION,    // To Flash/SSD
         OBJ_RANK_DEMOTION      // To disk
     } rankOperType;

     typedef std::unordered_map<ObjectID, rankOperType, ObjectHash> rank_delta_table_t;
     typedef std::unordered_map<ObjectID, fds_uint32_t, ObjectHash> obj_rank_cache_t;
     typedef std::unordered_map<ObjectID, fds_uint32_t, ObjectHash>::iterator obj_rank_cache_it_t;
     typedef std::multimap<fds_uint32_t, ObjectID> rank_order_objects_t;
     typedef std::multimap<fds_uint32_t, ObjectID>::iterator rank_order_objects_it_t;
     typedef std::multimap<fds_uint32_t, ObjectID>::reverse_iterator rank_order_objects_rit_t;

     // rankAndInsertObject() is called by placement engine on a new objectID
     fds_uint32_t rankAndInsertObject(const ObjectID& objId, const VolumeDesc& voldesc);

     // Delete an objectID from the rank list since it is no longer valid
     void deleteObject(const ObjectID& objId);

     // Obtain the rank assigned to an object
     fds_uint32_t getRank(const ObjectID& objId, const VolumeDesc& voldesc);

     /* check if ranking process is in progress */
     fds_bool_t rankingInProgress();

     // Start the ranking process
     void startObjRanking();

     // Stop ranking process - called for by administrator controlled config,
     // Admins can provide a scheduled time when ranking proces can run, without
     // interfering with the ingest traffic
     void stopObjRanking();

     // Migrator/Tiering Engine will periodically get the changes to the rankTbl
     // and apply the migrations as needed consulting the indexTbl
     // Currently: will return 0 if ranking process is in progress
     // The array must be allocated by the caller
     // returns number of actualy objects we put into 'objRankArray'
     // Once objects are returned, the rank table 'forgets' about them; as in it
     // does not keep track of those objects anymore.
     typedef std::pair<ObjectID, rankOperType> chg_table_entry_t;
     fds_uint32_t getDeltaChangeTblSegment(fds_uint32_t num_objs, chg_table_entry_t* objRankArray);

     // Migrator/Tiering Engine must call this method when done with RankDeltaChangeTable
     // For now we 'forget' about objects once we return them with getDeltaChangeTblSegment
     // void doneWithRankDeltaChangeTable(void);

     // getNextTblSegment returns the num_objs in the array provided by caller
     // returns actual number of objects that it put to the array
     // if return value < num_objects, means no more objects to return
     // Currently, it will stop providing objects if they are not cached;
     // also, assumes they get used, so will remove those from cache as soon as they are returned.
     fds_uint32_t getNextTblSegment(fds_uint32_t num_objs, ObjectID *objIdArray);

     // Returns the rank of the last object in the rank table (the lowest-rank)
     // If the rank table has free space, returns OBJECT_RANK_INVALID which is the
     // lowest possible rank;
     // This is the lowest rank as of the last ranking process
     inline fds_uint32_t getTblTailRank() const { return tail_rank; }

     static void runRankingThread(ObjectRankEngine* self);
     void analyzeStats(void); /* called by timer */

    private:
     /* completes initialization of the ranking engine
      * This is first thing that ranking background thread does,
      * so it's done in the background because it walks through rankdb */
     Error initialize();

     /* Object rank (a 32-bit value) is constructed as follows: 
      *  bits 32-17 : object subrank that captures recency and frequency of object accesses
      *  bits 16-1  : volume rank that captures volume policy, consists of 
      *               bits 16-9: policies related to performance 
      *               bits 8-4 : not used yet
      *               bit 3 : promotion (1)
      *               bit 2 : demotion (1)
      *               bit 1 : valid (0) /invalid (1)
      *  Special values:
      *  0x00000000  'all ssd' volume
      *  0xFFFFFF00  'all disk' volume
      *  0xFFFFFFFF   invalid rank id, used internally for deleted objects
      */

     // for now a simple rank based on volume's priority and whether volume
     // has min_iops requirement
     inline fds_uint16_t getVolumeRank(const VolumeDesc& voldesc) const {
         fds_uint8_t bin = (voldesc.iops_min > 0) ? 0 : 1;
         fds_uint16_t rank = (10 * bin + voldesc.relativePrio);
         return (( rank << 8 ) & 0xFF00);
     }

     inline fds_uint16_t getObjectSubrank(const ObjectID& objId) {
         /* we don't need to go exactly with frequency, e.g. if object 
          * was accessed 30 or 31 times recently it should yield the same 
          * rank, so we will reset few least significant bits */ 
         const fds_uint16_t frequency_mask = 0xFFFC; /* last two bits */
         fds_uint16_t frequency = obj_stats->getObjectAccess(objId);

         /* mask and invert so that the higher ranks have lower values */
         return ( ~(frequency & frequency_mask) );
     }

     inline fds_uint32_t updateRank(const ObjectID& objId, fds_uint32_t rank) {
         fds_uint32_t new_rank = rank & 0x0000FFFF; /* remove object subrank part */
         if ((isRankInvalid(rank)) ||
             (rank == OBJECT_RANK_ALL_SSD) ||
             (rank == OBJECT_RANK_ALL_DISK)) {
             return rank;
         }
         new_rank += ((getObjectSubrank(objId) << 16) & 0xFFFF0000);
         return new_rank;
     }

     inline fds_bool_t isRankDemotion(fds_uint32_t rank) const {
         return (( rank & RANK_DEMOTION_MASK ) != 0x00000000);
     }

     inline fds_bool_t isRankPromotion(fds_uint32_t rank) const {
         return (( rank & RANK_PROMOTION_MASK ) != 0x00000000);
     }

     inline fds_bool_t isRankInvalid(fds_uint32_t rank) const {
         return ( ( rank & RANK_INVALID_MASK ) != 0x00000000);
     }


     /* gets lowest rank from lowrank list,  */
     fds_uint32_t getLowestRank(void);
     fds_bool_t inRankDB(const ObjectID& oid);
     fds_bool_t inInsertDeleteCache(const ObjectID& oid);
     Error deleteFromRankDB(const ObjectID& oid);
     Error putToRankDB(const ObjectID& oid,
                       fds_uint32_t o_rank,
                       fds_bool_t b_addition,
                       fds_bool_t b_tmp_chgtbl);
     Error applyCachedOps(obj_rank_cache_t* cached_objects);

     /* methods that actually implement ranking process */
     void runRankingThreadInternal(void);
     Error doRanking(void);

     /* persist change table stored in tmp_chgTbl  and swap to rankDeltaChgTbl*/
     Error persistChgTableAndSwap();

    private:
     rankEngineState rankeng_state;

     /* persistent rank table */
     Catalog* rankDB;

     /* size of the rank table */
     fds_uint32_t rank_tbl_size;
     /* current number of objects in the rank table, normally will be same as table size */
     /* currently, we only access this in ranking thread or on timer when analyzing stats from 
      * stats tracker -- the latter runs only when ranking process is not running, so we don't lock
      * access to cur_rank_tbl_size */
     fds_uint32_t cur_rank_tbl_size;

     /**
      * Time interval in seconds when we do re-ranking of objects.
      */
     fds_uint32_t rank_period_sec;
     fds_uint32_t hot_threshold;
     fds_uint32_t cold_threshold;

     /* this is a cache of newly inserted/deleted objects (since last ranking process) */
     obj_rank_cache_t cached_obj_map;
     /* max number of objects that we can keep in the cached object map, if we reach
      * this max, we will start ranking process */
     fds_uint32_t max_cached_objects;
     fds_mutex* map_mutex;

     /* Delta Change table from the last migrator snapshot that holds objects ranks (not
      * just promote/demote) since we need to remember some volume info (such as 'all ssd')
      * when we re-compute ranking before migrator gets the delta change table */
     obj_rank_cache_t rankDeltaChgTbl;
     obj_rank_cache_t tmp_chgTbl; /* this is temporary space to work on during ranking process */
     rank_order_objects_t lowrank_objs; /* cache of N lowert rank objects in the rank table */
     fds_mutex* tbl_mutex; /* for both rankDeltaChgTbl and lowrank_objs */

     /* cache lowest rank in the ranking table for queries from the tiering engine 
      * this value is updated only at the end of each ranking process */
     fds_uint32_t tail_rank; /* updated only by the ranking thread */

     /* True if ranking process is enabled */
     std::atomic<fds_bool_t> rankingEnabled;
     /* for now one thread doing ranking process */
     boost::thread *rank_thread;
     fds_notification* rank_notify;

     /* does not own, passed to the constructor */
     ObjStatsTracker *obj_stats;

     /* does not own, passed to the constructor */
     StorMgrVolumeTable* sm_volTbl;

     /* timer to periodically get stats from stat tracker and 
      * and make 'demote/promote' decisions based on out cached lowrank list */
     FdsTimerPtr rankTimer;
     FdsTimerTaskPtr rankTimerTask;
};

class RankTimerTask: public FdsTimerTask {
    public:
     ObjectRankEngine* rank_eng;

     RankTimerTask(FdsTimer &timer, ObjectRankEngine* _rank_eng)
         : FdsTimerTask(timer) {
         rank_eng = _rank_eng;
     }
     ~RankTimerTask() {}

     void runTimerTask();
};

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_OBJRANK_H_
