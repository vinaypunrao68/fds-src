/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <util/path.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>
namespace fds {
namespace util {
bool dirExists(const std::string& dirname) {
    struct stat s;
    int err = stat(dirname.c_str(), &s);
    return (0 == err && (S_ISDIR(s.st_mode)));
}

bool fileExists(const std::string& filename) {
    struct stat s;
    int err = stat(filename.c_str(), &s);
    return (0 == err && (S_ISREG(s.st_mode)));
}

}  // namespace util
}  // namespace fds
