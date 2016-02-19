/*
 * Copyright 2016 Formation Data Systems, Inc.
 */

#ifndef SOURCE_UTIL_VOLUMECHECKER_INCLUDE_VOLUMECHECKER_H
#define SOURCE_UTIL_VOLUMECHECKER_INCLUDE_VOLUMECHECKER_H

#include <fds_process.h>
#include <net/SvcMgr.h>
#include <net/SvcProcess.h>

namespace fds {

class VolumeChecker : public SvcProcess {
public:
    explicit VolumeChecker(int argc, char **argv, bool initAsModule);
    ~VolumeChecker() = default;
    int run() override {
        LOGNORMAL << "Running volume checker";
        return 0;
    }

private:
//    std::unique_ptr<AccessMgr> am;

    fds::Module *vcVec[2];
};

} // namespace fds

#endif // SOURCE_UTIL_VOLUMECHECKER_INCLUDE_VOLUMECHECKER_H
