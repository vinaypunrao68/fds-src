
/* Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_TIMESTAMP_H_
#define SOURCE_INCLUDE_FDS_TIMESTAMP_H_

#include <boost/date_time/posix_time/posix_time.hpp>

namespace fds {
/**
 *
 * @return milliseconds from epoch
 */
inline uint64_t get_fds_timestamp_ms()
{
    using boost::gregorian::date;
    using boost::posix_time::ptime;
    using boost::posix_time::microsec_clock;

     static ptime const epoch(date(1970, 1, 1));
     return (microsec_clock::universal_time() - epoch).total_milliseconds();
}
}  // namespace fds
#endif  // SOURCE_INCLUDE_FDS_TIMESTAMP_H_
