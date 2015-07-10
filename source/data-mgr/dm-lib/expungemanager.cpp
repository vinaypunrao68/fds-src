/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <DataMgr.h>
#include <net/net_utils.h>
#include "fdsp/sm_api_types.h"
#include <util/stringutils.h>
namespace fds {

ExpungeDB::ExpungeDB() : db(dmutil::getExpungeDBPath()){
}

uint32_t ExpungeDB::increment(fds_volid_t volId, const ObjectID &objId) {
    uint32_t value = getExpungeCount(volId, objId);
    value++;
    db.Update(getKey(volId, objId), util::strformat("%d", value));
    return value;
}

uint32_t ExpungeDB::decrement(fds_volid_t volId, const ObjectID &objId) {
    uint32_t value = getExpungeCount(volId, objId);
    if (0 == value) {
        LOGERROR << "decrementing a refcount which already zero";
        return 0;
    }
    value--;
    if (value == 0) discard(volId, objId);
    else db.Update(getKey(volId, objId), util::strformat("%d", value));
    return value;
}

uint32_t ExpungeDB::getExpungeCount(fds_volid_t volId, const ObjectID &objId) {
    std::string value;
    Error err = db.Query(getKey(volId, objId), &value);
    if (err.ok()) {
        return *((uint32_t*)value.data());
    }
    return 0;
}

void ExpungeDB::discard(fds_volid_t volId, const ObjectID &objId) {
    if (!db.Delete(getKey(volId, objId))) {
        LOGWARN << "unable to delete from expungedb vol:" << volId
                << " obj:" << objId;
    }
}

std::string ExpungeDB::getKey(fds_volid_t volId, const ObjectID &objId) {
    return util::strformat("%ld:%b", volId, objId.GetId(),objId.getDigestLength());
}

ExpungeDB::~ExpungeDB() {

}

ExpungeManager::ExpungeManager(DataMgr* dm) : dm(dm) {
    auto efp = (Error (ExpungeManager::*)(fds_volid_t, const std::vector<ObjectID>&, bool))&ExpungeManager::expunge;
    auto func =std::bind(
        efp, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    dm->timeVolCat_->queryIface()->registerExpungeObjectsCb(func);
    expungeDB.reset(new ExpungeDB());
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

    // Assuming the number of primaries is the same as DM (it is for now),
    // the rest are backups.
    std::vector<fpi::SvcUuid> primaries, secondaries;
    auto const numPrimaries = dm->getNumOfPrimary();
    auto token_group = dlt->getNodes(objId);
    for (size_t i = 0; token_group->getLength() > i; ++i) {
        auto uuid = token_group->get(i).toSvcUuid();
        if (numPrimaries > i) {
            primaries.push_back(uuid);
            continue;
        }
        secondaries.push_back(uuid);
    }

    auto dlt_version = dlt->getVersion();
    auto multiReq = gSvcRequestPool->newMultiPrimarySvcRequest(primaries, secondaries, dlt_version);

    multiReq->setPayload(FDSP_MSG_TYPEID(fpi::DeleteObjectMsg), expReq);
    multiReq->setTimeoutMs(5000);
    multiReq->onPrimariesRespondedCb(RESPONSE_MSG_HANDLER(ExpungeManager::onDeleteResponse,
                                                   dlt_version, taskStatus));
    multiReq->invoke();
    if (!taskStatus->await(5000)) {
        return ERR_SVC_REQUEST_TIMEOUT;
    }
    return err;

}

void ExpungeManager::onDeleteResponse(fds_uint64_t dltVersion,
                                      SHPTR<concurrency::TaskStatus> taskStatus,
                                      MultiPrimarySvcRequest* svcReq,
                                      const Error& error,
                                      boost::shared_ptr<std::string> payload) {
    fpi::DeleteObjectMsgPtr delobj = svcReq->getRequestPayload <fpi::DeleteObjectMsg>
            (FDSP_MSG_TYPEID(fpi::DeleteObjectMsg));
    ObjectID objId(delobj->objId.digest);
    LOGDEBUG << "received response for volid:" << delobj->volId
             << " objId:" << objId
             << " err:" << error;

    if (error.ok()) {
        expungeDB->decrement((fds_volid_t)delobj->volId, objId);
    } else {
        LOGWARN << "error response for volid:" << delobj->volId
                << " objId:" << objId
                << " err:" << error;
    }

    DLTManagerPtr dltMgr = MODULEPROVIDER()->getSvcMgr()->getDltManager();
    dltMgr->decDLTRefcnt(dltVersion);
    taskStatus->done();
}

void ExpungeManager::threadTask(fds_volid_t volId, ObjectID objId, bool fFromSnapshot) {
    uint32_t count = 0;
    bool fIsInSnapshot = dm->timelineMgr->isObjectInSnapshot(objId, volId);
    if (fIsInSnapshot) {
        if (fFromSnapshot) {
            LOGDEBUG << "obj delete from snap in another snap, so ignoring. "
                     << "vol:" << volId << " obj:" << objId;
            return;
        } else {
            LOGDEBUG << "obj in snap so incrementing ref count "
                     << "vol:" << volId << " obj:" << objId;
            expungeDB->increment(volId, objId);
        }
    } else {
        uint32_t count = expungeDB->getExpungeCount(volId, objId);
        if (!fFromSnapshot) count++; // old count + current request;
        LOGDEBUG << "will send [" << count << "] delete requests for "
                 << "vol:" << volId << " obj:" << objId;

        // the double check is to avoid race conditions
        // the decrement happens after a response is received.
        for (; expungeDB->getExpungeCount(volId, objId) > 0 && count > 0; count--) {
            sendDeleteRequest(volId, objId);
        }
    }
}

}  // namespace fds
