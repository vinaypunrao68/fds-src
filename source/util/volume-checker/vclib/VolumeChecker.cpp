/*
 *  Copyright 2016 Formation Data Systems, Inc.
 */
#include <VolumeChecker.h>
#include <VCSvcHandler.h>
#include <fdsp/VCSvc.h>


namespace fds {

VolumeChecker::VolumeChecker(int argc, char **argv, bool initAsModule)
    : waitForShutdown(initAsModule)
{
    init(argc, argv, initAsModule);
    LOGNORMAL << "VolumeChecker initialized.";
}

void
VolumeChecker::init(int argc, char **argv, bool initAsModule)
{
    /**
     * Initialize VC service
     */
    auto svc_handler = boost::make_shared<VCSvcHandler>(this);
    auto svc_processor = boost::make_shared<fpi::VCSvcProcessor>(svc_handler);

    /**
     * Init service process
     */
    SvcProcess::init(argc,
                     argv,
                     initAsModule,
                     "platform.conf",
                     "fds.vc",
                     "vc.log",
                     nullptr,
                     svc_handler,
                     svc_processor);

    dmtMgr = MODULEPROVIDER()->getSvcMgr()->getDmtManager();
    dltMgr = MODULEPROVIDER()->getSvcMgr()->getDltManager();

}

Error
VolumeChecker::getDMT()
{
    return (MODULEPROVIDER()->getSvcMgr()->getDMT(maxRetries));
}

Error
VolumeChecker::getDLT()
{
    return (MODULEPROVIDER()->getSvcMgr()->getDLT(maxRetries));
}

int
VolumeChecker::run() {
    LOGNORMAL << "Running volume checker";

    // First pull the DMT and DLT from OM
    getDMT();
    getDLT();

    readyWaiter.done();
    if (waitForShutdown) {
        shutdownGate_.waitUntilOpened();
    }
    LOGNORMAL << "Shutting down volume checker";
    return 0;
}

} // namespace fds
