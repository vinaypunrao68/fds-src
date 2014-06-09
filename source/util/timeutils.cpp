/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <util/timeutils.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <assert.h>
#include "fds_types.h"

namespace fds { namespace util {

const boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));

fds_int64_t getTimeStampMillis() {
    boost::posix_time::time_duration diff = boost::posix_time::microsec_clock::universal_time()  - epoch;
    return diff.total_milliseconds();
}

} // namespace util
} // namespace fds

