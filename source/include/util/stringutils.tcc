///
/// @copyright 2015 Formation Data Systems, Inc.
///

// Standard includes.
#include <sstream>

// Class include.
#include "stringutils.h"

using std::ostringstream;
using std::string;

namespace fds {
namespace util {

template <class T, class CollT> string join (CollT const& collection, string const& delimiter)
{
    ostringstream oss;

    bool first = true;
    for (T const& element : collection)
    {
        if (first)
        {
            first = false;
        }
        else
        {
            oss << delimiter;
        }
        oss << element;
    }

    return oss.str();
}

}  // namespace util
}  // namespace fds
