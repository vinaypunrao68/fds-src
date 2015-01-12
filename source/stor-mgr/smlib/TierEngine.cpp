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

// TODO(Rao): Move this code to separate file
HybridTierPolicy::HybridTierPolicy()
{
    snapRequest_.io_type = FDS_SM_SNAPSHOT_TOKEN;
    snapRequest_.smio_snap_resp_cb = std::bind(&HybridTierPolicy::snapTokenCb,
                                              this,
                                              std::placeholders::_1,
                                              std::placeholders::_2,
                                              std::placeholders::_3,
                                              std::placeholders::_4);

    moveTierRequest_.oidList.reserve(HYBRID_POLICY_MIGRATION_BATCH_SZ);
    moveTierRequest_.fromTier = diskio::DataTier::flashTier;
    moveTierRequest_.toTier = diskio::DataTier::diskTier;
    moveTierRequest_.relocate = true;
    moveTierRequest_.moveObjsRespCb = std::bind(&HybridTierPolicy::moveObjsToTierCb,
                                                this,
                                                std::placeholders::_1,
                                                std::placeholders::_2);
}

void HybridTierPolicy::run()
{
    if (curTokenIdx_ == -1) {
        /* No previous state. Start from the first token */
        curTokenIdx_ = 0;
    }
    fds_verify(tokenList_.size() > 0);
    snapToken_();
}

HybridTierPolicy::snapToken_()
{
    snapRequest_.token_id = tokenList_[curTokenIdx_];
    // TODO(Rao): Enque request
}

HybridTierPolicy::snapTokenCb(const Error& err,
                                SmIoSnapshotObjectDB* snapReq,
                                leveldb::ReadOptions& options,
                                leveldb::DB* db)
{
    GLOGDEBUG << "Snapshot complete for token: " << tokenList_[curTokenIdx_];

    /* Initialize the tokenItr */
    tokenItr_.reset(new SMTokenItr());
    tokenItr_->itr = db->NewIterator(options);
    tokenItr_->itr->SeekToFirst();
    tokenItr_->db = db;
    tokenItr_->options = options;

    constructTierMigrationList_();
}

HybridTierPolicy::constructTierMigrationList_()
{
    // TODO(Rao): Assert pre conditions
    fds_assert(moveTierRequest_.oidList.empty() == true);

    ObjMetaData omd;

    /* Construct object list to update tier location from flash to disk */
    auto &itr = tokenItr_->itr;
    for (; itr->Valid() && tierMigrationList_.size() < HYBRID_POLICY_MIGRATION_BATCH_SZ;
         itr->Next()) {
        omd.deserializeFrom(itr->Value());
        if (omd.onFlashTier() && omd.getCreationTime() < hybridMoveTs_) {
            moveTierRequest_.oidList.push_back(ObjectID(itr->Key()));
        }
    }

    // TODO(Rao): Enqueue request
}

void HybridTierPolicy::moveObjsToTierCb(const Error& e,
                                        SmIoMoveObjsToTier *req)
{
    if (e != ERR_OK) {
        LOGWARN << "Failed to move some objects to disk from flash for token: "
            << tokenList_[curTokenIdx_];
    }

    if (tokenItr_->itr->Valid()) {
        moveTierRequest_.oidList.clear();
        constructTierMigrationList_();
        return;
    }
    
    LOGDEBUG << "Moved objects for token: " << tokenList_[curTokenIdx_];

    /* Release snapshot */
    tokenItr_->itr = nullptr;
    tokenItr_->db->ReleaseSnapshot(tokenItr_->options.snapshot);
    tokenItr_->db = nullptr;
    tokenItr_->done = true;

    curTokenIdx_++;

    if (curTokenIdx_ < tokenList_.size()) {
        /* Start moving objects for the next token */
        snapToken_();
    } else {
        /* Completed moving objects.  Schedule the next relocation task */
        // TODO(Rao):
    }
}

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
        case FDS_BLOOM_FILTER_TIME_DECAY_RANK_POLICY:
        case FDS_COUNT_MIN_SKETCH_RANK_POLICY:
        default:
            fds_panic("Invalid or unsupported rank policy provided!");
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
