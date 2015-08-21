///
/// @copyright 2015 Formation Data Systems, Inc.
///

#ifndef SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_CATALOGKEYCOMPARATOR_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_CATALOGKEYCOMPARATOR_H_

// Standard includes.
#include <string>

// Internal includes.
#include "leveldb/comparator.h"

namespace fds {

class CatalogKeyComparator : public leveldb::Comparator
{
public:

    int Compare (leveldb::Slice const& lhs, leveldb::Slice const& rhs) const override;

    void FindShortSuccessor (std::string* key) const override;

    void FindShortestSeparator (std::string* start, leveldb::Slice const& limit) const override;

    char const* Name () const override;

private:

    template <class T> static int _compareWithOperators (T const& lhs, T const& rhs);

};

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_CATALOGKEYCOMPARATOR_H_
