/*
 * Copyright 2016 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESSMGR_INCLUDE_AMDISPATCHER_TCC_
#define SOURCE_ACCESSMGR_INCLUDE_AMDISPATCHER_TCC_

#include "PerfTrace.h"
#include "TypeIdMap.h"
#include "net/SvcMgr.h"
#include "net/SvcRequestPool.h"
#include "util/Log.h"

namespace fds
{

/**
 * Read from a DM group
 *
 * This will dispatch the message directly to SvcLayer constructing a
 * FailoverRequest on the current DMT column for the indicated volume id.
 * The cb_func will be called with the response along with the original
 * AmRequest structure once the appropriate number of members have responded.
 *
 * @param [in] request  The AmRequest* needing dispatch
 * @param [in] message  The SvcLayer message for the request
 * @param [in] cb_func  A reference to an AmDispatcher method for the callback
 * @param [in] timeout  Option number of milliseconds to timeout
 *
 * @retval void  No return value
 */
template<typename CbMeth, typename MsgPtr, typename ReqPtr>
void
AmDispatcher::readFromDM(ReqPtr request, MsgPtr message, CbMeth cb_func, uint32_t const timeout) {
    auto const& vol_id = request->io_vol_id;
    auto const& dmt_ver = dmtMgr->getAndLockCurrentVersion();
    fds_assert(DMT_VER_INVALID != dmt_ver);
    request->dmt_version = dmt_ver;
    // Take the group from a provided table version
    auto dm_group = dmtMgr->getVersionNodeGroup(vol_id, dmt_ver);
    auto numNodes = std::min(dm_group->getLength(), numPrimaries);
    DmtColumnPtr dmPrimariesForVol = boost::make_shared<DmtColumn>(numNodes);
    for (uint32_t i = 0; numNodes > i; ++i) {
        dmPrimariesForVol->set(i, dm_group->get(i));
    }
    auto primary = boost::make_shared<DmtVolumeIdEpProvider>(dmPrimariesForVol);
    auto requestPool = MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr();
    auto failoverReq = requestPool->newFailoverSvcRequest(primary, dmt_ver);
    failoverReq->onResponseCb([cb_func, request, this] (FailoverSvcRequest* svc,
                                                        const Error& error,
                                                        shared_str payload) mutable -> void {
                                     (this->*(cb_func))(request, svc, error, payload); });
    failoverReq->setTimeoutMs((0 < timeout) ? timeout : message_timeout_default);
    failoverReq->setPayload(message_type_id(*message), message);
    failoverReq->onEPAppStatusCb([request, this] (Error const& error,
                                               shared_str payload) -> bool {
                              return missingBlobStatusCb(request, error, payload); });
    LOGTRACE << "Reading from volume: " << vol_id;
    failoverReq->invoke();
}

/**
 * Write to a DM group
 *
 * This will dispatch the message directly to SvcLayer constructing a
 * MultiPrimaryReq on the current DMT column for the indicated volume id.
 * The cb_func will be called with the response along with the original
 * AmRequest structure once the appropriate number of members have responded.
 *
 * @param [in] request  The AmRequest* needing dispatch
 * @param [in] message  The SvcLayer message for the request
 * @param [in] cb_func  A reference to an AmDispatcher method for the callback
 * @param [in] timeout  Option number of milliseconds to timeout
 *
 * @retval void  No return value
 */
template<typename CbMeth, typename MsgPtr, typename ReqPtr>
void
AmDispatcher::writeToDM(ReqPtr request, MsgPtr message, CbMeth cb_func, uint32_t const timeout) {
    auto const& vol_id = request->io_vol_id;
    auto const& dmt_ver = request->dmt_version;
    fds_assert(DMT_VER_INVALID != dmt_ver);
    // Take the group from a provided table version
    auto dm_group = dmtMgr->getVersionNodeGroup(vol_id, dmt_ver);
    // Assuming the first N (if any) nodes are the primaries and
    // the rest are backups.
    std::vector<fpi::SvcUuid> primaries, secondaries;
    for (size_t i = 0; dm_group->getLength() > i; ++i) {
        auto uuid = dm_group->get(i).toSvcUuid();
        if (numPrimaries > i) {
            primaries.push_back(uuid);
            continue;
        }
        secondaries.push_back(uuid);
    }

    auto requestPool = MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr();
    auto multiReq = requestPool->newMultiPrimarySvcRequest(primaries, secondaries, dmt_ver);
    // TODO(bszmyd): Mon 22 Jun 2015 12:08:25 PM MDT
    // Need to also set a onAllRespondedCb
    multiReq->onPrimariesRespondedCb([cb_func, request, this] (MultiPrimarySvcRequest* svc,
                                                      Error const& error,
                                                      shared_str payload) mutable -> void {
                                     (this->*(cb_func))(request, svc, error, payload); });
    multiReq->setTimeoutMs((0 < timeout) ? timeout : message_timeout_default);
    multiReq->setPayload(message_type_id(*message), message);
    multiReq->onEPAppStatusCb([request, this] (Error const& error,
                                               shared_str payload) -> bool {
                              return missingBlobStatusCb(request, error, payload); });
    setSerialization(request, multiReq);
    LOGTRACE << "Writing from volume: " << vol_id;
    multiReq->invoke();
}

/**
 * Read from an SM group
 *
 * This will dispatch the message directly to SvcLayer constructing a
 * FailoverRequst on the current DLT column for the indicated object id.
 * The cb_func will be called with the response along with the original
 * AmRequest structure once the appropriate number of members have responded.
 *
 * @param [in] request  The AmRequest* needing dispatch
 * @param [in] message  The SvcLayer message for the request
 * @param [in] cb_func  A reference to an AmDispatcher method for the callback
 * @param [in] timeout  Option number of milliseconds to timeout
 *
 * @retval void  No return value
 */
template<typename CbMeth, typename MsgPtr, typename ReqPtr>
void
AmDispatcher::readFromSM(ReqPtr request, MsgPtr message, CbMeth cb_func, uint32_t const timeout) {
    // get DLT and add refcnt to account for in-flight IO sent with
    // this DLT version; when DLT version changes, we don't ack to OM
    // until all in-flight IO for the previous version complete
    auto const dlt = dltMgr->getAndLockCurrentVersion();
    auto dlt_version = dlt->getVersion();
    request->dlt_version = dlt_version;
    auto const& objId = *request->obj_id;
    auto token_group = dlt->getNodes(objId);

    // Build a group of only the primaries for this read
    auto numNodes = token_group->getLength();
    auto nodes = boost::make_shared<DltTokenGroup>(numNodes);
    for (uint32_t i = 0; numNodes > i; ++i) {
       nodes->set(i, token_group->get(i));
    }
    auto provider = boost::make_shared<DltObjectIdEpProvider>(nodes);
    auto requestPool = MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr();
    auto failoverReq = requestPool->newFailoverSvcRequest(provider, dlt_version);
    failoverReq->onResponseCb([cb_func, request, this] (FailoverSvcRequest* svc,
                                                        const Error& error,
                                                        shared_str payload) mutable -> void {
                                     (this->*(cb_func))(request, svc, error, payload); });
    failoverReq->setTimeoutMs((0 < timeout) ? timeout : message_timeout_default);
    failoverReq->setPayload(message_type_id(*message), message);
    PerfTracer::tracePointBegin(request->sm_perf_ctx);
    LOGTRACE << "Reading object: " << objId;
    failoverReq->invoke();
}

/**
 * Write to an SM group
 *
 * This will dispatch the message directly to SvcLayer constructing a
 * QuorumRequest on the current DLT column for the indicated object id.
 * The cb_func will be called with the response along with the original
 * AmRequest structure once the appropriate number of members have responded.
 *
 * @param [in] request  The AmRequest* needing dispatch
 * @param [in] message  The SvcLayer message for the request
 * @param [in] cb_func  A reference to an AmDispatcher method for the callback
 * @param [in] timeout  Option number of milliseconds to timeout
 *
 * @retval void  No return value
 */
template<typename CbMeth, typename MsgPtr, typename ReqPtr>
void
AmDispatcher::writeToSM(ReqPtr request, MsgPtr payload, CbMeth cb_func, uint32_t const timeout) {
    auto const& objId = request->obj_id;
    // get DLT and add refcnt to account for in-flight IO sent with
    // this DLT version; when DLT version changes, we don't ack to OM
    // until all in-flight IO for the previous version complete
    auto const dlt = dltMgr->getAndLockCurrentVersion();
    auto dlt_version = dlt->getVersion();
    request->dlt_version = dlt_version;

    auto token_group = boost::make_shared<DltObjectIdEpProvider>(dlt->getNodes(objId));
    auto num_nodes = token_group->getEps().size();

    auto requestPool = MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr();
    auto quorumReq = requestPool->newQuorumSvcRequest(token_group, dlt_version);
    quorumReq->setTimeoutMs((0 < timeout) ? timeout : message_timeout_default);
    quorumReq->setPayload(message_type_id(*payload), payload);
    quorumReq->onResponseCb([cb_func, request, this] (QuorumSvcRequest* svc,
                                                      const Error& error,
                                                      shared_str payload) mutable -> void {
                                     (this->*(cb_func))(request, svc, error, payload); });
    quorumReq->setQuorumCnt(std::min(num_nodes, 2ul));
    setSerialization(request, quorumReq);
    PerfTracer::tracePointBegin(request->sm_perf_ctx);
    LOGTRACE << "Writing object: " << objId;
    quorumReq->invoke();
}

/**
 * Read from a Volume Group (Non-Mutating message)
 *
 * This will dispatch the message on the appropriate VolumeGroupHandle for the
 * volume in the request. The cb_func will be called with the response along
 * with the original AmRequest structure once the appropriate number of members
 * have responded.
 *
 * @param [in] request  The AmRequest* needing dispatch
 * @param [in] message  The SvcLayer message for the request
 * @param [in] cb_func  A reference to an AmDispatcher method for the callback
 *
 * @retval void  No return value
 */
template<typename CbMeth, typename MsgPtr, typename ReqPtr>
void AmDispatcher::volumeGroupRead(ReqPtr request, MsgPtr message, CbMeth cb_func) {
    auto const& vol_id = request->io_vol_id;
    {
        ReadGuard rg(volumegroup_lock);
        auto it = volumegroup_map.find(vol_id);
        if (volumegroup_map.end() != it) {
            LOGTRACE << "Reading from volume: " << vol_id;
            it->second->sendReadMsg(message_type_id(*message),
                                    message,
                                    [cb_func, request, this] (Error const& e, shared_str p) mutable -> void {
                                        (this->*(cb_func))(request, e, p);
                                    });
            return;
        }
    }
    LOGERROR << "Unknown volume to AmDispatcher: " << vol_id;
    AmDataProvider::unknownTypeCb(request, ERR_VOLUME_ACCESS_DENIED);
}

/**
 * Modify a Volume Group (Mutating message that is not-Readable till committed)
 *
 * This will dispatch the message on the appropriate VolumeGroupHandle for the
 * volume in the request. The cb_func will be called with the response along
 * with the original AmRequest structure once the appropriate number of members
 * have responded.
 *
 * @param [in] request  The AmRequest* needing dispatch
 * @param [in] message  The SvcLayer message for the request
 * @param [in] cb_func  A reference to an AmDispatcher method for the callback
 *
 * @retval void  No return value
 */
template<typename CbMeth, typename MsgPtr, typename ReqPtr>
void AmDispatcher::volumeGroupModify(ReqPtr request, MsgPtr message, CbMeth cb_func) {
    auto const& vol_id = request->io_vol_id;
    {
        ReadGuard rg(volumegroup_lock);
        auto it = volumegroup_map.find(vol_id);
        if (volumegroup_map.end() != it) {
            LOGTRACE << "Staging to volume: " << vol_id;
            it->second->sendModifyMsg(message_type_id(*message),
                                      message,
                                      [cb_func, request, this] (Error const& e, shared_str p) mutable -> void {
                                          (this->*(cb_func))(request, e, p);
                                      });
            return;
        }
    }
    LOGERROR << "Unknown volume to AmDispatcher: " << vol_id;
    AmDataProvider::unknownTypeCb(request, ERR_VOLUME_ACCESS_DENIED);
}

/**
 * Commit to a Volume Group (Mutating message that will be readable)
 *
 * This will dispatch the message on the appropriate VolumeGroupHandle for the
 * volume in the request. The cb_func will be called with the response along
 * with the original AmRequest structure once the appropriate number of members
 * have responded.
 *
 * @param [in] request  The AmRequest* needing dispatch
 * @param [in] message  The SvcLayer message for the request
 * @param [in] cb_func  A reference to an AmDispatcher method for the callback
 * @param [in] vol_lock The lock from the sequence dispatch table
 *
 * @retval void  No return value
 */
template<typename CbMeth, typename MsgPtr, typename ReqPtr>
void AmDispatcher::volumeGroupCommit(ReqPtr request,
                                     MsgPtr message,
                                     CbMeth cb_func,
                                     std::unique_lock<std::mutex>&& vol_lock) {
    auto const& vol_id = request->io_vol_id;
    {
        ReadGuard rg(volumegroup_lock);
        auto it = volumegroup_map.find(vol_id);
        if (volumegroup_map.end() != it) {
            LOGTRACE << "Writing to volume: " << vol_id;
            it->second->sendCommitMsg(message_type_id(*message),
                                      message,
                                      [cb_func, request, this] (Error const& e, shared_str p) mutable -> void {
                                          (this->*(cb_func))(request, e, p);
                                      });
            return;
        }
    }
    // Don't block other IO during our callback
    if (vol_lock) vol_lock.unlock();
    LOGERROR << "Unknown volume to AmDispatcher: " << vol_id;
    AmDataProvider::unknownTypeCb(request, ERR_VOLUME_ACCESS_DENIED);
}

} /* fds */ 

#endif  // SOURCE_ACCESSMGR_INCLUDE_AMDISPATCHER_TCC_
