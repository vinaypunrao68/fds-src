/*
 *  Copyright 2016 Formation Data Systems, Inc.
 */

#ifndef SOURCE_UTIL_VOLUME_CHECKER_INCLUDE_VCSVCHANDLER_H_
#define SOURCE_UTIL_VOLUME_CHECKER_INCLUDE_VCSVCHANDLER_H_

#include <fdsp/PlatNetSvc.h>
#include <net/PlatNetSvcHandler.h>
#include <fdsp/VCSvc.h>

#include <VolumeChecker.h>

namespace fds {

class VCSvcHandler : virtual public fpi::VCSvcIf, public PlatNetSvcHandler {
public:
    explicit VCSvcHandler(VolumeChecker *vc);
    ~VCSvcHandler() = default;

};

} // namespace fds
#endif // SOURCE_UTIL_VOLUME_CHECKER_INCLUDE_VCSVCHANDLER_H_
