/*
 *  Copyright 2016 Formation Data Systems, Inc.
 */
#include <VolumeChecker.h>
#include <VCSvcHandler.h>
#include <fdsp/VCSvc.h>
#include <boost/algorithm/string.hpp>


namespace fds {

VolumeChecker::VolumeChecker(int argc, char **argv, bool initAsModule)
    : waitForShutdown(initAsModule),
      initCompleted(false),
      currentStatusCode(VC_NOT_STARTED)
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
    auto svc_processor = boost::make_shared<fpi::PlatNetSvcProcessor>(svc_handler);

    if (!populateVolumeList(argc, argv).OK()) {
        LOGERROR << "Unable to parse volume list. Not initializing";
        return;
    }

    /**
     * Init service process
     */
    SvcProcess::init(argc,
                     argv,
                     initAsModule,
                     "platform.conf",
                     "fds.checker",
                     "vc.log",
                     nullptr,
                     svc_handler,
                     svc_processor);

    dmtMgr = MODULEPROVIDER()->getSvcMgr()->getDmtManager();
    dltMgr = MODULEPROVIDER()->getSvcMgr()->getDltManager();

    initCompleted = true;
}

Error
VolumeChecker::populateVolumeList(int argc, char **argv)
{
    Error err(ERR_INVALID);
    // First find the "-v=1,2,3" arg
    for (int i = 0; i < argc; ++i) {
        std::string anArg(argv[i]);
        if (anArg.substr(0,2) == "-v") {
            // volCSV == "1,2,3"
            auto volCSV = anArg.substr(3, std::string::npos);

            // Boost split allows string containers
            std::vector<std::string> bTokens;
            boost::split(bTokens, volCSV, boost::is_any_of(","));

            for (auto volIdString : bTokens) {
                volumeList.emplace_back(fds_volid_t(std::stoi(volIdString)));
            }

            err = ERR_OK;
            break;
        }
    }
    return err;
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
    if (!initCompleted) {
        return ERR_INVALID;
    } else {
        currentStatusCode = VC_RUNNING;
    }

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

VolumeChecker::vcStatus
VolumeChecker::getStatus() {
    return (currentStatusCode);
}

} // namespace fds
