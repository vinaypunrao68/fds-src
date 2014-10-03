/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <string>
#include <cstdlib>
#include <memory>
#include <cxxabi.h>
#include <util/type.h>
namespace fds { namespace util {
#ifdef __GNUG__
std::string demangle(const char* name) {
    int status = -4;  // some arbitrary value to eliminate the compiler warning

    // enable c++11 by passing the flag -std=c++11 to g++
    std::unique_ptr<char, void(*)(void*)> res { //NOLINT
        abi::__cxa_demangle(name, NULL, NULL, &status),
                std::free
                };

    return (status == 0) ? res.get() : name;
}

#else

// does nothing if not g++
std::string demangle(const char* name) {
    return name;
}

#endif

}  // namespace util
}  // namespace fds
