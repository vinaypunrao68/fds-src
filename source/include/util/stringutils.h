/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_UTIL_STRINGUTILS_H_
#define SOURCE_INCLUDE_UTIL_STRINGUTILS_H_
#include <string>
namespace fds {
namespace util {

std::string strformat(const std::string fmt_str, ...);
std::string strlower(const std::string& str);

} // namespace util
} // namespace fds

#endif // SOURCE_INCLUDE_UTIL_STRINGUTILS_H_
