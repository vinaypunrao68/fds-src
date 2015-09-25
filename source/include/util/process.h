/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_UTIL_PROCESS_H_
#define SOURCE_INCLUDE_UTIL_PROCESS_H_

#include <util/Log.h>
namespace fds {
namespace util {

void printBackTrace();
void print_stacktrace(unsigned int max_frames = 63);

/**
 * Helper class to run a shell command & read its output 
 * or send input to it.
 */
class SubProcess {
    FILE* fp = NULL;

  public:
    bool run(const std::string& cmd, bool readMode=true);
    bool write(const std::string& cmd);
    bool read(std::string& data, int sz=1024);
    bool readLine(std::string& data);
    bool close();
    ~SubProcess();
};

} // namespace util
} // namespace fds

#endif // SOURCE_INCLUDE_UTIL_PROCESS_H_
