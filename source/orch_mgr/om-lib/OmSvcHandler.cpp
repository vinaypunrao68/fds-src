
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

namespace fds {
Error
omSnapshotSvcHandler::omSnapshotCreate(const fpi::CreateSnapshotMsgPtr &stSnapCreateTxMsg) {
    Error err(ERR_OK);
    fds_verify(stSnapCreateTxMsg != NULL);
    fds_volid_t   volId = stSnapCreateTxMsg->snapshot.volumeId;
    fpi::CreateSnapshotRespMsg crtSnapshotRespMsg;

    QuorumSvcRequestRespCb respCb = RESPONSE_MSG_HANDLER(omSnapshotSvcHandler::
                                                    omSnapshotCreateResp);

   /*
    auto asyncCreateSnapshotTxReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DltObjectIdEpProvider>(om_client->getDMTNodesForVolume(volId)));
    asyncCreateSnapshotTxReq->setPayload(FDSP_MSG_TYPEID(fpi::CreateSnapshotMsg),
                                                                 stSnapCreateTxMsg);
    asyncCreateSnapshotTxReq->onResponseCb(respCb);
    asyncCreateSnapshotTxReq->invoke();

    LOGDEBUG << asyncCreateSnapshotTxReq->logString() << fds::logString(*stSnapCreateTxMsg);
   */

    return err;
}

void omSnapshotSvcHandler::omSnapshotCreateResp(QuorumSvcRequest* svcReq,
                            const Error& error,
                            boost::shared_ptr<std::string> payload) {
}


Error
omSnapshotSvcHandler::omSnapshotDelete(fds_uint64_t snapshotId, fds_uint64_t volId) {
    Error err(ERR_OK);
    fds_verify(createSnapshot != NULL);
    DeleteSnapshotMsgPtr stSnapDeleteTxMsg(new DeleteSnapshotMsg());

    stSnapDeleteTxMsg->snapshotId = snapshotId;

    QuorumSvcRequestRespCb respCb;

    auto asyncDeleteSnapshotTxReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DltObjectIdEpProvider>(om_client->getDMTNodesForVolume(volId)));
    asynDeleteSnapshotTxReq->setPayload(FDSP_MSG_TYPEID(fpi::DeleteSnapshotMsg), stSnapDeleteTxMsg);
    asyncDeleteSnapshotTxReq->onResponseCb(respCb);
    asyncDeleteSnapshotTxReq->invoke();

    LOGDEBUG << asyncDeleteSnapshotTxReq->logString() << fds::logString(*stSnapDeleteTxMsg);
}

void omSnapshotSvcHandler::omSnapshotDeleteResp(QuorumSvcRequest* svcReq,
                            const Error& error,
                            boost::shared_ptr<std::string> payload) {
}

Error
omSnapshotSvcHandler::omVolumeCloneCreate(const fpi::CreateVolumeCloneMsgPtr &stVolumeCloneTxMsg) {
    Error err(ERR_OK);
    fds_verify(stVolumeCloneTxMsg != NULL);
    fds_volid_t   volId = stVolumeCloneTxMsg->volumeId;
    fpi::CreateSnapshotRespMsg crtSnapshotRespMsg

    QuorumSvcRequestRespCb respCb;

    auto asyncCreateVolumeCloneTxReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DltObjectIdEpProvider>(om_client->getDMTNodesForVolume(volId)));
    asyncCreateVolumeCloneTxReq->setPayload(FDSP_MSG_TYPEID(fpi::CreateVolumeCloneMsg),
                                                                        stVolumeCloneTxMsg);
    asyncCreateVolumeCloneTxReq->onResponseCb(respCb);
    asyncCreateVolumeCloneTxReq->invoke();

    LOGDEBUG << asyncCreateVolumeCloneTxReq->logString() << fds::logString(*stVolumeCloneTxMsg);

    return err;
}

void omSnapshotSvcHandler::omVolumeCloneCreateResp(QuorumSvcRequest* svcReq,
                            const Error& error,
                            boost::shared_ptr<std::string> payload) {
}

}  // namespace fds
