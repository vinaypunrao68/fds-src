/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_RANDOMRANKPOLICY_H_
#define SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_RANDOMRANKPOLICY_H_

#include <fds_types.h>
#include <set>

#include <persistent-layer/dm_io.h>

#include <object-store/RankEngine.h>

namespace fds {
class RandomRankPolicy : public RankEngine {
  public:
    RandomRankPolicy() {}
    ~RandomRankPolicy() {}
    virtual void getObjectsToPromote(fds_uint32_t maxSize,
                                     PromotionSet& oidSet);

    virtual fds_bool_t isObjectDemotable(const ObjectID& oid);

    virtual void notifyDataPath(fds_io_op_t opType, const ObjectID& oid,
            diskio::DataTier tier);
};
}  // namespace fds
#endif  // SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_RANDOMRANKPOLICY_H_
