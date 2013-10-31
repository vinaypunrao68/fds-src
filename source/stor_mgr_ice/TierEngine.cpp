/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <include/TierEngine.h>

namespace fds {

TierEngine::TierEngine(TierPutAlgo *algo) :
    tpa(algo) {
}

/*
 * Note caller is responsible for freeing
 * the algorithm structure at the moment
 */
TierEngine::~TierEngine() {
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
