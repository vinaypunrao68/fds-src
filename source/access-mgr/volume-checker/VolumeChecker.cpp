/*
 *  Copyright 2016 Formation Data Systems, Inc.
 */
#include <VolumeChecker.h>
#include "AMSvcHandler.h"
#include <AmProcessor.h>
#include <AMSvcHandler.h>

namespace fds {

VolumeChecker::VolumeChecker(int argc, char *argv[], bool initAsModule)
{
    am.reset(new AccessMgr("VolumeChecker Module", this));
    vcVec[0] = am.get();
    vcVec[1] = nullptr;

    /**
     * Initialize VC service
     */
    auto svc_handler = boost::make_shared<AMSvcHandler>(this, am->getProcessor());
    auto svc_processor = boost::make_shared<fpi::AMSvcProcessor>(svc_handler);

    /**
     * Init service process
     */
    init(argc,
         argv,
         "platform.conf",
         "fds.vc",
         "vc.log",
         vcVec,
         svc_handler,
         svc_processor);
}

} // namespace fds

int main(int argc, char **argv) {
    fds::VolumeChecker checker(argc, argv, false);
    return checker.main();
}
