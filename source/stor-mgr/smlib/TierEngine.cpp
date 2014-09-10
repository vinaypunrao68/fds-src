/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <utility>
#include <TierEngine.h>
#include <TierPutAlgorithms.h>

namespace fds {

extern ObjectStorMgr *objStorMgr;
TierEngine::TierEngine(tierPutAlgoType _algo_type,
                       StorMgrVolumeTable* _sm_volTbl,
                       ObjectRankEngine* _rank_eng,
                       fds_log* _log) :
        rank_eng(_rank_eng),
        sm_volTbl(_sm_volTbl),
        te_log(_log) {
    switch (_algo_type) {
        case FDS_TIER_PUT_ALGO_BASIC_RANK:
            tpa = new RankTierPutAlgo(_sm_volTbl, _rank_eng, _log);
            FDS_PLOG(te_log) << "TierEngine: will use basic rank tier put algorithm";
            break;
        default:
            tpa = new RandomTestAlgo();
            FDS_PLOG(te_log) << "TierEngine: will use random test tier put algorithm";
    }
    migrator = new TierMigration(max_migration_threads, this, _log);
}

/*
 * Note caller is responsible for freeing
 * the algorithm structure at the moment
 */
TierEngine::~TierEngine() {
    delete tpa;
    delete migrator;
}

/*
 * TODO: Make this interface take volId instead of the entire
 * vol struct. A lookup should be done internally.
 */
diskio::DataTier TierEngine::selectTier(const ObjectID    &oid,
                                        fds_volid_t        vol) {
    return tpa->selectTier(oid, vol);
}



TierMigration::TierMigration(fds_uint32_t _nthreads,
                             TierEngine *te,
                             fds_log *log) :
        tm_log(log),
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
            FDS_PLOG(migrator->tm_log) << "rankMigrationJob: chg table obj "
                                       << oid.ToHex() << " will be promoted";
            objStorMgr->relocateObject(oid, diskio::diskTier, diskio::flashTier);
        } else {
            FDS_PLOG(migrator->tm_log) << "rankMigrationJob: chg table obj "
                                       << oid.ToHex() << " will be demoted";
            objStorMgr->relocateObject(oid, diskio::flashTier, diskio::diskTier);
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
        count = tier_eng->rank_eng->getDeltaChangeTblSegment(len, chg_tbl);
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
