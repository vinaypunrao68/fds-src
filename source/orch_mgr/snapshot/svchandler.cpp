
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>

#include <fds_error.h>
#include <fds_types.h>
#include <net/net-service.h>
#include <net/net-service-tmpl.hpp>
#include <net/SvcRequestPool.h>
#include <snapshot/svchandler.h>
#include <OmResources.h>
#include <orchMgr.h>

namespace fds { namespace snapshot {

OmSnapshotSvcHandler::OmSnapshotSvcHandler(OrchMgr* om) : om(om) {
}

Error OmSnapshotSvcHandler::omSnapshotCreate(const fpi::Snapshot& snapshot) {
    Error err(ERR_OK);
    fds_volid_t   volId = snapshot.volumeId;

    fpi::CreateSnapshotMsgPtr msg(new fpi::CreateSnapshotMsg());
    msg->snapshot = snapshot;
    auto cb = RESPONSE_MSG_HANDLER(OmSnapshotSvcHandler::omSnapshotCreateResp, msg);
    auto asyncReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DltObjectIdEpProvider>(om->getDMTNodesForVolume(volId)));
    asyncReq->setPayload(FDSP_MSG_TYPEID(fpi::CreateSnapshotMsg), msg);
    asyncReq->onResponseCb(cb);
    asyncReq->invoke();

    // LOGDEBUG << asyncReq->logString() << fds::logString(*stSnapCreateTxMsg);
    return err;
}

void OmSnapshotSvcHandler::omSnapshotCreateResp(fpi::CreateSnapshotMsgPtr request,
                                                QuorumSvcRequest* svcReq,
                                                const Error& error,
                                                boost::shared_ptr<std::string> payload) {
    // Return if err
    if (error != ERR_OK) {
        LOGWARN << "error in response: " << error;
        LOGWARN << "create snapshot for [" << request->snapshot.snapshotName << "] failed";
    }

    OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    VolumeContainer::pointer volContainer = local->om_vol_mgr();
    volContainer->addSnapshot(request->snapshot);
}


Error OmSnapshotSvcHandler::omSnapshotDelete(fds_uint64_t snapshotId, fds_uint64_t volId) {
    Error err(ERR_OK);

    fpi::DeleteSnapshotMsgPtr stSnapDeleteTxMsg(new fpi::DeleteSnapshotMsg());

    stSnapDeleteTxMsg->snapshotId = snapshotId;

    auto cb = RESPONSE_MSG_HANDLER(OmSnapshotSvcHandler::omSnapshotDeleteResp);
    auto asyncReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DltObjectIdEpProvider>(om->getDMTNodesForVolume(volId)));

    asyncReq->setPayload(FDSP_MSG_TYPEID(fpi::DeleteSnapshotMsg), stSnapDeleteTxMsg);
    asyncReq->onResponseCb(cb);
    asyncReq->invoke();

    // LOGDEBUG << asyncReq->logString() << fds::logString(*stSnapDeleteTxMsg);
    return err;
}

void OmSnapshotSvcHandler::omSnapshotDeleteResp(QuorumSvcRequest* svcReq,
                            const Error& error,
                            boost::shared_ptr<std::string> payload) {
}

Error OmSnapshotSvcHandler::omVolumeCloneCreate(const fpi::CreateVolumeCloneMsgPtr &msg) {
    Error err(ERR_OK);
    fds_verify(msg != NULL);
    fds_volid_t   volId = msg->volumeId;

    auto cb = RESPONSE_MSG_HANDLER(OmSnapshotSvcHandler::omVolumeCloneCreateResp);
    auto asyncReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DltObjectIdEpProvider>(om->getDMTNodesForVolume(volId)));

    asyncReq->setPayload(FDSP_MSG_TYPEID(fpi::CreateVolumeCloneMsg), msg);
    asyncReq->onResponseCb(cb);
    asyncReq->invoke();

    // LOGDEBUG << asyncReq->logString() << fds::logString(*msg);

    return err;
}

void OmSnapshotSvcHandler::omVolumeCloneCreateResp(QuorumSvcRequest* svcReq,
                            const Error& error,
                            boost::shared_ptr<std::string> payload) {
}
}  // namespace snapshot
}  // namespace fds
