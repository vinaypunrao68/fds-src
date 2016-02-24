/*
 *  Copyright 2016 Formation Data Systems, Inc.
 */
#include <VolumeChecker.h>
#include <VCSvcHandler.h>
#include <boost/algorithm/string.hpp>
#include <fds_volume.h>
#include <fdsp/vc_api_types.h>


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
    fds_assert(getDMT() != ERR_NOT_FOUND);
    getDLT();

    LOGNORMAL << "Running volume checker DM check (phase 1)";
    runPhase1();

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

Error
VolumeChecker::runPhase1() {
    Error err(ERR_OK);
    currentStatusCode = VC_PHASE_1;

    prepareDmCheckerMap();
    sendVolChkMsgsToDMs();

    return err;
}

void
VolumeChecker::prepareDmCheckerMap() {
    for (auto &vol : volumeList) {
        auto svcUuidVector = dmtMgr->getDMT(DMT_COMMITTED)->getSvcUuids(vol);
        std::vector<dmCheckerMetaData> dataPerVol;
        for (auto oneSvcUuid : svcUuidVector) {
            dataPerVol.emplace_back(vol, oneSvcUuid);
        }
        dmCheckerList.emplace_back(std::make_pair(vol, dataPerVol));
    }
}

Error
VolumeChecker::sendVolChkMsgsToDMs() {
    Error err(ERR_OK);
    for (auto &dmChecker : dmCheckerList) {
        for (auto &oneDMtoCheck : dmChecker.second) {
            oneDMtoCheck.sendVolChkMsg([this](EPSvcRequest *,
                                              const Error &e_,
                                              StringPtr payload) {
                // todo
                });
            }
    }

    return err;
}

void
VolumeChecker::dmCheckerMetaData::sendVolChkMsg(const EPSvcRequestRespCb &cb) {
    auto msg = fpi::CheckVolumeMetaDataMsgPtr(new fpi::CheckVolumeMetaDataMsg);
    msg->volumeId = volId.v;

    auto requestMgr = MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr();
    auto req = requestMgr->newEPSvcRequest(nodeUuid);
    req->setPayload(FDSP_MSG_TYPEID(fpi::CheckVolumeMetaDataMsg), msg);
    auto sysVol = FdsSysTaskQueueId;
    req->setTaskExecutorId(sysVol.v);
    if (cb) {
        req->onResponseCb(cb);
    }
    req->invoke();
    status = NS_CONTACTED;
}

} // namespace fds
