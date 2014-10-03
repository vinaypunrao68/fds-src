/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_UTIL_TYPE_H_
#define SOURCE_INCLUDE_UTIL_TYPE_H_

#include <typeinfo>
#include <string>
// http://stackoverflow.com/a/4541470/2643320
namespace fds { namespace util {

std::string demangle(const char* name);

template <class T>
std::string type(const T& t) {
    return demangle(typeid(t).name());
}

}  // namespace util
}  // namespace fds
#endif  // SOURCE_INCLUDE_UTIL_TYPE_H_
