/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_UTIL_STRINGUTILS_H_
#define SOURCE_INCLUDE_UTIL_STRINGUTILS_H_
#include <string>
namespace fds {
namespace util {

/**
 * printf like formatting for std::string
 */
std::string strformat(const std::string fmt_str, ...);

/**
 * lower case std::string
 */
std::string strlower(const std::string& str);

}  // namespace util
}  // namespace fds

#endif  // SOURCE_INCLUDE_UTIL_STRINGUTILS_H_
