/*
 * Copyright 2013-2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_MIGRATIONTOOLS_H_
#define SOURCE_STOR_MGR_INCLUDE_MIGRATIONTOOLS_H_

#include <list>
#include <utility>
#include <boost/shared_ptr.hpp>

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
using elem_type = boost::shared_ptr<ObjMetaData>;
using metadata_diff_type = output_type<std::pair<elem_type, elem_type>>;

void
diff(leveldb::DB* db,
     leveldb::Snapshot const* lhs,
     leveldb::Snapshot const* rhs,
     metadata_diff_type& diff);

}  // namespace metadata
}  // namespace fds


#endif  // SOURCE_STOR_MGR_INCLUDE_MIGRATIONTOOLS_H_
