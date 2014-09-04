
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <OmSvcHandler.h>
#include <fds_error.h>
#include <fds_types.h>
#include <net/net-service.h>
#include <net/net-service-tmpl.hpp>
#include <net/SvcRequestPool.h>
#include <orchMgr.h>

namespace fds {
Error
OmSnapshotSvcHandler::omSnapshotCreate(const fpi::CreateSnapshotMsgPtr &stSnapCreateTxMsg) {
    Error err(ERR_OK);
    fds_verify(stSnapCreateTxMsg != NULL);
    fds_volid_t   volId = stSnapCreateTxMsg->snapshot.volumeId;
    fpi::CreateSnapshotRespMsg crtSnapshotRespMsg;

    auto cb = RESPONSE_MSG_HANDLER(OmSnapshotSvcHandler::omSnapshotCreateResp);
    auto asyncReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DltObjectIdEpProvider>(gl_orch_mgr->getDMTNodesForVolume(volId)));
    asyncReq->setPayload(FDSP_MSG_TYPEID(fpi::CreateSnapshotMsg),
                                                                 stSnapCreateTxMsg);
    asyncReq->onResponseCb(cb);
    asyncReq->invoke();

    // LOGDEBUG << asyncReq->logString() << fds::logString(*stSnapCreateTxMsg);
    return err;
}

void OmSnapshotSvcHandler::omSnapshotCreateResp(QuorumSvcRequest* svcReq,
                            const Error& error,
                            boost::shared_ptr<std::string> payload) {
}


Error
OmSnapshotSvcHandler::omSnapshotDelete(fds_uint64_t snapshotId, fds_uint64_t volId) {
    Error err(ERR_OK);

    fpi::DeleteSnapshotMsgPtr stSnapDeleteTxMsg(new fpi::DeleteSnapshotMsg());

    stSnapDeleteTxMsg->snapshotId = snapshotId;

    auto cb = RESPONSE_MSG_HANDLER(OmSnapshotSvcHandler::omSnapshotDeleteResp);
    auto asyncReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DltObjectIdEpProvider>(gl_orch_mgr->getDMTNodesForVolume(volId)));

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

Error
OmSnapshotSvcHandler::omVolumeCloneCreate(const fpi::CreateVolumeCloneMsgPtr &msg) {
    Error err(ERR_OK);
    fds_verify(msg != NULL);
    fds_volid_t   volId = msg->volumeId;

    auto cb = RESPONSE_MSG_HANDLER(OmSnapshotSvcHandler::omVolumeCloneCreateResp);
    auto asyncReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DltObjectIdEpProvider>(gl_orch_mgr->getDMTNodesForVolume(volId)));

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

}  // namespace fds
