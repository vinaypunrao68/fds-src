/*
 *  Copyright 2016 Formation Data Systems, Inc.
 */
#include <VolumeChecker.h>
#include <VCSvcHandler.h>
#include <boost/algorithm/string.hpp>
#include <fds_volume.h>
#include <fdsp/vc_api_types.h>
#include "fdsp/common_constants.h"

namespace fds {

VolumeChecker::VolumeChecker(int argc, char **argv, bool initAsModule)
    : waitForShutdown(initAsModule),
      initCompleted(false),
      currentStatusCode(VC_NOT_STARTED),
      batchSize(100)
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

    g_fdslog->setSeverityFilter(fds_log::severity_level::debug);

    if (!populateVolumeList(argc, argv).OK()) {
        LOGERROR << "Unable to parse volume list. Not initializing";
        return;
    }

    TProcessorMap processors;
    processors.insert(std::make_pair<std::string, boost::shared_ptr<apache::thrift::TProcessor>>(
        fpi::commonConstants().PLATNET_SERVICE_NAME, svc_processor));

    /**
     * Init service process
     */
    SvcProcess::init(argc,
                     argv,
                     initAsModule,
                     "platform.conf",
                     "fds.checker.",
                     "vc.log",
                     nullptr,
                     svc_handler,
                     processors);

    if (!initAsModule) {
        LOGDEBUG << "Starting modules";
        start_modules();
    }

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
    int retCode = ERR_OK;

    if (!initCompleted) {
        return ERR_INVALID;
    } else {
        currentStatusCode = VC_RUNNING;
    }

    LOGNORMAL << "Running volume checker";

    /**
     * OM schedules the setupNewNode() task for this service registration
     * to kick in after a 3 second delay. We should *really* remove that.
     * So... in the case where setupNewNode() hasn't kicked in yet, if
     * we send the request out to DMs, and the DMs
     * finish their request and tries to send the results back but haven't
     * yet heard the service map update from OM, then they will fail to send
     * the results back and VolumeChecker will just wait here forever.
     * I'm reluctant on wanting to implement a timeout mechanism, since we will
     * then need to implement a heart-beat mechanism, which I dont' want.
     * Let's just ensure that we start after svclayer has its chance to est
     * everything.
     */
    LOGNORMAL << "Sleeping 5 seconds for OM's setupNewNode() to kick in";
    sleep(5);

    // First pull the DMT and DLT from OM
    fds_assert(getDMT() != ERR_NOT_FOUND);
    getDLT();

    LOGNORMAL << "Running volume checker DM check (phase 1)";
    runPhase1();

    if (currentStatusCode == VC_ERROR) {
        LOGERROR << "Volume Checker experienced data inconsistency!";
        retCode = ERR_CHECKSUM_MISMATCH;
    }

    readyWaiter.done();
    if (waitForShutdown) {
        shutdownGate_.waitUntilOpened();
    }
    LOGNORMAL << "Shutting down volume checker";
    return retCode;
}

VolumeChecker::VcStatus
VolumeChecker::getStatus() {
    return (currentStatusCode);
}

void
VolumeChecker::runPhase1() {
    currentStatusCode = VC_DM_HASHING;

    initialMsgTracker = new MigrationTrackIOReqs;
    prepareDmCheckerMap();
    sendVolChkMsgsToDMs();
    if (!waitForVolChkMsg().OK()) {
        handleVolumeCheckerError();
    }
    if (!checkDMHashQuorum().OK()) {
        handleVolumeCheckerError();
    } else {
        currentStatusCode = VC_DM_DONE;
        LOGNORMAL << "DM checks all succeeded";
    }
    delete initialMsgTracker;
}

void
VolumeChecker::prepareDmCheckerMap() {
    /**
     * For each volume that needs to be checked:
     * 1. From the committed DMT, get the DMs responsible for this volume.
     * 2. Create a vector called dataPerVol, each one will contain a DmCheckerMetaData
     *    obj, representing the vol->DM relationship and used to carry out DM
     *    operations.
     * 3. in vgCheckerList, we add the pair of vol -> list of DM metadatas
     */
    for (auto &vol : volumeList) {
        auto svcUuidVector = dmtMgr->getDMT(DMT_COMMITTED)->getSvcUuids(vol);
        std::vector<DmCheckerMetaData> dataPerVol;
        for (auto oneSvcUuid : svcUuidVector) {
            LOGDEBUG << "VolumeChecker creating metadata for volume "
                    << vol << " for node: " << oneSvcUuid;
            dataPerVol.emplace_back(vol, oneSvcUuid, batchSize);
        }
        LOGDEBUG << "VolumeChecker created all nodes metadata for volume: " << vol;
        vgCheckerList.emplace_back(std::make_pair(vol, dataPerVol));
    }
}

void
VolumeChecker::sendVolChkMsgsToDMs() {
    // For each volume that needs checking, send a msg to the DM responsible
    for (auto &dmChecker : vgCheckerList) {
        for (auto &oneDMtoCheck : dmChecker.second) {
            DmCheckerMetaData *dmData = &oneDMtoCheck;
            MigrationTrackIOReqs *msgTracker = initialMsgTracker;
            initialMsgTracker->startTrackIOReqs();
            LOGDEBUG << "Sending initial check msg to node : "
                    << dmData->svcUuid.svc_uuid << " for volume: "
                    << dmData->volId;
            dmData->sendVolChkMsg([dmData, msgTracker](EPSvcRequest *,
                                              const Error &e_,
                                              StringPtr payload) {
                if (!e_.OK()) {
                    dmData->status = DmCheckerMetaData::NS_ERROR;
                } else {
                    Error err(ERR_OK);
                    dmData->status = DmCheckerMetaData::NS_FINISHED;
                    fpi::CheckVolumeMetaDataRspMsgPtr respMsg =
                            fds::deserializeFdspMsg<fpi::CheckVolumeMetaDataRspMsg>(err, payload);
                    dmData->hashResult = respMsg->hash_result;
                }
                fds_assert(msgTracker);
                msgTracker->finishTrackIOReqs();
            });
        }
    }
}

Error
VolumeChecker::waitForVolChkMsg() {
    Error err(ERR_OK);

    fds_assert(initialMsgTracker);
    initialMsgTracker->waitForTrackIOReqs();

    for (auto &dmChecker : vgCheckerList) {
        for (auto &oneDMtoCheck : dmChecker.second) {
            if (oneDMtoCheck.status == DmCheckerMetaData::NS_ERROR) {
                err = ERR_INVALID;
                break;
            } else {
                // Record this in hashQuorumCheckMap
                ++hashQuorumCheckMap.left[oneDMtoCheck.hashResult];
                LOGNOTIFY << "Received checker status " <<  oneDMtoCheck.status
                          << " from  " << oneDMtoCheck.svcUuid << " with hash result: "
                          << oneDMtoCheck.hashResult << " "
                          << hashQuorumCheckMap.left[oneDMtoCheck.hashResult] << " times";
            }
        }
    }
    return err;
}

void
VolumeChecker::handleVolumeCheckerError() {
    // All we have to do now is to set the error code. More to come?
    currentStatusCode = VC_ERROR;
}

void
VolumeChecker::DmCheckerMetaData::sendVolChkMsg(const EPSvcRequestRespCb &cb) {
    auto msg = fpi::CheckVolumeMetaDataMsgPtr(new fpi::CheckVolumeMetaDataMsg);
    msg->volume_id = volId.v;
    msg->batch_size = batchSize;

    auto requestMgr = MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr();
    auto req = requestMgr->newEPSvcRequest(svcUuid);
    req->setPayload(FDSP_MSG_TYPEID(fpi::CheckVolumeMetaDataMsg), msg);
    req->setTimeoutMs(time_out);
    auto sysVol = FdsSysTaskQueueId;
    req->setTaskExecutorId(sysVol.v);
    if (cb) {
        req->onResponseCb(cb);
    }
    req->invoke();
    /**
     * Note: For receiving end, start with DMSvcHandler.cpp and follow
     * DmIoVolumeCheck
     */
    status = NS_CONTACTED;
}

Error
VolumeChecker::checkDMHashQuorum() {
    Error err(ERR_OK);
    bool noQuorum(false);
    fds_assert(hashQuorumCheckMap.size() > 0);

    if (hashQuorumCheckMap.size() > 1) {
        // Sort it in reverse order
        hashQuorumCheckMap.right.sort(std::greater<unsigned>());
        // The 0th element of the map should be the one with the most count
        auto firstIt = hashQuorumCheckMap.right.begin();
        if (firstIt->first == 1) {
            LOGERROR << "No quorum found";
            noQuorum = true;
        }

        // All the DMs did not return the same hash
        err = ERR_INVALID;
        for (auto dmChecker : vgCheckerList) {
            for (auto oneDMtoCheck : dmChecker.second) {
                if (noQuorum) {
                    oneDMtoCheck.status = DmCheckerMetaData::chkNodeStatus::NS_ERROR;
                } else if (oneDMtoCheck.hashResult != firstIt->second) {
                    LOGERROR << "Node: " << oneDMtoCheck.svcUuid << " is out of sync";
                    oneDMtoCheck.status = DmCheckerMetaData::chkNodeStatus::NS_OUT_OF_SYNC;
                }
            }
        }
    }
    return err;
}


/** Unit test implementations */
size_t
VolumeChecker::testGetVgCheckerListSize() {
    return vgCheckerList.size();
}

size_t
VolumeChecker::testGetVgCheckerListSize(unsigned index) {
    return vgCheckerList[index].second.size();
}

bool
VolumeChecker::testVerifyCheckerListStatus(unsigned castCode) {
    bool ret = true;
    for (auto dmChecker : vgCheckerList) {
        for (auto oneDMtoCheck : dmChecker.second) {
            if (oneDMtoCheck.status != static_cast<DmCheckerMetaData::chkNodeStatus>(castCode)) {
                ret = false;
            }
        }
    }
    return ret;
}
} // namespace fds
