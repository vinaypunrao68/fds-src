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
                                 const VolumeDesc& voldesc);

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
