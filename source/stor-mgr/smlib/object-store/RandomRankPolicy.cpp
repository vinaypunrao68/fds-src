/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <object-store/RandomRankPolicy.h>

namespace fds {

/**
 * This is the Version1 of the Object Rank Policy.
 *
 * This algorithm randomly choose a ObjectID for promotion or
 * demotion in a hybrid tier model.
 *
 * If other algorithms can't do better than this algorithm, it's
 * not worth pursuing it.
 */

/**
* Determine and return a set of objects to be promoted.
* @param max_size The maxiumum number of objects to find for promotion.
* Note that max is just an upper bound.
* @param oidSet A set of ObjectIDs returned to the caller.
*
* @returns PromotionSet of objects to be promoted
*/
void
RandomRankPolicy::getObjectsToPromote(fds_uint32_t maxSize,
                                      PromotionSet& oidSet)
{
    // Do random selection
    return;
}

/**
* Determine if an object should be demoted or not. This will be called
* by the scavenger during garbage collection to determine if an object
* should be removed from SSD or not.
*
* @param oid An objectID of the object to check on
*
* @returns true if the object should be demoted, false otherwise
*/
fds_bool_t
RandomRankPolicy::isObjectDemotable(const ObjectID& oid)
{
    return false;
}

/**
* Gets called every time an IO occurs to alert the rank engine to update
* data structures, etc.
*/
void
RandomRankPolicy::notifyDataPath(fds_io_op_t opType,
        const ObjectID& oid,
        diskio::DataTier tier,
        const VolumeDesc& voldesc)
{
    return;
}

}  // namespace fds
