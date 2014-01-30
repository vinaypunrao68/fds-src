/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <list>

#include <OmDataPlacement.h>

namespace fds {
/**********
 * Functions definitions for placement
 * algorithms
 **********/
Error
RoundRobinAlgorithm::computeNewDlt(const ClusterMap &currMap,
                                   const std::list<boost::shared_ptr<NodeAgent>>
                                   &addNodes,
                                   const std::list<boost::shared_ptr<NodeAgent>>
                                   &rmNodes,
                                   const FdsDlt &currDlt,
                                   fds_uint64_t depth,
                                   fds_uint64_t width) {
    Error err(ERR_OK);
    return err;
}

Error
ConsistHashAlgorithm::computeNewDlt(const ClusterMap &currMap,
                                    const std::list<boost::shared_ptr<NodeAgent>>
                                    &addNodes,
                                    const std::list<boost::shared_ptr<NodeAgent>>
                                    &rmNodes,
                                    const FdsDlt &currDlt,
                                    fds_uint64_t depth,
                                    fds_uint64_t width) {
    Error err(ERR_OK);
    return err;
}
}  // namespace fds
