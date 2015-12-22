/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <DmMigrationDest.h>

namespace fds {

Error
DmMigrationDest::start()
{
    Error err(ERR_OK);
    LOGMIGRATE << "migrationid: " << migrationId
            << " starting Destination blob diff for volume: " << volumeUuid;

    // true - volumeGroupMode
    processInitialBlobFilterSet(true);

    return err;
}

void
DmMigrationDest::staticMigrationStatusToSrc(NodeUuid srcNodeUuid,
                                            fds_volid_t volumeId,
                                            const Error &result,
                                            migrationCb cbToCoordinator)
{
    fpi::CtrlNotifyPeerSrcDmStaticDonePtr finMsg(new fpi::CtrlNotifyPeerSrcDmStaticDone);
    finMsg->result = static_cast<int64_t>(result.GetErrno());
    finMsg->volume_id = volumeId.get();

    auto msgToSrc = gSvcRequestPool->newEPSvcRequest(srcNodeUuid.toSvcUuid());
    // msgToSrc->setTimeoutMs(dataMgr.dmMigrationMgr->getTimeoutValue());
    msgToSrc->setPayload(FDSP_MSG_TYPEID(fpi::CtrlNotifyPeerSrcDmStaticDone),
                                   finMsg);

    // Do we need a cb check?
    // msgToSrc->onResponseCb(RESPONSE_MSG_HANDLER(DmMigrationBase::dmMigrationCheckResp, abortBind, passBind));
    msgToSrc->invoke();

    // Return result to coordinator
    fds_assert(cbToCoordinator != NULL);
    cbToCoordinator(result);
}

} // namespace fds
