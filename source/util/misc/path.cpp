/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <util/path.h>
#include <util/stringutils.h>
#include <util/process.h>

#include <hash/md5.h>

#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>

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
            name = ep->d_name;
            bool fIsDir = ep->d_type == DT_DIR;
            if (ep->d_type == DT_UNKNOWN) {
                fIsDir = dirExists(dirname + "/" + name);
            }
            if (fIsDir) {
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
            bool fIsFile = ep->d_type == DT_REG;
            if (ep->d_type == DT_UNKNOWN) {
                fIsFile = fileExists(dirname + "/" + std::string(ep->d_name));
            }

            if (fIsFile) {
                vecDirs.push_back(ep->d_name);
            }
        }
        (void) closedir (dp);
    }
}

std::string getFileChecksum(const std::string& filename) {
    std::ifstream is (filename.c_str(), std::ifstream::binary);
    size_t length = 1024*1024;
    char * buffer = new char [length];
    checksum_calc sum;
    while(!is.eof() && is.good()) {
        is.read (buffer,length);
        sum.checksum_update((unsigned  char *)buffer, is.gcount());
    }

    is.close();
    std::string strsum;
    sum.get_checksum(strsum);
    return strsum;
}

/**
 * Return bytes from human file size
 * input string as: case-insensitive
 * "12345",""12345b", "12345bytes" = 12345
 * "1g", "1 GB" = 1024*1024*1024 bytes
 * "60k", "60KB" = 60*1024
 */
fds_uint64_t getBytesFromHumanSize(const std::string& strFileSize) {
    fds_uint64_t mult = 1;
    fds_uint64_t bytes = 0;
    char *p, *end;
    bytes =  std::strtol(strFileSize.c_str(), &end, 10);
    for ( ; end && *end && *end==' '; ++end);
    p = end;
    for ( ; *p; ++p) *p = tolower(*p);
    if (end != strFileSize.c_str()) {
        if (0 == strncmp(end, "b", 1)) mult = 1;
        else if (0 == strncmp(end, "k", 1)) mult = 1024;
        else if (0 == strncmp(end, "m", 1)) mult = 1024*1024;
        else if (0 == strncmp(end, "g", 1)) mult = 1024*1024*1024;
        else if (0 == strncmp(end, "t", 1)) mult = 1024L*1024L*1024L*1024L;
    }
    return bytes * mult;
}

// get the PATH env variable
void getPATH(std::vector<std::string>& paths) {
    char *path = getenv("PATH");
    tokenize(path, paths, ':');
}

// add the given path to the PATH env variable
bool addPATH(const std::string& path) {
    std::vector<std::string> paths;
    getPATH(paths);
    bool fExists = false;
    for(const auto& item : paths) {
        if (item == path) {
            fExists = true;
            break;
        }
    }

    if (!fExists) {
        paths.push_back(path);
        setenv("PATH", join(paths,':').c_str(), true);
        return true;
    }
    return false;
}

// remove the given path from the PATH env variable
bool removePATH(const std::string& path) {
    std::vector<std::string> paths;
    getPATH(paths);
    bool fExists = false;
    auto pos = 0;
    for(const auto& item : paths) {
        if (item == path) {
            fExists = true;
            break;
        } else {
            pos ++ ;
        }
    }

    if (fExists) {
        paths.erase(paths.begin()+pos);
        setenv("PATH", join(paths,':').c_str(), true);
        return true;
    }
    return false;
}

void populateRootDirectories(std::vector<std::string> &roots, int numOfNodes)
{
    std::string homedir = boost::filesystem::path(getenv("HOME")).string();
    std::string baseDir =  homedir + "/temp";

    for (int i = 1; i <= numOfNodes; i++) {
        roots.push_back(util::strformat("--fds-root=%s/node%d", baseDir.c_str(), i));
    }
}


// get the binary location of the given command
std::string which(const std::string& path) {
    SubProcess process;
    std::string output;
    process.run(std::string("which ") + path);
    process.readLine(output);
    process.close();
    return output;
}
}  // namespace util
}  // namespace fds
