/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <DataMgr.h>
#include <net/net_utils.h>
#include "fdsp/sm_api_types.h"
namespace fds {

ExpungeManager::ExpungeManager(DataMgr* dm) : dm(dm) {
    auto efp = (Error (ExpungeManager::*)(fds_volid_t, const std::vector<ObjectID>&, bool))&ExpungeManager::expunge;
    auto func =std::bind(
        efp, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    dm->timeVolCat_->queryIface()->registerExpungeObjectsCb(func);
}

Error ExpungeManager::expunge(fds_volid_t volId, const std::vector<ObjectID>& vecObjIds, bool force) {
    if (dm->features.isTestMode()) return ERR_OK;  // no SMs, no one to notify
    if (!dm->amIPrimary(volId)) return ERR_OK;

    for (const auto& objId : vecObjIds) {
        dm->lowPriorityTasks.schedule(&ExpungeManager::threadTask, this, volId, objId, force);
    }
    return ERR_OK;
}


Error ExpungeManager::expunge(fds_volid_t volId, const ObjectID& objId, bool force) {
    if (dm->features.isTestMode()) return ERR_OK;  // no SMs, no one to notify
    if (!dm->amIPrimary(volId)) return ERR_OK;

    dm->lowPriorityTasks.schedule(&ExpungeManager::threadTask, this, volId, objId, force);
    return ERR_OK;
}

// request to/response from SMs
Error ExpungeManager::sendDeleteRequest(fds_volid_t volId, const ObjectID &objId) {
    Error err(ERR_OK);

    LOGDEBUG << "sending delete request for vol:" << volId << " obj:" << objId;
    // Create message
    fpi::DeleteObjectMsgPtr expReq(new fpi::DeleteObjectMsg());

    // Set message parameters
    expReq->volId = volId.get();
    fds::assign(expReq->objId, objId);

    // Make RPC call
    DLTManagerPtr dltMgr = MODULEPROVIDER()->getSvcMgr()->getDltManager();
    // get DLT and increment refcount so that DM will respond to
    // DLT commit of the next DMT only after all deletes with this DLT complete
    const DLT* dlt = dltMgr->getAndLockCurrentDLT();
    SHPTR<concurrency::TaskStatus> taskStatus(new concurrency::TaskStatus());
    auto asyncExpReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DltObjectIdEpProvider>(dlt->getNodes(objId)));
    asyncExpReq->setPayload(FDSP_MSG_TYPEID(fpi::DeleteObjectMsg), expReq);
    asyncExpReq->setTimeoutMs(5000);
    asyncExpReq->onResponseCb(RESPONSE_MSG_HANDLER(ExpungeManager::onDeleteResponse,
                                                   dlt->getVersion(), taskStatus));
    asyncExpReq->invoke();
    if (!taskStatus->await(5000)) {
        return ERR_SVC_REQUEST_TIMEOUT;
    }
    return err;

}

void ExpungeManager::onDeleteResponse(fds_uint64_t dltVersion,
                                      SHPTR<concurrency::TaskStatus> taskStatus,
                                      QuorumSvcRequest* svcReq,
                                      const Error& error,
                                      boost::shared_ptr<std::string> payload) {
    fpi::DeleteObjectMsgPtr delobj = svcReq->getRequestPayload <fpi::DeleteObjectMsg>
            (FDSP_MSG_TYPEID(fpi::DeleteObjectMsg));
    ObjectID objId(delobj->objId.digest);
    LOGDEBUG << "received response for volid:" << delobj->volId
             << " objId:" << objId;

    DLTManagerPtr dltMgr = MODULEPROVIDER()->getSvcMgr()->getDltManager();
    dltMgr->decDLTRefcnt(dltVersion);
    taskStatus->done();
}

void ExpungeManager::threadTask(fds_volid_t volId, ObjectID objId, bool force) {
    if (force) {
        LOGDEBUG << "skipping verification for vol:" << volId
                 << " obj:" << objId;
    } else {
        LOGWARN << "checking before objId deletion NOT IMPLEMENTED";
    }

    // do objId checks
    sendDeleteRequest(volId, objId);
}

}  // namespace fds
