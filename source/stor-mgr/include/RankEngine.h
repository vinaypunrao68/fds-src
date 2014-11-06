/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_RANKENGINE_H_
#define SOURCE_STOR_MGR_INCLUDE_RANKENGINE_H_

#include <fds_types.h>
#include <persistent-layer/dm_io.h>
#include <set>

namespace fds {
class RankEngine {
  public:
    /**
    * Typedef for our set of promotable items
    */
    typedef std::set<ObjectID> PromotionSet;

    /**
    * Calculate and return a set of objects that should be promoted.
    *
    * @param max_size the maximum cardinality of the set to be returned.
    * Note that max_size should be an upper bound, and PromotionSets
    * smaller than max_size can be returned.
    */
    virtual PromotionSet getObjectsToPromote(fds_uint32_t max_size) = 0;

    /**
    * Determine if a particular object should be demoted or not.
    * @param oid The ID of the object to check
    *
    * @returns True if the object should be demoted, false otherwise
    */
    virtual fds_bool_t isObjectDemotable(const ObjectID& oid) = 0;

    /**
    * Called on every IO to notify the rank policy of an IO. This method
    * should perform lightweight stats collection and tracking to enable
    * object ranking.
    *
    * @param opType the operation type (read/write)
    * @param oid The ID of the object that is being acted upon
    * @param tier The tier that the action occurred on
    */
    virtual void notifyDataPath(fds_io_op_t opType, const ObjectID& oid, diskio::DataTier tier) = 0;
};
}  // namespace fds
#endif  // SOURCE_STOR_MGR_INCLUDE_RANKENGINE_H_
