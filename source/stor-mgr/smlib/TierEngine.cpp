/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <utility>
#include <string>
#include <fds_process.h>
#include <ObjStats.h>
#include <TierEngine.h>

namespace fds {

TierEngine::TierEngine(const std::string &modName,
                       tierPutAlgoType _algo_type,
                       StorMgrVolumeTable* _sm_volTbl) :
        Module(modName.c_str()),
        algoType(_algo_type),
        sm_volTbl(_sm_volTbl) {
    objStats = new ObjStatsTracker();
}

/*
 * Note caller is responsible for freeing
 * the algorithm structure at the moment
 */
TierEngine::~TierEngine() {
    delete objStats;
    delete rankEng;
    delete migrator;
}

int TierEngine::mod_init(SysParams const *const param) {
    Module::mod_init(param);

    boost::shared_ptr<FdsConfig> conf = g_fdsprocess->get_fds_config();

    // config for rank engine
    fds_uint32_t rank_tbl_size = DEFAULT_RANK_TBL_SIZE;
    fds_uint32_t hot_threshold = DEFAULT_HOT_THRESHOLD;
    fds_uint32_t cold_threshold = DEFAULT_COLD_THRESHOLD;
    fds_uint32_t rank_freq_sec = DEFAULT_RANK_FREQUENCY;
    // if tier testing is on, config will over-write default tier params
    if (conf->get<bool>("fds.sm.testing.test_tier")) {
        rank_tbl_size = conf->get<int>("fds.sm.testing.rank_tbl_size");
        rank_freq_sec = conf->get<int>("fds.sm.testing.rank_freq_sec");
        hot_threshold = conf->get<int>("fds.sm.testing.hot_threshold");
        cold_threshold = conf->get<int>("fds.sm.testing.cold_threshold");
    }
    std::string obj_stats_dir = g_fdsprocess->proc_fdsroot()->dir_fds_var_stats();
    rankEng = new ObjectRankEngine(obj_stats_dir, rank_tbl_size, rank_freq_sec,
                                   hot_threshold, cold_threshold, sm_volTbl, objStats);

    migrator = new TierMigration(max_migration_threads, this);
    return 0;
}

void TierEngine::mod_startup() {
}

void TierEngine::mod_shutdown() {
}

/*
 * TODO: Make this interface take volId instead of the entire
 * vol struct. A lookup should be done internally.
 */
diskio::DataTier TierEngine::selectTier(const ObjectID    &oid,
                                        const VolumeDesc& voldesc) {
    diskio::DataTier ret_tier = diskio::diskTier;
    FDSP_MediaPolicy media_policy = voldesc.mediaPolicy;
    fds_uint32_t rank;

    if (media_policy == FDSP_MEDIA_POLICY_SSD) {
        /* if 'all ssd', put to ssd */
        ret_tier = diskio::flashTier;
    } else if ((media_policy == FDSP_MEDIA_POLICY_HDD) ||
            (media_policy == FDSP_MEDIA_POLICY_HYBRID_PREFCAP)) {
        /* if 'all disk', put to disk
         * or if hybrid but first preference to capacity tier, put to disk  */
        ret_tier = diskio::diskTier;
    } else if (media_policy == FDSP_MEDIA_POLICY_HYBRID) {
        /* hybrid tier policy */
        fds_uint32_t rank = rankEng->getRank(oid, voldesc);
        if (rank < rankEng->getTblTailRank()) {
            /* lower value means higher rank */
            ret_tier = diskio::flashTier;
        }
    } else {  // else ret_tier already set to disk
        LOGDEBUG << "RankTierPutAlgo: selectTier received unexpected media policy: "
                << media_policy;
    }

    return ret_tier;
}

void
TierEngine::handleObjectPutToFlash(const ObjectID& objId,
                                   const VolumeDesc& voldesc) {
    rankEng->rankAndInsertObject(objId, voldesc);
}

TierMigration::TierMigration(fds_uint32_t _nthreads,
                             TierEngine *te) :
        tier_eng(te) {
    threadPool = new fds_threadpool(_nthreads);
    stopMigrationFlag = true;
}

TierMigration::~TierMigration() {
    stopMigrationFlag = true;
    delete threadPool;
}

void migrationJob(TierMigration *migrator, void *arg, fds_uint32_t count) {
    fds_uint32_t len = 100;
    std::pair<ObjectID, ObjectRankEngine::rankOperType> * chg_tbl = (
        std::pair<ObjectID, ObjectRankEngine::rankOperType> *) arg;
    ObjectID oid;
    for (uint i = 0; i < count; ++i)
    {
        if (migrator->stopMigrationFlag) return;
        oid = chg_tbl[i].first;
        if (chg_tbl[i].second == ObjectRankEngine::OBJ_RANK_PROMOTION) {
            LOGDEBUG << "rankMigrationJob: chg table obj "
                     << oid << " will be promoted";
            // objStorMgr->relocateObject(oid, diskio::diskTier, diskio::flashTier);
        } else {
            LOGDEBUG << "rankMigrationJob: chg table obj "
                     << oid << " will be demoted";
            // objStorMgr->relocateObject(oid, diskio::flashTier, diskio::diskTier);
        }
    }
    delete []  chg_tbl;
}

void TierMigration::startRankTierMigration(void) {
    stopMigrationFlag = false;
    fds_uint32_t len = 100;
    fds_uint32_t count = 0;
    do {
        std::pair<ObjectID, ObjectRankEngine::rankOperType> *chg_tbl =
                new std::pair<ObjectID, ObjectRankEngine::rankOperType> [len];
        count = tier_eng->rankEng->getDeltaChangeTblSegment(len, chg_tbl);
        if (count == 0)  {
            delete [] chg_tbl;
            break;
        }
        threadPool->schedule(migrationJob, this, static_cast<void *>(chg_tbl), count);
    } while (count > 0 && !stopMigrationFlag);
}

void TierMigration::stopRankTierMigration(void) {
    stopMigrationFlag = true;
}

}  // namespace fds
