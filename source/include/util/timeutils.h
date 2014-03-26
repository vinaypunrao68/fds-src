/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_UTIL_TIMEUTILS_H_
#define SOURCE_UTIL_TIMEUTILS_H_

#include <boost/date_time/posix_time/posix_time.hpp>
#include <assert.h>
#include "fds_types.h"

namespace fds {
    using TimeStamp = fds_int64_t;
    namespace util {

        extern const boost::posix_time::ptime epoch;
        TimeStamp getTimeStampMillis() ;

    } // namespace util
} // namespace fds

#endif // SOURCE_UTIL_TIMEUTILS_H_
