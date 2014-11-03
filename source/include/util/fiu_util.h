/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_UTIL_FIU_UTIL_H_
#define SOURCE_INCLUDE_UTIL_FIU_UTIL_H_

#include <string>
#include <fiu-local.h>
namespace fds { namespace util {

/**
* @brief Fault injection exception
*/
struct FiuException : std::exception {
    explicit FiuException(const std::string &e)
        : what_(e)
    {
    }
    virtual const char* what() const noexcept(true) {
        return what_.c_str();
    }
 protected:
    std::string what_;
};

}  // namespace util
}  // namespace fds
#endif  // SOURCE_INCLUDE_UTIL_FIU_UTIL_H_
