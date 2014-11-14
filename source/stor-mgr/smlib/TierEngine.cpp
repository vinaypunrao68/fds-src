/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <utility>
#include <string>
#include <fds_process.h>
#include <ObjStats.h>
#include <object-store/RankEngine.h>
#include <TierEngine.h>
#include <object-store/RandomRankPolicy.h>

namespace fds {

TierEngine::TierEngine(const std::string &modName,
        rankPolicyType _rank_type,
        StorMgrVolumeTable* _sm_volTbl,
        SmIoReqHandler* storMgr) :
        Module(modName.c_str()),
        migrator(nullptr),
        sm_volTbl(_sm_volTbl) {
    switch (_rank_type) {
        case FDS_RANDOM_RANK_POLICY:
            rankEngine = boost::shared_ptr<RankEngine>(new RandomRankPolicy(storMgr, 50));
            break;
        case FDS_COUNTING_BLOOM_RANK_POLICY:
            break;
        default:
            fds_panic("No valid rank policy provided!");
    }

    migrator = new SmTierMigration(storMgr);
}


TierEngine::~TierEngine() {
    if (migrator != nullptr) {
        delete migrator;
    }
}

int TierEngine::mod_init(SysParams const *const param) {
    Module::mod_init(param);

    /*
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
    */

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

    // TODO(brian): Add check for whether or not this will exceed SSD capacity
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
        // and return appropriate
        ret_tier = diskio::flashTier;
    } else {  // else ret_tier already set to disk
        LOGDEBUG << "RankTierPutAlgo: selectTier received unexpected media policy: "
                << media_policy;
    }
    return ret_tier;
}

void
TierEngine::notifyIO(const ObjectID& objId, fds_io_op_t opType,
        const VolumeDesc& volDesc, diskio::DataTier tier) {
    // Only notifyDataPath on hybrid IOs
    if (volDesc.mediaPolicy == fpi::FDSP_MEDIA_POLICY_HYBRID ||
            volDesc.mediaPolicy == fpi::FDSP_MEDIA_POLICY_HYBRID_PREFCAP) {
        rankEngine->notifyDataPath(opType, objId, tier);
    }
    if ((opType == FDS_SM_PUT_OBJECT) && (tier == diskio::flashTier)) {
        migrator->notifyHybridVolFlashPut(objId);
    }
}
}  // namespace fds
