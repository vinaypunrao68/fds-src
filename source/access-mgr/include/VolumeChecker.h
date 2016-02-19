/*
 * Copyright 2016 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_VOLUMECHECKER_H_
#define SOURCE_ACCESS_MGR_INCLUDE_VOLUMECHECKER_H_

#include <fds_process.h>
#include <fdsp/PlatNetSvc.h>
#include <net/SvcMgr.h>
#include <net/SvcProcess.h>
#include <net/PlatNetSvcHandler.h>
#include <AccessMgr.h>

namespace fds {

class VolumeChecker : public SvcProcess {
public:
    VolumeChecker(int argc, char *argv[], bool initAsModule);
    ~VolumeChecker() = default;
    int run() override {
        LOGNORMAL << "Running volume checker";
        return 0;
    }

private:
    std::unique_ptr<AccessMgr> am;

    fds::Module *vcVec[2];
};

} // namespace fds

#endif // SOURCE_ACCESS_MGR_INCLUDE_VOLUMECHECKER_H_
