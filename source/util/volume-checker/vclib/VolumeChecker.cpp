/*
 *  Copyright 2016 Formation Data Systems, Inc.
 */
#include <VolumeChecker.h>
#include <VCSvcHandler.h>
#include <fdsp/VCSvc.h>


namespace fds {

VolumeChecker::VolumeChecker(int argc, char **argv, bool initAsModule)
{
    vcVec[0] = nullptr;
    vcVec[1] = nullptr;

    /**
     * Initialize VC service
     */
    auto svc_handler = boost::make_shared<VCSvcHandler>(this);
    auto svc_processor = boost::make_shared<fpi::VCSvcProcessor>(svc_handler);

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
#if 0
int main(int argc, char **argv) {
    fds::VolumeChecker checker(argc, argv, false);
    checker.run();
    return checker.main();
}
#endif
