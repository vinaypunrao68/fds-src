/*
 * Copyright 2016 Formation Data Systems, Inc.
 */

#ifndef SOURCE_UTIL_VOLUMECHECKER_INCLUDE_VOLUMECHECKER_H
#define SOURCE_UTIL_VOLUMECHECKER_INCLUDE_VOLUMECHECKER_H

#include <boost/shared_ptr.hpp>
#include <fdsp/svc_api_types.h>
#include <net/SvcMgr.h>
#include <net/SvcProcess.h>
#include <net/PlatNetSvcHandler.h>
#include <net/SvcRequestPool.h>
#include <net/SvcProcess.h>
#include <fdsp_utils.h>

namespace fds {

class VolumeChecker : public SvcProcess {
public:
    explicit VolumeChecker(int argc, char **argv, bool initAsModule);
    ~VolumeChecker() = default;
    void init(int argc, char **argv, bool initAsModule);
    int run() override;

private:
};

} // namespace fds

#endif // SOURCE_UTIL_VOLUMECHECKER_INCLUDE_VOLUMECHECKER_H
