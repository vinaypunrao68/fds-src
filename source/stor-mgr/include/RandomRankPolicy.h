/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_RANK_POLICIES_RANDOMRANKPOLICY_H_
#define SOURCE_STOR_MGR_INCLUDE_RANK_POLICIES_RANDOMRANKPOLICY_H_

#include <fds_types.h>
#include <persistent-layer/dm_io.h>
#include <set>

namespace fds {
class RandomRankPolicy : public RankEngine {
  public:
    virtual RandomRankPolicy() {}
    virtual PromotionSet getObjectsToPromote(fds_uint32_t max_size);
    virtual fds_bool_t isObjectDemotable(const ObjectID& oid);
    virtual void notifyDataPath(fds_io_op_t opType, const ObjectID& oid, diskio::DataTier tier);
};
}  // namespace fds
#endif  // SOURCE_STOR_MGR_INCLUDE_RANK_POLICIES_RANDOMRANKPOLICY_H_