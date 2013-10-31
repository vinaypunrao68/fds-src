/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_ICE_INCLUDE_TIERPUTALGORITHMS_H_
#define SOURCE_STOR_MGR_ICE_INCLUDE_TIERPUTALGORITHMS_H_

#include "stor_mgr_ice/include/TierEngine.h"

namespace fds {

  /*
   * A test algorithm that just selects a random tier.
   * Obviously not practical, but useful for testing.
   */
  class RandomTestAlgo : public TierPutAlgo {
 private:
 public:
    diskio::DataTier selectTier(const ObjectID &oid,
                                fds_volid_t     vol) {
      if ((random() % 2) == 0) {
        return diskio::flashTier;
      }
      return diskio::diskTier;
    }
  };

}  // namespace fds

#endif  // SOURCE_STOR_MGR_ICE_INCLUDE_TIERPUTALGORITHMS_H_
