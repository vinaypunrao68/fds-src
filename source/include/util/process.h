/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_UTIL_PROCESS_H_
#define SOURCE_INCLUDE_UTIL_PROCESS_H_

#include <util/Log.h>
namespace fds {
namespace util {

void printBackTrace();
void print_stacktrace(unsigned int max_frames = 63);

} // namespace util
} // namespace fds

#endif // SOURCE_INCLUDE_UTIL_PROCESS_H_
