/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_TIERENGINE_H_
#define SOURCE_STOR_MGR_INCLUDE_TIERENGINE_H_

#include <string>
#include <fds_module.h>
#include <StorMgrVolumes.h>
#include <ObjRank.h>
#include <TierMigration.h>
#include <persistent-layer/dm_io.h>

namespace fds {

class RankEngine;

const fds_uint32_t max_migration_threads = 30;

/*
 * Defines the main class that determines tier
 * placement for objects.
 */
class TierEngine : public Module {
  public:
    typedef enum {
        FDS_RANDOM_RANK_POLICY,
        FDS_COUNTING_BLOOM_RANK_POLICY
    } rankPolicyType;

  private:
    fds_uint32_t  numMigThrds;
    boost::shared_ptr<RankEngine> rankEngine;
    
    /*
     * Member references to external objects.
     * Stats: provides obj/vol usage stats
     * Ranker: provides in flash obj ranks
     * Volume meta: provides vol placement metadata
     */
    StorMgrVolumeTable* sm_volTbl;
    SmTierMigration* migrator;

  public:
    /*
     * Constructor for tier engine. This will take
     * references to required external classes and
     * start the tier migration threads.
     */
    TierEngine(const std::string &modName,
            rankPolicyType _rank_type,
            StorMgrVolumeTable* _sm_volTbl,
            SmIoReqHandler* storMgr);
    ~TierEngine();

    typedef std::unique_ptr<TierEngine> unique_ptr;

    /**
    * Determines the tier for an object that's
    * looking for a tier. Intended to be called
    * on a put path.
    *
    * @param[in] oid ID of object looking for tier
    * @param[in] vol Object's volume
    *
    * @return the tier to place object
    */
    // TODO(brian): If we have capacity put to SSD (in SSD case), otherwise right to HDD
    // Remove all calls to RankEngine
    diskio::DataTier selectTier(const ObjectID &oid,
            const VolumeDesc& voldesc);

    /**
    * Called on put/get/delete, and handles any tasks that should occur on IO.
    * For example, will call RankEngine's updateDataPath method, enabling the
    * RankEngine to update it's internal stats for rank calculations.
    *
    * @param objId[in] Object that was acted on
    * @param opType[in] type of operation that occurred
    * @param voldesc[in] Volume descriptor for the object that was acted on
    * @param tier[in] The tier that the IO occurred on
    */
    void notifyIO(const ObjectID& objId,
            fds_io_op_t opType,
            const VolumeDesc& volDesc,
            diskio::DataTier tier);

    // FDS module methods
    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();
};

}  // namespace fds
#endif  // SOURCE_STOR_MGR_INCLUDE_TIERENGINE_H_
