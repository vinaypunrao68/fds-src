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
    typedef std::set<ObjectID> PromotionSet;

    virtual PromotionSet getObjectsToPromote(fds_uint32_t max_size) = 0;
    virtual fds_bool_t isObjectDemotable(const ObjectID& oid) = 0;
    virtual void notifyDataPath(fds_io_op_t opType, const ObjectID& oid, diskio::DataTier tier) = 0;
};
}  // namespace fds
#endif  // SOURCE_STOR_MGR_INCLUDE_RANKENGINE_H_
