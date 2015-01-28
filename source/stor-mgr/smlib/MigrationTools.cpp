/*
 * Copyright 2013-2015 Formation Data Systems, Inc.
 */

#include <list>
#include <utility>
#include <leveldb/db.h>
#include "ObjMeta.h"

#include "MigrationTools.h"

namespace fds {
namespace metadata {

using leveldb::DB;
using leveldb::ReadOptions;
using leveldb::Snapshot;

enum class cmp_result { removal, exists, addition };

// Make the result of comparison function easier to follow
constexpr cmp_result
tri_comp(int const c)
{ return (c <= 0) ? ((c == 0) ? cmp_result::exists : cmp_result::removal) : cmp_result::addition; }

void
diff(DB* db, Snapshot const* lhs, Snapshot const* rhs, metadata_diff_type& diff) {
    // Create the needed ReadOptions objects
    ReadOptions l_opts; l_opts.snapshot = lhs;
    ReadOptions r_opts; r_opts.snapshot = rhs;

    // Obtain iterators to the snapshots
    auto l_it   = db->NewIterator(l_opts);
    auto r_it   = db->NewIterator(r_opts);

    l_it->SeekToFirst();
    r_it->SeekToFirst();
    while (r_it->Valid()) {
        // If we still have elements in the lhs, compare keys
        // otherwise drain rhs of new elements;
        auto elem_state = l_it->Valid() ?
            tri_comp(l_it->key().ToString().compare(r_it->key().ToString())) :
            cmp_result::addition;

        switch (elem_state) {
        case cmp_result::exists:
            {
            // Keys are the same, push a difference if values are not.
            auto l_v = l_it->value().ToString();
            auto r_v = r_it->value().ToString();
            if (l_v != r_v) {
                diff.emplace_back(std::make_pair(new ObjMetaData(),
                                                 new ObjMetaData()));
                diff.back().first->deserializeFrom(l_v);
                diff.back().second->deserializeFrom(r_v);
            }
            l_it->Next();
            r_it->Next();
            break;
            }
        case cmp_result::addition:
            {
            // New key in the rhs, append to the metadata list
            diff.emplace_back(std::make_pair(nullptr, new ObjMetaData()));
            diff.back().second->deserializeFrom(r_it->value().ToString());
            r_it->Next();
            break;
            }
        case cmp_result::removal:
            {
            // Key no longer exists in rhs! Since GC and tiering are disabled
            // during snapshot delta calculation this shouldn't happen.
            LOGERROR << "Missing key in snap2: " << l_it->key().ToString();
            fds_assert(false);
            }
        default:
            l_it->Next();
            continue;
        }
    }
    if (l_it->Valid()) {
        LOGERROR << "Had remaining keys in snap1: ";
        fds_assert(false);
    }
    delete l_it;
    delete r_it;
}

}  // namespace metadata
}  // namespace fds
