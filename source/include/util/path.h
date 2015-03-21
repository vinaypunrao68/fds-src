/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_UTIL_PATH_H_
#define SOURCE_INCLUDE_UTIL_PATH_H_
#include <string>

namespace fds {
namespace util {
bool dirExists(const std::string& dirname);
bool fileExists(const std::string& filename);
}  // namespace util
}  // namespace fds

#endif  // SOURCE_INCLUDE_UTIL_PATH_H_
