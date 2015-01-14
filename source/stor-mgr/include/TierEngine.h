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
#include <HybridTierCtrlr.h>
#include <persistent-layer/dm_io.h>
#include <object-store/SmDiskMap.h>

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
        FDS_BLOOM_FILTER_TIME_DECAY_RANK_POLICY,
        FDS_COUNT_MIN_SKETCH_RANK_POLICY
    } rankPolicyType;

    /*
     * Constructor for tier engine. This will take
     * references to required external classes and
     * start the tier migration threads.
     */
    TierEngine(const std::string &modName,
            rankPolicyType _rank_type,
            StorMgrVolumeTable* _sm_volTbl,
            const SmDiskMap::ptr& diskMap,
            SmIoReqHandler* storMgr);
    ~TierEngine();

    typedef std::unique_ptr<TierEngine> unique_ptr;

    /**
     * Enable/disable tier migration. Does not enable/disable
     * write-back thread
     * TODO(Anna) this does not do anything yet actually, because
     * we do not do tier migration yet.
     */
    void disableTierMigration();
    void enableTierMigration();
    /* For manually starting hybrid tier controller */
    void startHybridTierCtrlr();

    /** Gets the threshold for considering SSDs full from
    * the tier migration policy.
    *
    * @returns number between 0-100 that represents the % of
    * SSD capacity that must be used before the SSD is considered 'full'.
    */
    inline fds_uint32_t getFlashFullThreshold() const {
        return migrator->flashFullThreshold();
    }

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
    * @param ioSize[in] the size in bytes of the IO
    */
    void notifyIO(const ObjectID& objId,
            fds_io_op_t opType,
            const VolumeDesc& volDesc,
            diskio::DataTier tier);

    // FDS module methods
    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();

  private:
    uint32_t  numMigThrds;
    boost::shared_ptr<RankEngine> rankEngine;

    StorMgrVolumeTable* sm_volTbl;
    SmTierMigration* migrator;

    HybridTierCtrlr hybridTierCtrlr;
};

}  // namespace fds
#endif  // SOURCE_STOR_MGR_INCLUDE_TIERENGINE_H_
