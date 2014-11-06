/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_RANKENGINE_H_
#define SOURCE_STOR_MGR_INCLUDE_RANKENGINE_H_

#include <fds_types.h>
#include <dm_io.h>

namespace fds {
class RankEngine {
  private:

  public:
    typedef std::set<ObjectID> PromotionSet;

    virtual RankEngine() = 0;

    virtual PromotionSet getObjectsToPromote(fds_uint_t max_size) = 0;
    virtual bool isObjectDemotable(ObjectID oid) = 0;
    virtual void notifyDataPath(fds_io_op_t opType, const ObjectID& oid, diskio::DataTier tier) = 0;
};
}  // namespace fds
#endif  // SOURCE_STOR_MGR_INCLUDE_TIERENGINE_H_