///
/// @copyright 2015 Formation Data Systems, Inc.
///

// Class include.
#include "CatalogKeyComparator.h"

namespace fds {

template <class T> int CatalogKeyComparator::_compareWithOperators (T const& lhs, T const& rhs)
{
    if (lhs < rhs)
    {
        return -1;
    }
    else if (lhs > rhs)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

}  // namespace fds
