/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_RANKENGINE_H_
#define SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_RANKENGINE_H_

#include <fds_types.h>
#include <fds_volume.h>
#include <persistent-layer/dm_io.h>
#include <set>

namespace fds {

/**
 * Typedef for our set of promotable items
 */
typedef std::set<ObjectID> PromotionSet;

class RankEngine {
  public:
    // TODO(brian): Add method for asking if promotion can occur

    /**
    * Calculate and return a set of objects that should be promoted.
    *
    * @param max_size the maximum cardinality of the set to be returned.
    * @param oidSet the set of ObjectIDs.  The caller provides a reference
    *               to a set, and the Object Ranking Policy engine fills
    *               as many ObjectIDs as possible.
    *
    * Note: that max_size should be an upper bound, and PromotionSets
    * smaller than max_size can be returned.
    */
    virtual void getObjectsToPromote(fds_uint32_t maxSize,
                                     PromotionSet& oidSet) = 0;

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
    virtual void notifyDataPath(fds_io_op_t opType, const ObjectID& oid,
            diskio::DataTier tier, const VolumeDesc& voldesc) = 0;
};
}  // namespace fds
#endif  // SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_RANKENGINE_H_
