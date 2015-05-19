/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <algorithm>
#include <string>
#include <vector>

#include "fds_process.h"
#include "fdsp/dm_api_types.h"
#include "fdsp/sm_api_types.h"

#include <AmDispatcher.h>
#include <net/SvcRequestPool.h>
#include <net/SvcMgr.h>
#include <fiu-control.h>
#include <util/fiu_util.h>
#include "AsyncResponseHandlers.h"

#include "requests/requests.h"
#include <net/MockSvcHandler.h>
#include <AmDispatcherMocks.hpp>
#include "lib/OMgrClient.h"
#include "lib/StatsCollector.h"
#include "TypeIdMap.h"

namespace fds {

// Some logging routines have external linkage
extern std::string logString(const FDS_ProtocolInterface::AbortBlobTxMsg& abortBlobTx);
extern std::string logString(const FDS_ProtocolInterface::CommitBlobTxMsg& commitBlobTx);
extern std::string logString(const FDS_ProtocolInterface::QueryCatalogMsg& qryCat);
extern std::string logString(const FDS_ProtocolInterface::SetBlobMetaDataRspMsg& msg);
extern std::string logString(const FDS_ProtocolInterface::StartBlobTxMsg& stBlobTx);
extern std::string logString(const FDS_ProtocolInterface::UpdateCatalogMsg& updCat);
extern std::string logString(const FDS_ProtocolInterface::UpdateCatalogRspMsg& updCat);
extern std::string logString(const FDS_ProtocolInterface::UpdateCatalogOnceMsg& updCat);
extern std::string logString(const FDS_ProtocolInterface::UpdateCatalogOnceRspMsg& updCat);

extern std::string logString(const FDS_ProtocolInterface::GetObjectMsg &getObj);
extern std::string logString(const FDS_ProtocolInterface::GetObjectResp &getObj);
extern std::string logString(const FDS_ProtocolInterface::PutObjectMsg& putObj);
extern std::string logString(const FDS_ProtocolInterface::PutObjectRspMsg& putObj);

AmDispatcher::AmDispatcher(CommonModuleProviderIf *modProvider) 
        : HasModuleProvider(modProvider)
{
}
AmDispatcher::~AmDispatcher() = default;

void
AmDispatcher::start() {
    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.testing.");
    if (conf.get<fds_bool_t>("uturn_dispatcher_all", false)) {
        fiu_enable("am.uturn.dispatcher", 1, NULL, 0);
        mockTimeoutUs_ = conf.get<uint32_t>("uturn_dispatcher_timeout");
        mockHandler_.reset(new MockSvcHandler());
    } else {
        // Set a custom time for the io messages to dm and sm
        message_timeout_io = conf.get_abs<fds_uint32_t>("fds.am.svc.timeout.io_message", message_timeout_io);
        LOGNOTIFY << "AM IO timeout set to: " << message_timeout_io  << " ms";
    }

    if (conf.get<bool>("standalone", false)) {
        dmtMgr = boost::make_shared<DMTManager>(1);
        dltMgr = boost::make_shared<DLTManager>();
        noNetwork = true;
    } else {
        // Set the DLT manager in svc layer so that it knows to dispatch
        // requests on behalf of specific placement table versions.
        dmtMgr = MODULEPROVIDER()->getSvcMgr()->getDmtManager();
        dltMgr = MODULEPROVIDER()->getSvcMgr()->getDltManager();
        gSvcRequestPool->setDltManager(dltMgr);

	fds_bool_t print_qos_stats = conf.get<bool>("print_qos_stats");
	fds_bool_t disableStreamingStats = conf.get<bool>("toggleDisableStreamingStats");
	if (print_qos_stats) {
	    StatsCollector::singleton()->enableQosStats("AM");
	}
	if (!disableStreamingStats) {
        StatsCollector::singleton()->setSvcMgr(MODULEPROVIDER()->getSvcMgr());
	    StatsCollector::singleton()->startStreaming(nullptr, nullptr);
	}
    }
}

Error
AmDispatcher::updateDlt(bool dlt_type, std::string& dlt_data, OmDltUpdateRespCbType cb) {
    auto err = MODULEPROVIDER()->getSvcMgr()->updateDlt(dlt_type, dlt_data, cb);
    if (ERR_DUPLICATE == err) {
        LOGWARN << "Received duplicate DLT version, ignoring";
        err = ERR_OK;
    }
    return err;
}

Error
AmDispatcher::updateDmt(bool dmt_type, std::string& dmt_data) {
    auto err = MODULEPROVIDER()->getSvcMgr()->updateDmt(dmt_type, dmt_data);
    if (ERR_DUPLICATE == err) {
        LOGWARN << "Received duplicate DMT version, ignoring";
        err = ERR_OK;
    }
    return err;
}

Error
AmDispatcher::getDMT() {
	if (getNoNetwork()) {
		// Standalone mode
		return (ERR_OK);
	} else {
		return (MODULEPROVIDER()->getSvcMgr()->getDMT());
	}
}

Error
AmDispatcher::getDLT() {
	if (getNoNetwork()) {
		// Standalone mode
		return (ERR_OK);
	} else {
		return (MODULEPROVIDER()->getSvcMgr()->getDLT());
	}
}

Error
AmDispatcher::attachVolume(std::string const& volume_name) {
    // should we check the netwrok  readiness here??.  This code is moved  from OMgrClient  to AM. 

    if (noNetwork) return 0;
    try {
        auto req =  gSvcRequestPool->newEPSvcRequest(MODULEPROVIDER()->getSvcMgr()->getOmSvcUuid());
        fpi::GetVolumeDescriptorPtr msg(new fpi::GetVolumeDescriptor());
        msg->volume_name = volume_name;
        req->setPayload(FDSP_MSG_TYPEID(fpi::GetVolumeDescriptor), msg);
        req->invoke();
        LOGNOTIFY << " retrieving volume descriptor from OM for " << volume_name;
    } catch(...) {
        LOGERROR << "OMClient unable to request volume descriptor from OM. Check if OM is up and restart.";
    }
    return 0;

}

/**
 * This dispatch does not get an async reply
 * via SvcLayer since it's an RPC call through
 * OMgrClient. Expect the response via AMSvc
 */
void
AmDispatcher::dispatchAttachVolume(AmRequest *amReq) {
    LOGDEBUG << "Attempting to attach to volume: " << amReq->volume_name;
    auto error = attachVolume(amReq->volume_name);
    amReq->proc_cb(error);
}

/**
 * Quorum request with volume id will always be sent to Data Manager.
 * Look at the dmt to find possible destination DMs.
 */
template<typename Msg>
QuorumSvcRequestPtr
AmDispatcher::createQuorumRequest(fds_volid_t const& volId,
                                  boost::shared_ptr<Msg> const& payload,
                                  QuorumSvcRequestRespCb cb,
                                  uint32_t timeout) const {
    auto quorumReq = gSvcRequestPool->newQuorumSvcRequest(
                            boost::make_shared<DmtVolumeIdEpProvider>(
                                        dmtMgr->getCommittedNodeGroup(volId)));
    auto quorumCnt = dmtMgr->getDMT(DMT_COMMITTED)->getDepth()/2 + 1;
    quorumReq->setQuorumCnt(quorumCnt);
    quorumReq->onResponseCb(cb);
    quorumReq->setTimeoutMs(timeout);
    quorumReq->setPayload(message_type_id(*payload), payload);
    return quorumReq;
}

/**
 * Quorum request with object id will always be sent to Storage Manager.
 * Look at the dmt to find possible destination SMs.
 */
template<typename Msg>
QuorumSvcRequestPtr
AmDispatcher::createQuorumRequest(ObjectID const& objId,
                                  boost::shared_ptr<Msg> const& payload,
                                  QuorumSvcRequestRespCb cb,
                                  uint32_t timeout) const {
    auto const dlt = dltMgr->getDLT();
    auto quorumReq = gSvcRequestPool->newQuorumSvcRequest(
                            boost::make_shared<DltObjectIdEpProvider>(
                                                         dlt->getNodes(objId)));
    auto quorumCnt = dlt->getDepth()/2 + 1;
    quorumReq->setQuorumCnt(quorumCnt);
    quorumReq->onResponseCb(cb);
    quorumReq->setTimeoutMs(timeout);
    quorumReq->setPayload(message_type_id(*payload), payload);
    return quorumReq;
}

template<typename Msg>
FailoverSvcRequestPtr
AmDispatcher::createFailoverRequest(fds_volid_t const& volId,
                                    boost::shared_ptr<Msg> const& payload,
                                    FailoverSvcRequestRespCb cb,
                                    uint32_t timeout) const {
    auto failoverReq = gSvcRequestPool->newFailoverSvcRequest(
                            boost::make_shared<DmtVolumeIdEpProvider>(
                                        dmtMgr->getCommittedNodeGroup(volId)));
    failoverReq->onResponseCb(cb);
    failoverReq->setTimeoutMs(timeout);
    failoverReq->setPayload(message_type_id(*payload), payload);
    return failoverReq;
}

template<typename Msg>
FailoverSvcRequestPtr
AmDispatcher::createFailoverRequest(ObjectID const& objId,
                                  boost::shared_ptr<Msg> const& payload,
                                  FailoverSvcRequestRespCb cb,
                                  uint32_t timeout) const {
    auto const dlt = dltMgr->getDLT();
    auto failoverReq = gSvcRequestPool->newFailoverSvcRequest(
                            boost::make_shared<DltObjectIdEpProvider>(
                                                         dlt->getNodes(objId)));
    failoverReq->onResponseCb(cb);
    failoverReq->setTimeoutMs(timeout);
    failoverReq->setPayload(message_type_id(*payload), payload);
    return failoverReq;
}

/**
 * Dispatch a request to DM asking for permission to access this volume.
 */
void
AmDispatcher::dispatchOpenVolume(fds_volid_t const vol_id,
                                 fds_int64_t const token,
                                 std::function<void(fds_int64_t const, Error const)> cb) {
    fiu_do_on("am.uturn.dispatcher", cb(0, ERR_OK); return;);

    LOGDEBUG << "Attempting to open volume: " << std::hex << vol_id;
    auto volMDMsg = boost::make_shared<fpi::OpenVolumeMsg>();
    volMDMsg->volume_id = vol_id;
    volMDMsg->token = token;

    /** What to do with the response */
    auto svc_cb = [cb] (QuorumSvcRequest* svcReq,
                        const Error& error,
                        boost::shared_ptr<std::string> payload) {
        auto e = error;
        auto msg = deserializeFdspMsg<fpi::OpenVolumeRspMsg>(e, payload);
        cb((msg ? msg->token : invalid_vol_token), e);
    };

    auto asyncOpenVolReq = createQuorumRequest(vol_id, volMDMsg, svc_cb);
    asyncOpenVolReq->invoke();
}

/**
 * Dispatch a request to DM asking for permission to access this volume.
 */
void
AmDispatcher::dispatchCloseVolume(fds_int64_t vol_id, fds_int64_t token) {
    fiu_do_on("am.uturn.dispatcher", return;);

    LOGDEBUG << "Attempting to close volume: " << std::hex << vol_id;
    auto volMDMsg = boost::make_shared<fpi::CloseVolumeMsg>();
    volMDMsg->volume_id = vol_id;
    volMDMsg->token = token;

    QuorumSvcRequestRespCb cb;
    auto asyncCloseVolReq = createQuorumRequest(vol_id, volMDMsg, cb);
    asyncCloseVolReq->invoke();
}

void
AmDispatcher::dispatchStatVolume(AmRequest *amReq) {
    fpi::StatVolumeMsgPtr volMDMsg = boost::make_shared<fpi::StatVolumeMsg>();
    volMDMsg->volume_id = amReq->io_vol_id;

    auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::statVolumeCb, amReq));
    auto asyncStatVolReq = createFailoverRequest(amReq->io_vol_id, volMDMsg,respCb);
    asyncStatVolReq->invoke();
}

void
AmDispatcher::statVolumeCb(AmRequest* amReq,
                           FailoverSvcRequest* svcReq,
                           const Error& error,
                           boost::shared_ptr<std::string> payload) {
    fds_verify(amReq->magicInUse());
    auto volReq = static_cast<fds::StatVolumeReq*>(amReq);
    auto volMDMsg = deserializeFdspMsg<fpi::StatVolumeMsg>(const_cast<Error&>(error), payload);

    if (ERR_OK == error)
        volReq->volumeStatus = volMDMsg->volumeStatus;
    // Notify upper layers that the request is done.
    amReq->proc_cb(error);
}

void
AmDispatcher::dispatchSetVolumeMetadata(AmRequest *amReq) {
    fiu_do_on("am.uturn.dispatcher", amReq->proc_cb(ERR_OK); return;);

    fpi::SetVolumeMetadataMsgPtr volMetaMsg =
            boost::make_shared<fpi::SetVolumeMetadataMsg>();
    auto volReq = static_cast<SetVolumeMetadataReq*>(amReq);

    volMetaMsg->volumeId = amReq->io_vol_id;
    // Copy api structure into fdsp structure.
    // TODO(Andrew): Make these calls use the same structure.
    fpi::FDSP_MetaDataPair metaPair;
    for (auto const &meta : *(volReq->metadata)) {
        metaPair.key = meta.first;
        metaPair.value = meta.second;
        volMetaMsg->metadataList.push_back(metaPair);
    }

    auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::setVolumeMetadataCb, amReq));
    auto asyncSetVolMetadataReq = createQuorumRequest(amReq->io_vol_id,
                                                      volMetaMsg,
                                                      respCb);
    asyncSetVolMetadataReq->invoke();
}

void
AmDispatcher::setVolumeMetadataCb(AmRequest* amReq,
                                  QuorumSvcRequest* svcReq,
                                  const Error& error,
                                  boost::shared_ptr<std::string> payload) {
    fds_verify(amReq->magicInUse());
    auto volReq = static_cast<SetVolumeMetadataReq*>(amReq);
    auto volMetaMsgRsp = deserializeFdspMsg<fpi::SetVolumeMetadataMsgRsp>(
        const_cast<Error&>(error), payload);

    amReq->proc_cb(error);
}

void
AmDispatcher::dispatchGetVolumeMetadata(AmRequest *amReq) {
    fiu_do_on("am.uturn.dispatcher", amReq->proc_cb(ERR_OK); return;);

    auto volMetaMsg = boost::make_shared<fpi::GetVolumeMetadataMsg>();
    volMetaMsg->volumeId = amReq->io_vol_id;

    auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::getVolumeMetadataCb, amReq));
    auto asyncGetVolMetadataReq = createFailoverRequest(amReq->io_vol_id, volMetaMsg, respCb);
    asyncGetVolMetadataReq->invoke();
}

void
AmDispatcher::getVolumeMetadataCb(AmRequest* amReq,
                                  FailoverSvcRequest* svcReq,
                                  const Error& error,
                                  boost::shared_ptr<std::string> payload) {
    fds_verify(amReq->magicInUse());
    auto err = error;

    if (error.ok()) {
        auto response = deserializeFdspMsg<fpi::GetVolumeMetadataMsgRsp>(
            const_cast<Error&>(error), payload);
        auto cb = SHARED_DYN_CAST(GetVolumeMetadataCallback, amReq->cb);
        cb->metadata = boost::make_shared<std::map<std::string, std::string>>();
        // Copy the FDSP structure into the API structure
        for (auto const &meta : response->metadataList) {
            cb->metadata->emplace(std::pair<std::string, std::string>(meta.key, meta.value));
        }
    }

    amReq->proc_cb(error);
}

void
AmDispatcher::dispatchAbortBlobTx(AmRequest *amReq) {
    fiu_do_on("am.uturn.dispatcher", amReq->proc_cb(ERR_OK); return;);

    fds_volid_t volId = amReq->io_vol_id;

    auto stBlobTxMsg = boost::make_shared<fpi::AbortBlobTxMsg>();
    stBlobTxMsg->blob_name      = amReq->getBlobName();
    stBlobTxMsg->blob_version   = blob_version_invalid;
    stBlobTxMsg->txId           = static_cast<AbortBlobTxReq *>(amReq)->tx_desc->getValue();
    stBlobTxMsg->volume_id      = volId;

    auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::abortBlobTxCb, amReq));
    auto asyncAbortBlobTxReq = createQuorumRequest(volId, stBlobTxMsg,respCb);
    asyncAbortBlobTxReq->invoke();

    LOGDEBUG << asyncAbortBlobTxReq->logString() << fds::logString(*stBlobTxMsg);
}

void
AmDispatcher::abortBlobTxCb(AmRequest *amReq,
                            QuorumSvcRequest *svcReq,
                            const Error &error,
                            boost::shared_ptr<std::string> payload) {
    fds_verify(amReq->magicInUse());
    amReq->proc_cb(error);
}

void
AmDispatcher::dispatchStartBlobTx(AmRequest *amReq) {
    auto *blobReq = static_cast<StartBlobTxReq *>(amReq);

    // Update DMT version in request
    // TODO(Andrew): There's a potential race here between the
    // version we cache in the blobReq now and the version we
    // actually dispatch on below. Make the update/dispatch consistent.
    blobReq->dmt_version = dmtMgr->getCommittedVersion();

    fiu_do_on("am.uturn.dispatcher", amReq->proc_cb(ERR_OK); return;);

    // Create network message
    auto startBlobTxMsg = boost::make_shared<fpi::StartBlobTxMsg>();
    startBlobTxMsg->blob_name    = amReq->getBlobName();
    startBlobTxMsg->blob_version = blob_version_invalid;
    startBlobTxMsg->volume_id    = amReq->io_vol_id;
    startBlobTxMsg->blob_mode    = blobReq->blob_mode;
    startBlobTxMsg->txId         = blobReq->tx_desc->getValue();
    startBlobTxMsg->dmt_version  = blobReq->dmt_version;

    auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::startBlobTxCb, amReq));
    auto asyncStartBlobTxReq = createQuorumRequest(amReq->io_vol_id, startBlobTxMsg, respCb);
    asyncStartBlobTxReq->invoke();

    LOGDEBUG << asyncStartBlobTxReq->logString()
             << logString(*startBlobTxMsg);
}

void
AmDispatcher::startBlobTxCb(AmRequest *amReq,
                            QuorumSvcRequest *svcReq,
                            const Error &error,
                            boost::shared_ptr<std::string> payload) {
    fds_verify(amReq->magicInUse());

    // Notify upper layers that the request is done.
    amReq->proc_cb(error);
}

void
AmDispatcher::dispatchDeleteBlob(AmRequest *amReq)
{
    fiu_do_on("am.uturn.dispatcher", amReq->proc_cb(ERR_OK); return;);

    auto message = boost::make_shared<fpi::DeleteBlobMsg>();
    message->volume_id = amReq->io_vol_id;
    message->blob_name = amReq->getBlobName();
    message->blob_version = blob_version_invalid;
    message->txId = static_cast<DeleteBlobReq *>(amReq)->tx_desc->getValue();

    // Create callback
    auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::deleteBlobCb, amReq));
    auto asyncReq = createQuorumRequest(amReq->io_vol_id, message,respCb);

    asyncReq->invoke();
}

void
AmDispatcher::deleteBlobCb(AmRequest* amReq,
                           QuorumSvcRequest* svcReq,
                           const Error& error,
                           boost::shared_ptr<std::string> payload)
{
    fds_verify(amReq->magicInUse());
    LOGDEBUG << " volume:" << amReq->io_vol_id
             << " blob:" << amReq->getBlobName();

    // Return if err
    if (error != ERR_OK) {
        LOGWARN << "error in response: " << error;
    }
    amReq->proc_cb(error);
}

void
AmDispatcher::dispatchUpdateCatalog(AmRequest *amReq) {
    auto blobReq = static_cast<PutBlobReq *>(amReq);
    fiu_do_on("am.uturn.dispatcher",
              blobReq->notifyResponse(ERR_OK);  \
              return;);

    auto updCatMsg(boost::make_shared<fpi::UpdateCatalogMsg>());
    updCatMsg->blob_name    = amReq->getBlobName();
    updCatMsg->blob_version = blob_version_invalid;
    updCatMsg->volume_id    = amReq->io_vol_id;
    updCatMsg->txId         = blobReq->tx_desc->getValue();

    // Setup blob offset updates
    // TODO(Andrew): Today we only expect one offset update
    // TODO(Andrew): Remove lastBuf when we have real transactions
    FDS_ProtocolInterface::FDSP_BlobObjectInfo updBlobInfo;
    updBlobInfo.offset   = amReq->blob_offset;
    updBlobInfo.size     = amReq->data_len;
    updBlobInfo.data_obj_id.digest = std::string(
        reinterpret_cast<const char*>(amReq->obj_id.GetId()),
        amReq->obj_id.GetLen());
    // Add the offset info to the DM message
    updCatMsg->obj_list.push_back(updBlobInfo);

    auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::updateCatalogCb, amReq));
    auto asyncUpdateCatReq = createQuorumRequest(amReq->io_vol_id,
                                                 updCatMsg,
                                                 respCb,
                                                 message_timeout_io);

    fds::PerfTracer::tracePointBegin(amReq->dm_perf_ctx);
    asyncUpdateCatReq->invoke();

    LOGDEBUG << asyncUpdateCatReq->logString() << fds::logString(*updCatMsg);
}

void
AmDispatcher::dispatchUpdateCatalogOnce(AmRequest *amReq) {
    auto blobReq = static_cast<PutBlobReq *>(amReq);
    fiu_do_on("am.uturn.dispatcher",
              blobReq->notifyResponse(ERR_OK); \
              return;);

    auto updCatMsg(boost::make_shared<fpi::UpdateCatalogOnceMsg>());
    updCatMsg->blob_name    = amReq->getBlobName();
    updCatMsg->blob_version = blob_version_invalid;
    updCatMsg->volume_id    = amReq->io_vol_id;
    updCatMsg->txId         = blobReq->tx_desc->getValue();
    updCatMsg->blob_mode    = blobReq->blob_mode;
    updCatMsg->dmt_version  = dmtMgr->getCommittedVersion();

    // Setup blob offset updates
    // TODO(Andrew): Today we only expect one offset update
    // TODO(Andrew): Remove lastBuf when we have real transactions
    fpi::FDSP_BlobObjectInfo updBlobInfo;
    updBlobInfo.offset   = amReq->blob_offset;
    updBlobInfo.size     = amReq->data_len;
    updBlobInfo.data_obj_id.digest = std::string(
        reinterpret_cast<const char*>(amReq->obj_id.GetId()),
        amReq->obj_id.GetLen());
    // Add the offset info to the DM message
    updCatMsg->obj_list.push_back(updBlobInfo);

    // Setup blob metadata updates
    FDS_ProtocolInterface::FDSP_MetaDataPair metaDataPair;
    for (auto& meta : *(blobReq->metadata)) {
        metaDataPair.key   = meta.first;
        metaDataPair.value = meta.second;
        updCatMsg->meta_list.push_back(metaDataPair);
    }

    auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::updateCatalogOnceCb, amReq));
    // Always use the current DMT version since we're updating in a single request
    auto asyncUpdateCatReq = createQuorumRequest(amReq->io_vol_id,
                                                 updCatMsg,
                                                 respCb,
                                                 message_timeout_io);

    PerfTracer::tracePointBegin(amReq->dm_perf_ctx);
    asyncUpdateCatReq->invoke();

    LOGDEBUG << asyncUpdateCatReq->logString() << logString(*updCatMsg);
}

void
AmDispatcher::updateCatalogOnceCb(AmRequest* amReq,
                              QuorumSvcRequest* svcReq,
                              const Error& error,
                              boost::shared_ptr<std::string> payload) {
    fds_verify(amReq->magicInUse());
    PerfTracer::tracePointEnd(amReq->dm_perf_ctx);
    auto updCatRsp = deserializeFdspMsg<fpi::UpdateCatalogOnceRspMsg>(const_cast<Error&>(error), payload);

    auto blobReq = static_cast<PutBlobReq *>(amReq);
    if (error != ERR_OK) {
        LOGERROR << "Obj ID: " << amReq->obj_id
                 << " blob name: " << amReq->getBlobName()
                 << " offset: " << amReq->blob_offset
                 << " Error: " << error;
    } else {
        LOGDEBUG << svcReq->logString() << fds::logString(*updCatRsp);
        blobReq->final_blob_size = updCatRsp->byteCount;
        blobReq->final_meta_data.swap(updCatRsp->meta_list);
    }
    blobReq->notifyResponse(error);
}

void
AmDispatcher::updateCatalogCb(AmRequest* amReq,
                              QuorumSvcRequest* svcReq,
                              const Error& error,
                              boost::shared_ptr<std::string> payload) {
    fds_verify(amReq->magicInUse());
    PerfTracer::tracePointEnd(amReq->dm_perf_ctx);
    auto updCatRsp = deserializeFdspMsg<fpi::UpdateCatalogRspMsg>(const_cast<Error&>(error), payload);

    if (error != ERR_OK) {
        LOGERROR << "Obj ID: " << amReq->obj_id
                 << " blob name: " << amReq->getBlobName()
                 << " offset: " << amReq->blob_offset
                 << " Error: " << error;
    } else {
        LOGDEBUG << svcReq->logString() << fds::logString(*updCatRsp);
    }
    static_cast<PutBlobReq*>(amReq)->notifyResponse(error);
}

void
AmDispatcher::dispatchPutObject(AmRequest *amReq) {
    auto blobReq = static_cast<PutBlobReq *>(amReq);
    fds_verify(amReq->data_len > 0);

    fiu_do_on("am.uturn.dispatcher",
              static_cast<PutBlobReq*>(amReq)->notifyResponse(ERR_OK); \
              return;);

    auto putObjMsg(boost::make_shared<fpi::PutObjectMsg>());
    putObjMsg->volume_id        = amReq->io_vol_id;
    putObjMsg->data_obj.assign(blobReq->dataPtr->c_str(), amReq->data_len);
    putObjMsg->data_obj_len     = amReq->data_len;
    putObjMsg->data_obj_id.digest = std::string(
        reinterpret_cast<const char*>(amReq->obj_id.GetId()),
        amReq->obj_id.GetLen());

    // get DLT and add refcnt to account for in-flight IO sent with
    // this DLT version; when DLT version changes, we don't ack to OM
    // until all in-flight IO for the previous version complete
    auto const dlt = dltMgr->getAndLockCurrentDLT();
    auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::putObjectCb,
                                     amReq, dlt->getVersion()));
    auto asyncPutReq = createQuorumRequest(amReq->obj_id,
                                           putObjMsg,
                                           respCb,
                                           message_timeout_io);

    PerfTracer::tracePointBegin(amReq->sm_perf_ctx);
    asyncPutReq->invoke();

    LOGDEBUG << asyncPutReq->logString() << logString(*putObjMsg)
             << " DLT version " << dlt->getVersion();
}

void
AmDispatcher::putObjectCb(AmRequest* amReq,
                          fds_uint64_t dltVersion,
                          QuorumSvcRequest* svcReq,
                          const Error& error,
                          boost::shared_ptr<std::string> payload) {
    fds_verify(amReq->magicInUse());
    PerfTracer::tracePointEnd(amReq->sm_perf_ctx);
    auto putObjRsp = deserializeFdspMsg<fpi::PutObjectRspMsg>(const_cast<Error&>(error), payload);

    if (error != ERR_OK) {
        LOGERROR << "Obj ID: " << amReq->obj_id
            << " blob name: " << amReq->getBlobName()
            << " offset: " << amReq->blob_offset << " Error: " << error;
    } else {
        LOGDEBUG << svcReq->logString() << logString(*putObjRsp)
                 << " DLT version " << dltVersion;
    }

    // notify DLT manager that request completed, so we can decrement refcnt
    dltMgr->decDLTRefcnt(dltVersion);

    static_cast<fds::PutBlobReq*>(amReq)->notifyResponse(error);
}

void
AmDispatcher::dispatchGetObject(AmRequest *amReq)
{
    // The connectors expect some underlying string even for empty buffers
    fiu_do_on("am.uturn.dispatcher",
              mockHandler_->schedule(mockTimeoutUs_,
                                     std::bind(&AmDispatcherMockCbs::getObjectCb, amReq)); \
                                     return;);

    fds_volid_t volId = amReq->io_vol_id;
    ObjectID objId    = amReq->obj_id;

    // TODO(Andrew): This is a hack to handle reading zero length blobs.
    // The DM or AM cache is going to return a invalid object Id (i.e., 0)
    // the represents an empty blob. Here's we're just going to set the data
    // length to 0 and return. We should figure out how we want this flow to
    // actually go (the entire read API and path should be improved).
    if (objId == NullObjectID) {
        auto cb = SHARED_DYN_CAST(GetObjectCallback, amReq->cb);
        cb->return_buffers = nullptr;
        cb->returnSize = 0;

        amReq->proc_cb(ERR_OK);
        return;
    }

    auto getObjMsg(boost::make_shared<fpi::GetObjectMsg>());
    getObjMsg->volume_id = volId;
    getObjMsg->data_obj_id.digest = std::string(
        reinterpret_cast<const char*>(objId.GetId()),
        objId.GetLen());

    // get DLT and add refcnt to account for in-flight IO sent with
    // this DLT version; when DLT version changes, we don't ack to OM
    // until all in-flight IO for the previous version complete
    auto const dlt = dltMgr->getAndLockCurrentDLT();
    auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::getObjectCb,amReq,dlt->getVersion()));
    auto asyncGetReq = createFailoverRequest(objId,
                                             getObjMsg,
                                             respCb,
                                             message_timeout_io);

    PerfTracer::tracePointBegin(amReq->sm_perf_ctx);
    asyncGetReq->invoke();

    LOGDEBUG << asyncGetReq->logString() << logString(*getObjMsg);
}

void
AmDispatcher::getObjectCb(AmRequest* amReq,
                          fds_uint64_t dltVersion,
                          FailoverSvcRequest* svcReq,
                          const Error& error,
                          boost::shared_ptr<std::string> payload)
{
    fds_verify(amReq->magicInUse());
    PerfTracer::tracePointEnd(amReq->sm_perf_ctx);

    auto cb = SHARED_DYN_CAST(GetObjectCallback, amReq->cb);
    // notify DLT manager that request completed, so we can decrement refcnt
    dltMgr->decDLTRefcnt(dltVersion);

    auto getObjRsp = deserializeFdspMsg<fpi::GetObjectResp>(const_cast<Error&>(error), payload);

    if (error == ERR_OK) {
        LOGDEBUG << svcReq->logString() << logString(*getObjRsp)
                 << " DLT version " << dltVersion;
        cb->returnSize = std::min(amReq->data_len, getObjRsp->data_obj.size());
        auto buffer = boost::make_shared<std::string>(std::move(getObjRsp->data_obj));
        cb->return_buffers = boost::make_shared<std::vector<decltype(buffer)>>(1, std::move(buffer));
    } else {
        cb->returnSize = 0;
        LOGERROR << "blob name: " << amReq->getBlobName() << "offset: "
            << amReq->blob_offset << " Error: " << error;
    }

    amReq->proc_cb(error);
}

void
AmDispatcher::dispatchQueryCatalog(AmRequest *amReq) {
    fiu_do_on("am.uturn.dispatcher",
              mockHandler_->schedule(mockTimeoutUs_,
                                     std::bind(&AmDispatcherMockCbs::queryCatalogCb, amReq)); \
                                     return;);

    PerfTracer::tracePointBegin(amReq->dm_perf_ctx);
    auto blobOffset = amReq->blob_offset;
    auto volId = amReq->io_vol_id;

    LOGDEBUG << "blob name: " << amReq->getBlobName() << " offset: " << blobOffset << " volid: " << volId;
    /*
     * TODO(Andrew): We should eventually specify the offset in the blob
     * we want...all objects won't work well for large blobs.
     */
    auto queryMsg = boost::make_shared<fpi::QueryCatalogMsg>();
    queryMsg->volume_id    = volId;
    queryMsg->blob_name    = amReq->getBlobName();
    queryMsg->start_offset = blobOffset;
    queryMsg->end_offset   = blobOffset;
    // We don't currently specify a version
    queryMsg->blob_version = blob_version_invalid;
    queryMsg->obj_list.clear();
    queryMsg->meta_list.clear();

    auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::getQueryCatalogCb, amReq));
    auto asyncQueryReq = createFailoverRequest(amReq->io_vol_id,
                                               queryMsg,
                                               respCb,
                                               message_timeout_io);

    asyncQueryReq->onEPAppStatusCb(std::bind(&AmDispatcher::getQueryCatalogAppStatusCb,
                                             this, amReq, std::placeholders::_1,
                                             std::placeholders::_2));
    asyncQueryReq->invoke();
    LOGDEBUG << asyncQueryReq->logString() << logString(*queryMsg);
}

fds_bool_t
AmDispatcher::getQueryCatalogAppStatusCb(AmRequest* amReq,
                                         const Error& error,
                                         boost::shared_ptr<std::string> payload) {
    // Tell service layer that it's OK to see these errors. These
    // could mean we're just reading something we haven't written
    // before.
    if ((ERR_CAT_ENTRY_NOT_FOUND == error) ||
        (ERR_BLOB_NOT_FOUND == error)) {
        return true;
    }
    return false;
}

void
AmDispatcher::getQueryCatalogCb(AmRequest* amReq,
                                FailoverSvcRequest* svcReq,
                                const Error& error,
                                boost::shared_ptr<std::string> payload)
{
    fds_verify(amReq->magicInUse());
    PerfTracer::tracePointEnd(amReq->dm_perf_ctx);

    auto qryCatRsp = deserializeFdspMsg<fpi::QueryCatalogMsg>(const_cast<Error&>(error), payload);

    if (error != ERR_OK) {
        // TODO(Andrew): We should consider logging this error at a
        // higher level when the volume is not block
        LOGDEBUG << "blob name: " << amReq->getBlobName() << " offset: "
                 << amReq->blob_offset << " Error: " << error;
        amReq->proc_cb(error == ERR_CAT_ENTRY_NOT_FOUND ? ERR_BLOB_NOT_FOUND : error);
        return;
    }

    LOGDEBUG << svcReq->logString() << logString(*qryCatRsp);

    // Copy the metadata into the callback, if needed
    auto blobReq = static_cast<GetBlobReq *>(amReq);
    if (true == blobReq->get_metadata) {
        auto cb = SHARED_DYN_CAST(GetObjectWithMetadataCallback, amReq->cb);
        // Fill in the data here
        cb->blobDesc = boost::make_shared<BlobDescriptor>();
        cb->blobDesc->setBlobName(amReq->getBlobName());
        cb->blobDesc->setBlobSize(qryCatRsp->byteCount);
        for (const auto& meta : qryCatRsp->meta_list) {
            cb->blobDesc->addKvMeta(meta.key,  meta.value);
        }
    }

    // TODO(xxx) should be able to have multiple object id + implement range
    // queries in DM
    for (fpi::FDSP_BlobObjectList::const_iterator it = qryCatRsp->obj_list.cbegin();
         it != qryCatRsp->obj_list.cend();
         ++it) {
        if ((fds_uint64_t)(*it).offset == amReq->blob_offset) {
            // found offset!!!
            ObjectID objId((*it).data_obj_id.digest);
            // TODO(bszmyd): Mon 23 Mar 2015 02:49:01 AM PDT
            // This is the matching error scenario from the trickery
            // in AmProcessor::getBlobCb due to the write/read race
            // between DM/SM. If this is a retry then the object id should be
            // anything but what it was...or we should be able to get the object
            if (blobReq->retry && (blobReq->last_obj_id != objId)) {
                blobReq->retry = false; // We've gotten a new Id we're not insane
            }
            amReq->obj_id = objId;
            amReq->proc_cb(ERR_OK);
            return;
        }
    }

    // if we are here, we did not get response for offset we needed!
    LOGDEBUG << "blob name: " << amReq->getBlobName() << " offset: "
             << amReq->blob_offset << " not in returned offset list from DM";
    amReq->proc_cb(ERR_BLOB_OFFSET_INVALID);
}

void
AmDispatcher::dispatchStatBlob(AmRequest *amReq)
{
    fiu_do_on("am.uturn.dispatcher",
              auto cb = SHARED_DYN_CAST(StatBlobCallback, amReq->cb); \
              cb->blobDesc = boost::make_shared<BlobDescriptor>(); \
              cb->blobDesc->setBlobName(amReq->getBlobName()); \
              cb->blobDesc->setBlobSize(0); \
              amReq->proc_cb(ERR_OK); \
              return;);

    auto message = boost::make_shared<fpi::GetBlobMetaDataMsg>();
    message->volume_id = amReq->io_vol_id;
    message->blob_name = amReq->getBlobName();

    auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::statBlobCb, amReq));
    auto asyncReq = createFailoverRequest(amReq->io_vol_id, message, respCb);
    asyncReq->invoke();
}

void
AmDispatcher::dispatchSetBlobMetadata(AmRequest *amReq) {
    auto vol_id = amReq->io_vol_id;

    auto blobReq = static_cast<SetBlobMetaDataReq *>(amReq);
    auto setMDMsg = boost::make_shared<fpi::SetBlobMetaDataMsg>();
    setMDMsg->blob_name = amReq->getBlobName();
    setMDMsg->blob_version = blob_version_invalid;
    setMDMsg->volume_id = vol_id;
    setMDMsg->txId = blobReq->tx_desc->getValue();

    setMDMsg->metaDataList = std::move(*blobReq->getMetaDataListPtr());

    // Create callback
    auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::setBlobMetadataCb, amReq));
    auto asyncSetMDReq = createQuorumRequest(vol_id, setMDMsg, respCb);
    asyncSetMDReq->invoke();
}

void
AmDispatcher::setBlobMetadataCb(AmRequest *amReq,
                                QuorumSvcRequest *svcReq,
                                const Error &error,
                                boost::shared_ptr<std::string> payload) {
    fds_verify(amReq->magicInUse());
    auto setMDRsp = deserializeFdspMsg<fpi::SetBlobMetaDataRspMsg>(const_cast<Error&>(error), payload);
    if (error != ERR_OK) {
        LOGERROR << "Set metadata blob name: " << amReq->getBlobName() << " Error: " << error;
    } else {
        LOGDEBUG << svcReq->logString() << fds::logString(*setMDRsp);
    }
    amReq->proc_cb(error);
}

void
AmDispatcher::statBlobCb(AmRequest* amReq,
                         FailoverSvcRequest* svcReq,
                         const Error& error,
                         boost::shared_ptr<std::string> payload)
{
    fds_verify(amReq->magicInUse());
    // Deserialize if all OK
    if (ERR_OK == error) {
        // using the same structure for input and output
        auto response = MSG_DESERIALIZE(GetBlobMetaDataMsg, error, payload);

        auto cb = SHARED_DYN_CAST(StatBlobCallback, amReq->cb);
        // Fill in the data here
        cb->blobDesc = boost::make_shared<BlobDescriptor>();
        cb->blobDesc->setBlobName(amReq->getBlobName());
        cb->blobDesc->setBlobSize(response->byteCount);
        for (const auto& meta : response->metaDataList) {
            cb->blobDesc->addKvMeta(meta.key,  meta.value);
        }
    }
    amReq->proc_cb(error);
}

void
AmDispatcher::dispatchCommitBlobTx(AmRequest *amReq) {
    fiu_do_on("am.uturn.dispatcher", amReq->proc_cb(ERR_OK); return;);

    // Create network message
    auto commitBlobTxMsg = boost::make_shared<fpi::CommitBlobTxMsg>();
    commitBlobTxMsg->blob_name    = amReq->getBlobName();
    commitBlobTxMsg->blob_version = blob_version_invalid;
    commitBlobTxMsg->volume_id    = amReq->io_vol_id;
    commitBlobTxMsg->txId         = static_cast<CommitBlobTxReq *>(amReq)->tx_desc->getValue();
    commitBlobTxMsg->dmt_version  = dmtMgr->getCommittedVersion();

    // Create callback
    auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::commitBlobTxCb, amReq));
    auto asyncCommitBlobTxReq = createQuorumRequest(amReq->io_vol_id, commitBlobTxMsg,respCb);
    asyncCommitBlobTxReq->invoke();

    LOGDEBUG << asyncCommitBlobTxReq->logString()
             << logString(*commitBlobTxMsg);
}

void
AmDispatcher::commitBlobTxCb(AmRequest *amReq,
                            QuorumSvcRequest *svcReq,
                            const Error &error,
                            boost::shared_ptr<std::string> payload) {
    fds_verify(amReq->magicInUse());
    auto response = deserializeFdspMsg<fpi::CommitBlobTxRspMsg>(const_cast<Error&>(error), payload);
    LOGDEBUG << svcReq->logString();
    if (ERR_OK == error) {
        auto blobReq =  static_cast<CommitBlobTxReq *>(amReq);
        blobReq->final_blob_size = response->byteCount;
        blobReq->final_meta_data.swap(response->meta_list);
    }
    // Notify upper layers that the request is done.
    amReq->proc_cb(error);
}

void
AmDispatcher::dispatchVolumeContents(AmRequest *amReq)
{
    fiu_do_on("am.uturn.dispatcher",
              auto cb = SHARED_DYN_CAST(GetBucketCallback, amReq->cb); \
              cb->vecBlobs = boost::make_shared<std::vector<fpi::BlobDescriptor>>(); \
              amReq->proc_cb(ERR_OK); \
              return;);

    auto message = boost::make_shared<fpi::GetBucketMsg>();
    message->volume_id = amReq->io_vol_id;
    message->startPos  = static_cast<VolumeContentsReq *>(amReq)->offset;
    message->count   = static_cast<VolumeContentsReq *>(amReq)->count;
    message->pattern = static_cast<VolumeContentsReq *>(amReq)->pattern;
    message->orderBy = static_cast<VolumeContentsReq *>(amReq)->orderBy;
    message->descending = static_cast<VolumeContentsReq *>(amReq)->descending;

    auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::volumeContentsCb, amReq));
    auto asyncReq = createFailoverRequest(amReq->io_vol_id, message, respCb);
    asyncReq->invoke();
}

void
AmDispatcher::volumeContentsCb(AmRequest* amReq,
                               FailoverSvcRequest* svcReq,
                               const Error& error,
                               boost::shared_ptr<std::string> payload)
{
    fds_verify(amReq->magicInUse());
    // Return if err
    if (ERR_OK == error) {
        // using the same structure for input and output
        auto response = MSG_DESERIALIZE(GetBucketRspMsg, error, payload);

        LOGDEBUG << " volid: " << amReq->io_vol_id << " numBlobs: " <<
                response->blob_descr_list.size();

        auto cb = SHARED_DYN_CAST(GetBucketCallback, amReq->cb);
        cb->vecBlobs = boost::make_shared<std::vector<fpi::BlobDescriptor>>();
        cb->vecBlobs->swap(response->blob_descr_list);
    }
    amReq->proc_cb(error);
}
}  // namespace fds
