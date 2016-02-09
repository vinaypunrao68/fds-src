/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <util/path.h>
#include "boost/program_options.hpp"

namespace po = boost::program_options;

namespace {

const std::string READ_MODE("read");
const std::string WRITE_MODE("write");
const uint32_t MIN_JOBS = 1;
const uint32_t MAX_JOBS = 100;

std::string path("/tmp");
std::string mode("write");
uint64_t filesize = 1024 * 1024 * 1024;
uint32_t objsize = 131072;
uint64_t startnum = 0;
uint64_t count = 1024;
uint32_t jobs = MIN_JOBS;
bool syncMode = false;
bool deviceMode = false;

po::options_description visible("fdsio data generation tool");
po::options_description hidden("internal arguments");
po::options_description desc("allowed options");

int rc = 1;

void * buffer = NULL;
uint32_t len = 0;

uint64_t objcount = 0;

void do_read(int fd, uint64_t & num) {
    for (uint64_t c = 0; c < objcount; ++c) {
        memset(buffer, 0, objsize);
        ssize_t b = deviceMode ? pread(fd, buffer, objsize, num * sizeof(uint64_t)) :
                read(fd, buffer, objsize);
        if (b < 0) {
            std::cerr << "failed to read from file, error='" << errno
                      << "', num='" << num << "'" << std::endl;
            return;
        }

        for (uint32_t i = 0; i < len; ++i) {
            if (num++ != static_cast<uint64_t *>(buffer)[i]) {
                std::cerr << "Consistency check failed!" << std::endl;
                return;
            }
        }
    }
    rc = 0;
}

void do_write(int fd, uint64_t & num) {
    for (uint64_t c = 0; c < objcount; ++c) {
        memset(buffer, 0, objsize);
        uint64_t tnum = num;
        for (uint32_t i = 0; i < len; ++i) {
            static_cast<uint64_t *>(buffer)[i] = num++;
        }

        ssize_t b = deviceMode ? pwrite(fd, buffer, objsize, tnum * sizeof(uint64_t)) :
                write(fd, buffer, objsize);
        if (b < 0) {
            std::cerr << "failed to write to file, error: '" << errno
                      << "', num='" << tnum << "'" << std::endl;
            return;
        }
    }
    rc = 0;
}

}  // namespace

void usage() {
    std::cout << std::endl << visible << std::endl;
    exit(rc);
}

void validateArgs(const po::variables_map & vmap) {
    if (vmap.end() != vmap.find("help")) {
        usage();
    } else if (mode != READ_MODE && mode != WRITE_MODE) {
        std::cerr << "unrecognized mode! allowed values are 'read' and 'write'." << std::endl;
        usage();
    } else if (!filesize || filesize % objsize) {
        std::cerr << "invalid filesize! filesize cannot be 0 and should be multiples of objsize."
                  << std::endl;
        usage();
    } else if (!count) {
        std::cerr << "invalid count! count cannot be 0." << std::endl;
        usage();
    } else if (jobs < MIN_JOBS || jobs > MAX_JOBS) {
        std::cerr << "invalid number of jobs! allowed values for the jobs are " << MIN_JOBS
                  << " to " << MAX_JOBS << std::endl;
        usage();
    } else if (!objsize || objsize % sizeof(uint64_t)) {
        std::cerr << "invalid objsize! objsize should be multiple of "
                  << sizeof(uint64_t) << std::endl;
        usage();
    } else if (path.empty()) {
        std::cerr << "invalid path! can not be empty" << std::endl;
        usage();
    }

    syncMode = (vmap.end() != vmap.find("sync"));
}

int main(int argc, char * argv[]) {
    /*
     * Define and parse the program options
     */
    std::string strFileSize="1G";
    std::string strCount = "1";
    std::string strObjSize = "128K";
    bool fVerify = false;
    visible.add_options()
            ("help", "print help messages")
            ("path", po::value<std::string>(&path)->default_value(path), "directory or device path")
            ("verify,v", po::bool_switch(&fVerify), "verify previously created files")
            ("size,s", po::value<std::string>(&strFileSize)->default_value(strFileSize),
             "file size .eg : 1024, 1K, 2M, 3G")
            ("count", po::value<std::string>(&strCount)->default_value(strCount), "file count")
            ("jobs", po::value<uint32_t>(&jobs)->default_value(jobs), "# of concurrent jobs")
            ("objsize", po::value<std::string>(&strObjSize)->default_value(strObjSize), "object size - 1024,1K, 2M")
            ("sync", "synchronous I/O");

    hidden.add_options()
            ("startnum", po::value<uint64_t>(&startnum)->default_value(startnum),
             "startnum with number");

    desc.add(visible).add(hidden);

    // Parse common line options
    po::variables_map vmap;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vmap);
    } catch (std::exception & e) {
        std::cerr << e.what() << std::endl;
        usage();
    }
    if (fVerify) mode="read";
    po::notify(vmap);

    filesize = fds::util::getBytesFromHumanSize(strFileSize);
    count = fds::util::getBytesFromHumanSize(strCount);
    objsize = fds::util::getBytesFromHumanSize(strObjSize);

    // Validate command line options
    validateArgs(vmap);

    // allocate buffer and calculate len, objcount
    buffer = calloc(objsize, sizeof(char));
    if (!buffer) {
        std::cerr << "calloc() failed!" << std::endl;
        return rc;
    }
    len = objsize / sizeof(uint64_t);
    objcount = filesize / objsize;

    bool readOp = (mode == READ_MODE);  // read or write
    struct stat st = {0};
    if (!stat(path.c_str(), &st)) {
        deviceMode = (st.st_rdev != 0);
    } else {  // error
        if (readOp) {
            std::cerr << path << ": not found! error='" << errno
                      << "'" << std::endl;
            usage();
        }
    }
    std::string filePrefix(path + "/fdsdata.");

    int flags = readOp ? O_RDONLY : O_WRONLY | O_CREAT;
    if (syncMode) {
        flags |= O_SYNC;
    }

    int fd = 0;
    if (deviceMode) {
        fd = open(path.c_str(), flags, S_IRWXU | S_IRGRP);
        if (fd < 0) {
            std::cerr << path << ": unable to open file for I/O! error='"
                      << errno << "'" << std::endl;
            return rc;
        }
    }

    if (jobs > 1) {
        // prepare for spawning child
        uint32_t tjobs = jobs;
        jobs = 1;

        uint64_t tcount = count;
        count = tcount / tjobs;

        pid_t pid = 0;

        for (uint32_t i = 0; i < tjobs; ++i) {
            startnum += (i * count * filesize) / sizeof(uint64_t);

            if ((tjobs - 1) == i) {
                // last process, schedule remaining files
                count += tcount % tjobs;
            }

            pid = fork();
            if (pid < 0) {  // error
                std::cerr << "error while spawning jobs! error='" << errno << "'" << std::endl;
                return rc;
            } else if (0 == pid) {  // child
                break;
            }
        }

        // wait for children
        if (pid) {
            while (tjobs) {
                if (wait(&rc) < 0) {
                    std::cerr << "error in child process! error='" << errno
                              << "'" << std::endl;
                    break;
                }
                tjobs--;
            }
            return rc;
        }
    }

    uint64_t num = startnum;
    for (uint64_t i = 0; i < count; ++i) {
        if (!deviceMode) {
            std::ostringstream filename;
            filename << filePrefix << num;

            fd = open(filename.str().c_str(), flags, S_IRWXU | S_IRGRP);
            if (fd < 0) {
                std::cerr << filename << ": unable to open a file for I/O! error='"
                          << errno << "'" << std::endl;
                return rc;
            }
        }

        readOp ? do_read(fd, num) : do_write(fd, num);
        if (rc) {
            std::cerr << "I/O ERROR!" << std::endl;
            break;
        }

        if (!deviceMode) {
            close(fd);
        }
    }

    return rc;
}
