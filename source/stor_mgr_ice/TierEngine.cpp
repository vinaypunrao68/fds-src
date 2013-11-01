/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <include/TierEngine.h>
#include <include/TierPutAlgorithms.h>

namespace fds {

TierEngine::TierEngine(tierPutAlgoType _algo_type,
		       StorMgrVolumeTable* _sm_volTbl,
		       ObjectRankEngine* _rank_eng,
		       fds_log* _log) :
  rank_eng(_rank_eng),
  sm_volTbl(_sm_volTbl),
  te_log(_log)
{
  switch(_algo_type) {
  case FDS_TIER_PUT_ALGO_BASIC_RANK:
    tpa = new RankTierPutAlgo(_sm_volTbl, _rank_eng, _log);
    FDS_PLOG(te_log) << "TierEngine: will use basic rank tier put algorithm";
    break;
  default:
    tpa = new RandomTestAlgo();
    FDS_PLOG(te_log) << "TierEngine: will use random test tier put algorithm";
  }

}

/*
 * Note caller is responsible for freeing
 * the algorithm structure at the moment
 */
TierEngine::~TierEngine() {
  delete tpa;
}

/*
 * TODO: Make this interface take volId instead of the entire
 * vol struct. A lookup should be done internally.
 */
diskio::DataTier TierEngine::selectTier(const ObjectID    &oid,
                                        fds_volid_t        vol) {
  return tpa->selectTier(oid, vol);
}

}  // namespace fds
