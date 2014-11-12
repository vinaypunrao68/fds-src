/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_TIERINGCONFIG_H_
#define SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_TIERINGCONFIG_H_

#include <fds_types.h>

namespace fds {

/**
 * Tiering engine configurable parameters
 * Parameters are internal, and could be changing dynamically
 * (in the future), but for now static
 */
class TieringParams {
  public:
    /**
     * Threshold for percent of used SSD capacity; when threshold is reached,
     * PUT to hybrid SSD preferred volumes will go to HDD, in order to keep
     * some remaining capacity for SSD-only volumes
     */
    fds_uint32_t flashFullThreshold;
    /**
     * Max number of objects that are batched into a single QoS request
     * for writing objects that are put to flash tier to HDD tier
     */
    fds_uint32_t maxWritebackBatchSize;
    /**
     * Max number of objects that are batched into a single QoS request
     * for promotion to a flash tier
     */
    fds_uint32_t maxPromoteBatchSize;

    friend std::ostream& operator<< (std::ostream &out,
                                     const TieringParams& params) {
        return out << "TieringParams flashFullThreshold = " << params.flashFullThreshold
                   << "%, maxWritebackBatchSize = " << params.maxWritebackBatchSize
                   << " objects, maxPromoteBatchSize = " << params.maxPromoteBatchSize;
    }
};

}  // namespace fds
#endif  // SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_TIERINGCONFIG_H_
