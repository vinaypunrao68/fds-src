/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_TIERENGINE_H_
#define SOURCE_STOR_MGR_INCLUDE_TIERENGINE_H_

#include <string>
#include <fds_module.h>
#include <StorMgrVolumes.h>
#include <ObjRank.h>
#include <persistent-layer/dm_io.h>

namespace fds {

class ObjStatsTracker;
class TierEngine;
/*
 * Abstract base that defines what a migration algorithm
 * class should provide.
 */
class MigrationAlgo {
    private:
     /*
      * Member references to needed external objects
      * Stats: provides obj/vol usage stats
      * Ranker: provides in flash obj ranks
      * Volume meta: provides vol placement metadata
      */
    public:
     /*
      * Determines the next object in disk that
      * should be promoted to flash.
      *
      * @return an object to promote or invalid
      * object id if nothing to promote
      */
     virtual const ObjectID& objToPromote() = 0;
     /*
      * Determines the next object in flash that
      * should be demoted to disk.
      *
      * @return an object to demote or invalid
      * object id if nothing to promote
      */
     virtual const ObjectID& objToDemote()  = 0;
};

/*
 * Abstract base class for inline placement
 * algorithms
 */
class TierPutAlgo {
    private:
     /*
      * Member references to needed external objects
      * Stats: provides obj/vol usage stats
      * Ranker: provides in flash obj ranks
      * Volume meta: provides vol placement metadata 
      */
    public:
     /*
      * Determines the tier to place an object for a
      * volume.
      *
      * @return the tier to place the object
      */
     virtual diskio::DataTier selectTier(const ObjectID &oid,
                                         fds_volid_t     vol) = 0;
     virtual ~TierPutAlgo() {}
};

const fds_uint32_t max_migration_threads = 30;

/*
 * Class responsible for background migration
 * of data between tiers
 */
class TierMigration {
    private:
     /*
      * Members that manage the migration threads
      * running
      */
     fds_uint32_t    numThrds;
     fds_threadpool* threadPool;
     MigrationAlgo*  migAlgo;

     /*
      * Function to be run in a tier migration thread.
      */

    public:
     void startRankTierMigration(void);
     void stopRankTierMigration(void);
     TierEngine *tier_eng;
     fds_bool_t   stopMigrationFlag;

     TierMigration(fds_uint32_t _nThreads, TierEngine *te);
     ~TierMigration();
};

/*
 * Defines the main class that determines tier
 * placement for objects.
 */
class TierEngine : public Module {
    public:
     typedef enum {
         FDS_TIER_PUT_ALGO_RANDOM,
         FDS_TIER_PUT_ALGO_BASIC_RANK,
     } tierPutAlgoType;

    private:
     fds_uint32_t  numMigThrds;

     /*
      * Member algorithms.
      */
     tierPutAlgoType algoType;
     TierPutAlgo *tpa;

     /*
      * Member references to external objects.
      * Stats: provides obj/vol usage stats
      * Ranker: provides in flash obj ranks
      * Volume meta: provides vol placement metadata
      */
     StorMgrVolumeTable* sm_volTbl;

    public:
     TierMigration *migrator;

     /// Ranking engine
     ObjectRankEngine* rankEng;
     ObjStatsTracker *objStats;

     /*
      * Constructor for tier engine. This will take
      * references to required external classes and
      * start the tier migration threads.
      */
     TierEngine(const std::string &modName,
                tierPutAlgoType _algo_type,
                StorMgrVolumeTable* _sm_volTbl);
     ~TierEngine();

     typedef std::unique_ptr<TierEngine> unique_ptr;

     /**
      * Determines the tier for an object that's
      * looking for a tier. Intended to be called
      * on a put path.
      *
      * @param[in] oid ID of object looking for tier
      * @param[ib] vol Object's volume
      *
      * @return the tier to place object
      */
     diskio::DataTier selectTier(const ObjectID &oid,
                                 fds_volid_t     vol);

     /**
      * Called when new object is inserted to flash tier
      */
     void handleObjectPutToFlash(const ObjectID& objId,
                                 const VolumeDesc& voldesc);

     // FDS module methods
     int  mod_init(SysParams const *const param);
     void mod_startup();
     void mod_shutdown();
};

}  // namespace fds
#endif  // SOURCE_STOR_MGR_INCLUDE_TIERENGINE_H_
