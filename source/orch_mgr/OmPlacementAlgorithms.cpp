/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <OmDataPlacement.h>

namespace fds {
/**********
 * Functions definitions for placement
 * algorithms
 **********/
Error
RoundRobinAlgorithm::computeNewDlt(const ClusterMap &currMap,
                                   const FdsDlt &currDlt,
                                   fds_uint64_t depth,
                                   fds_uint64_t width) {
    Error err(ERR_OK);
    return err;
}

Error
ConsistHashAlgorithm::computeNewDlt(const ClusterMap &currMap,
                                    const FdsDlt &currDlt,
                                    fds_uint64_t depth,
                                    fds_uint64_t width) {
    Error err(ERR_OK);
    return err;
}
}  // namespace fds
