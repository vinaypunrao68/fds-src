/*
 * Copyright 2013-2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_MIGRATIONTOOLS_H_
#define SOURCE_STOR_MGR_INCLUDE_MIGRATIONTOOLS_H_

#include <list>
#include <utility>

namespace leveldb
{
struct DB;
struct Snapshot;
}  // namespace leveldb

namespace fds
{
struct ObjMetaData;
namespace metadata
{

template<typename T>
using output_type = std::list<T>;
using metadata_diff_type = output_type<std::pair<ObjMetaData*, ObjMetaData*>>;

void
diff(leveldb::DB* db,
     leveldb::Snapshot const* lhs,
     leveldb::Snapshot const* rhs,
     metadata_diff_type& diff);

}  // namespace metadata
}  // namespace fds


#endif  // SOURCE_STOR_MGR_INCLUDE_MIGRATIONTOOLS_H_
