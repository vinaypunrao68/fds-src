/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_UTIL_PATH_H_
#define SOURCE_INCLUDE_UTIL_PATH_H_
#include <string>
#include <vector>

namespace fds {
namespace util {

//checks if the given path is a directory
bool dirExists(const std::string& dirname);

// checks if the given path is a file
bool fileExists(const std::string& filename);

// gets the list of directories in the given dir
void getSubDirectories(const std::string& dirname, std::vector<std::string>& vecDirs);

// gets the list fo files in the given directory
void getFiles(const std::string& dirname, std::vector<std::string>& vecDirs);

std::string getFileChecksum(const std::string& filename);
}  // namespace util
}  // namespace fds

#endif  // SOURCE_INCLUDE_UTIL_PATH_H_
