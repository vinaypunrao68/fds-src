/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <util/path.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

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

// gets the list of directories in the given dir
void getSubDirectories(const std::string& dirname, std::vector<std::string>& vecDirs) {
    DIR *dp;
    struct dirent *ep;
    dp = opendir (dirname.c_str());

    if (dp != NULL) {
        std::string name;
        while ((ep = readdir(dp))) {
            if (ep->d_type == DT_DIR) {
                name = ep->d_name;
                if (name != ".." && name != ".") {
                    vecDirs.push_back(name);
                }
            }
        }
        (void) closedir (dp);
    }
}

// gets the list fo files in the given directory
void getFiles(const std::string& dirname, std::vector<std::string>& vecDirs) {
    DIR *dp;
    struct dirent *ep;
    dp = opendir(dirname.c_str());

    if (dp != NULL) {
        std::string name;
        while ((ep = readdir (dp))) {
            if (ep->d_type == DT_REG) {
                vecDirs.push_back(ep->d_name);
            }
        }
        (void) closedir (dp);
    }
}
}  // namespace util
}  // namespace fds
