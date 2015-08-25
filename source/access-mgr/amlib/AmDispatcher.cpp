/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <algorithm>
#include <future>
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
#include "requests/GetObjectReq.h"
#include <net/MockSvcHandler.h>
#include <AmDispatcherMocks.hpp>
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

/**
 * See Serialization enumeration for interpretation.
 */
static const char* SerialNames[] = {
    "none",
    "volume",
    "blob"
};

enum class Serialization {
    SERIAL_NONE,      // May result in inconsistencies between members of redundancy groups.
    SERIAL_VOLUME,    // Prevents inconsistencies, but offers less concurrency than SERIAL_BLOB.
    SERIAL_BLOB       // Prevents inconsistencies and offers the greatest degree of concurrency.
} serialization;

static constexpr uint32_t DmDefaultPrimaryCnt { 2 };
static const std::string DefaultSerialization { "volume" };

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
    if (conf.get<bool>("fault.unreachable", false)) {
        float frequency = 0.001;
        MODULEPROVIDER()->getSvcMgr()->setUnreachableInjection(frequency);
    }

    numPrimaries = conf.get_abs<fds_uint32_t>("fds.dm.number_of_primary", DmDefaultPrimaryCnt);

    /**
     * What request serialization technique will we use?
     */
    int serialSelection;
    auto serializationString = conf.get_abs<std::string>("fds.feature_toggle.am.serialization", DefaultSerialization);
    if (strcasecmp(serializationString.c_str(), SerialNames[0]) == 0) {
        serialization = Serialization::SERIAL_NONE;
        serialSelection = 0;
    } else if (strcasecmp(serializationString.c_str(), SerialNames[1]) == 0) {
        serialization = Serialization::SERIAL_VOLUME;
        serialSelection = 1;
    } else if (strcasecmp(serializationString.c_str(), SerialNames[2]) == 0) {
        serialization = Serialization::SERIAL_BLOB;
        serialSelection = 2;
    } else {
        serialization = Serialization::SERIAL_NONE;
        serialSelection = 0;
        LOGNOTIFY << "AM request serialization unrecognized: " << serializationString  << ".";
    }
    LOGNOTIFY << "AM request serialization set to: " << SerialNames[serialSelection]  << ".";
}

Error
AmDispatcher::updateDlt(bool dlt_type, std::string& dlt_data, FDS_Table::callback_type const& cb) {
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
    if (!noNetwork) {
        // We need valid DLT and DMTs before we can start issuing IO,
        // block attachments here until this is true.
        if (DMT_VER_INVALID == dmtMgr->getCommittedVersion() ||
            nullptr == dltMgr->getDLT()) {
            LOGWARN << "Could not attach to volume before receiving domain tables.";
            return ERR_NOT_READY;
        }
        try {
            auto req =  gSvcRequestPool->newEPSvcRequest(MODULEPROVIDER()->getSvcMgr()->getOmSvcUuid());
            fpi::GetVolumeDescriptorPtr msg(new fpi::GetVolumeDescriptor());
            msg->volume_name = volume_name;
            req->setPayload(FDSP_MSG_TYPEID(fpi::GetVolumeDescriptor), msg);
            req->invoke();
            LOGNOTIFY << " retrieving volume descriptor from OM for " << volume_name;
        } catch(...) {
            LOGERROR << "OMClient unable to request volume descriptor from OM. Check if OM is up and restart.";
            return ERR_NOT_READY;
        }
    }
    return ERR_OK;
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
 * MultiPrimary request with volume id will always be sent to Data Manager.
 * Look at the dmt to find possible destination DMs.
 */
template<typename Msg>
MultiPrimarySvcRequestPtr
AmDispatcher::createMultiPrimaryRequest(fds_volid_t const& volId,
                                        fds_uint64_t const dmt_ver,
                                        boost::shared_ptr<Msg> const& payload,
                                        MultiPrimarySvcRequestRespCb cb,
                                        uint32_t timeout) const {
    fds_assert(DMT_VER_INVALID != dmt_ver);
    // Take the group from a provided table version
    auto token_group = dmtMgr->getVersionNodeGroup(volId, dmt_ver);

    // Assuming the first N (if any) nodes are the primaries and
    // the rest are backups.
    std::vector<fpi::SvcUuid> primaries, secondaries;
    for (size_t i = 0; token_group->getLength() > i; ++i) {
        auto uuid = token_group->get(i).toSvcUuid();
        if (numPrimaries > i) {
            primaries.push_back(uuid);
            continue;
        }
        secondaries.push_back(uuid);
    }

    auto multiReq = gSvcRequestPool->newMultiPrimarySvcRequest(primaries, secondaries);
    // TODO(bszmyd): Mon 22 Jun 2015 12:08:25 PM MDT
    // Need to also set a onAllRespondedCb
    multiReq->onPrimariesRespondedCb(cb);
    multiReq->setTimeoutMs(timeout);
    multiReq->setPayload(message_type_id(*payload), payload);
    return multiReq;
}

/**
 * MultiPrimary request with object id will always be sent to Storage Manager.
 * Look at the dmt to find possible destination SMs.
 */
template<typename Msg>
MultiPrimarySvcRequestPtr
AmDispatcher::createMultiPrimaryRequest(ObjectID const& objId,
                                        DLT const* dlt,
                                        boost::shared_ptr<Msg> const& payload,
                                        MultiPrimarySvcRequestRespCb cb,
                                        uint32_t timeout) const {
    auto token_group = dlt->getNodes(objId);
    auto dlt_version = dlt->getVersion();

    // Assuming the first N (if any) nodes are the primaries and
    // the rest are backups.
    std::vector<fpi::SvcUuid> primaries, secondaries;
    for (size_t i = 0; token_group->getLength() > i; ++i) {
        auto uuid = token_group->get(i).toSvcUuid();
        if (numPrimaries > i) {
            primaries.push_back(uuid);
            continue;
        }
        secondaries.push_back(uuid);
    }

    auto multiReq = gSvcRequestPool->newMultiPrimarySvcRequest(primaries, secondaries, dlt_version);
    // TODO(bszmyd): Mon 22 Jun 2015 12:08:25 PM MDT
    // Need to also set a onAllRespondedCb
    multiReq->onPrimariesRespondedCb(cb);
    multiReq->setTimeoutMs(timeout);
    multiReq->setPayload(message_type_id(*payload), payload);
    return multiReq;
}

template<typename Msg>
FailoverSvcRequestPtr
AmDispatcher::createFailoverRequest(fds_volid_t const& volId,
                                    fds_uint64_t const dmt_ver,
                                    boost::shared_ptr<Msg> const& payload,
                                    FailoverSvcRequestRespCb cb,
                                    uint32_t timeout) const {
    fds_assert(DMT_VER_INVALID != dmt_ver);
    // Take the group from a provided table version
    auto token_group = dmtMgr->getVersionNodeGroup(volId, dmt_ver);
    auto numNodes = std::min(token_group->getLength(), numPrimaries);
    DmtColumnPtr dmPrimariesForVol = boost::make_shared<DmtColumn>(numNodes);
    for (uint32_t i = 0; numNodes > i; ++i) {
        dmPrimariesForVol->set(i, token_group->get(i));
    }
    auto primary = boost::make_shared<DmtVolumeIdEpProvider>(dmPrimariesForVol);
    auto failoverReq = gSvcRequestPool->newFailoverSvcRequest(primary);
    failoverReq->onResponseCb(cb);
    failoverReq->setTimeoutMs(timeout);
    failoverReq->setPayload(message_type_id(*payload), payload);
    return failoverReq;
}

template<typename Msg>
FailoverSvcRequestPtr
AmDispatcher::createFailoverRequest(ObjectID const& objId,
                                    DLT const* dlt,
                                    boost::shared_ptr<Msg> const& payload,
                                    FailoverSvcRequestRespCb cb,
                                    uint32_t timeout) const {
    auto token_group = dlt->getNodes(objId);
    auto dlt_version = dlt->getVersion();

    // Build a group of only the primaries for this read
    auto numNodes = std::min(token_group->getLength(), numPrimaries);
    auto primary_nodes = boost::make_shared<DltTokenGroup>(numNodes);
    for (uint32_t i = 0; numNodes > i; ++i) {
       primary_nodes->set(i, token_group->get(i));
    }
    auto primary = boost::make_shared<DltObjectIdEpProvider>(primary_nodes);
    auto failoverReq = gSvcRequestPool->newFailoverSvcRequest(primary, dlt_version);
    failoverReq->onResponseCb(cb);
    failoverReq->setTimeoutMs(timeout);
    failoverReq->setPayload(message_type_id(*payload), payload);
    return failoverReq;
}

/**
 * @brief Set the configured request serialization.
 */
void
AmDispatcher::setSerialization(AmRequest* amReq, boost::shared_ptr<SvcRequestIf> svcReq) {
    static std::hash<fds_volid_t> volIDHash;
    static std::hash<std::string> blobNameHash;

    switch (serialization) {
        case Serialization::SERIAL_VOLUME:
            svcReq->setTaskExecutorId(volIDHash(amReq->io_vol_id));
            break;

        case Serialization::SERIAL_BLOB:
            svcReq->setTaskExecutorId((volIDHash(amReq->io_vol_id) + blobNameHash(amReq->getBlobName())));
            break;

        default:
            break;
    }
}

/**
 * Dispatch a request to DM asking for permission to access this volume.
 */
void
AmDispatcher::dispatchOpenVolume(AmRequest* amReq) {
    fiu_do_on("am.uturn.dispatcher", amReq->proc_cb(ERR_OK); return;);
    auto volReq = static_cast<fds::AttachVolumeReq*>(amReq);
    volReq->dmt_version = dmtMgr->getAndLockCurrentVersion();

    LOGDEBUG << "Attempting to open volume: " << std::hex << amReq->io_vol_id
             << " with token: " << volReq->token;

    auto volMDMsg = boost::make_shared<fpi::OpenVolumeMsg>();
    volMDMsg->volume_id = amReq->io_vol_id.get();
    volMDMsg->token = volReq->token;
    volMDMsg->mode = volReq->mode;

    /** What to do with the response */
    auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::dispatchOpenVolumeCb, amReq));
    auto asyncOpenVolReq = createMultiPrimaryRequest(amReq->io_vol_id, volReq->dmt_version, volMDMsg, respCb);
    setSerialization(amReq, asyncOpenVolReq);
    asyncOpenVolReq->invoke();
}

void
AmDispatcher::dispatchOpenVolumeCb(AmRequest* amReq,
                                   MultiPrimarySvcRequest* svcReq,
                                   const Error& error,
                                   boost::shared_ptr<std::string> payload) const {
    auto volReq = static_cast<fds::AttachVolumeReq*>(amReq);
    dmtMgr->releaseVersion(volReq->dmt_version);
    auto e = error;
    auto msg = deserializeFdspMsg<fpi::OpenVolumeRspMsg>(e, payload);
    if (e.ok()) {
        // Set the token and volume sequence returned by the DM
        volReq->token = (msg ? msg->token : invalid_vol_token);
        volReq->vol_sequence = msg->sequence_id;
    }
    amReq->proc_cb(e);
}

/**
 * Dispatch a request to DM asking for permission to access this volume.
 */
Error
AmDispatcher::dispatchCloseVolume(fds_volid_t vol_id, fds_int64_t token) {
    fiu_do_on("am.uturn.dispatcher", return ERR_OK;);

    LOGDEBUG << "Attempting to close volume: " << vol_id;
    auto volMDMsg = boost::make_shared<fpi::CloseVolumeMsg>();
    volMDMsg->volume_id = vol_id.get();
    volMDMsg->token = token;

    // This gives this request blocking semantics
    std::promise<Error> done;
    std::shared_future<Error> ready(done.get_future());
    MultiPrimarySvcRequestRespCb cb =
        [&done] (MultiPrimarySvcRequest* svcReq,
                 const Error& error,
                 boost::shared_ptr<std::string> payload) mutable -> void {
            return done.set_value(error); // Trigger processor thread
        };

    auto dmt_version = dmtMgr->getAndLockCurrentVersion();
    auto asyncCloseVolReq = createMultiPrimaryRequest(vol_id, dmt_version, volMDMsg, cb);

    /**
     * Need an AmRequest to call the serialization setter.
     */
    auto amReq = std::unique_ptr<AmRequest>(new AmRequest(fds::fds_io_op_t(0), vol_id, "", "", nullptr));
    setSerialization(amReq.get(), asyncCloseVolReq);

    asyncCloseVolReq->invoke();
    auto err = ready.get();

    // Release the DMT version so we can roll if needed
    dmtMgr->releaseVersion(dmt_version);
    return err;
}

void
AmDispatcher::dispatchStatVolume(AmRequest *amReq) {
    auto volMDMsg = boost::make_shared<fpi::StatVolumeMsg>();
    volMDMsg->volume_id = amReq->io_vol_id.get();

    amReq->dmt_version = dmtMgr->getAndLockCurrentVersion();
    auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::statVolumeCb, amReq));
    auto asyncStatVolReq = createFailoverRequest(amReq->io_vol_id,
                                                 amReq->dmt_version,
                                                 volMDMsg,
                                                 respCb);
    asyncStatVolReq->invoke();
}

void
AmDispatcher::statVolumeCb(AmRequest* amReq,
                           FailoverSvcRequest* svcReq,
                           const Error& error,
                           boost::shared_ptr<std::string> payload) {
    // Ensure we haven't already replied to this request
    if (!amReq->isCompleted()) { return; }

    // Release DMT version
    dmtMgr->releaseVersion(amReq->dmt_version);

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

    volReq->dmt_version = dmtMgr->getAndLockCurrentVersion();
    volMetaMsg->volumeId = amReq->io_vol_id.get();
    // Copy api structure into fdsp structure.
    // TODO(Andrew): Make these calls use the same structure.
    fpi::FDSP_MetaDataPair metaPair;
    for (auto const &meta : *(volReq->metadata)) {
        metaPair.key = meta.first;
        metaPair.value = meta.second;
        volMetaMsg->metadataList.push_back(metaPair);
    }

    auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::setVolumeMetadataCb, amReq));
    auto asyncSetVolMetadataReq = createMultiPrimaryRequest(amReq->io_vol_id,
                                                            amReq->dmt_version,
                                                            volMetaMsg,
                                                            respCb);
    setSerialization(amReq, asyncSetVolMetadataReq);
    asyncSetVolMetadataReq->invoke();
}

void
AmDispatcher::setVolumeMetadataCb(AmRequest* amReq,
                                  MultiPrimarySvcRequest* svcReq,
                                  const Error& error,
                                  boost::shared_ptr<std::string> payload) {
    // Ensure we haven't already replied to this request
    if (!amReq->isCompleted()) { return; }
    dmtMgr->releaseVersion(amReq->dmt_version);
    auto volReq = static_cast<SetVolumeMetadataReq*>(amReq);
    auto volMetaMsgRsp = deserializeFdspMsg<fpi::SetVolumeMetadataMsgRsp>(
        const_cast<Error&>(error), payload);

    amReq->proc_cb(error);
}

void
AmDispatcher::dispatchGetVolumeMetadata(AmRequest *amReq) {
    fiu_do_on("am.uturn.dispatcher", amReq->proc_cb(ERR_OK); return;);

    auto volMetaMsg = boost::make_shared<fpi::GetVolumeMetadataMsg>();
    volMetaMsg->volumeId = amReq->io_vol_id.get();

    amReq->dmt_version = dmtMgr->getAndLockCurrentVersion();
    auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::getVolumeMetadataCb, amReq));
    auto asyncGetVolMetadataReq = createFailoverRequest(amReq->io_vol_id,
                                                        amReq->dmt_version,
                                                        volMetaMsg,
                                                        respCb);
    asyncGetVolMetadataReq->invoke();
}

void
AmDispatcher::getVolumeMetadataCb(AmRequest* amReq,
                                  FailoverSvcRequest* svcReq,
                                  const Error& error,
                                  boost::shared_ptr<std::string> payload) {
    // Ensure we haven't already replied to this request
    if (!amReq->isCompleted()) { return; }
    dmtMgr->releaseVersion(amReq->dmt_version);
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

    auto blobReq = static_cast<AbortBlobTxReq*>(amReq);
    auto abBlobTxMsg = boost::make_shared<fpi::AbortBlobTxMsg>();
    abBlobTxMsg->blob_name      = amReq->getBlobName();
    abBlobTxMsg->blob_version   = blob_version_invalid;
    abBlobTxMsg->volume_id      = volId.get();
    abBlobTxMsg->txId           = blobReq->tx_desc->getValue();

    auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::abortBlobTxCb, amReq));
    auto asyncAbortBlobTxReq = createMultiPrimaryRequest(volId, blobReq->dmt_version, abBlobTxMsg,respCb);
    setSerialization(amReq, asyncAbortBlobTxReq);
    asyncAbortBlobTxReq->invoke();

    LOGDEBUG << asyncAbortBlobTxReq->logString() << fds::logString(*abBlobTxMsg);
}

void
AmDispatcher::abortBlobTxCb(AmRequest *amReq,
                            MultiPrimarySvcRequest *svcReq,
                            const Error &error,
                            boost::shared_ptr<std::string> payload) {
    // Ensure we haven't already replied to this request
    if (!amReq->isCompleted()) { return; }
    dmtMgr->releaseVersion(amReq->dmt_version);
    amReq->proc_cb(error);
}

void
AmDispatcher::dispatchStartBlobTx(AmRequest *amReq) {
    auto *blobReq = static_cast<StartBlobTxReq *>(amReq);

    // Update DMT version in request
    // TODO(Andrew): There's a potential race here between the
    // version we cache in the blobReq now and the version we
    // actually dispatch on below. Make the update/dispatch consistent.
    blobReq->dmt_version = dmtMgr->getAndLockCurrentVersion();

    fiu_do_on("am.uturn.dispatcher", amReq->proc_cb(ERR_OK); return;);

    // Create network message
    auto startBlobTxMsg = boost::make_shared<fpi::StartBlobTxMsg>();
    startBlobTxMsg->blob_name    = amReq->getBlobName();
    startBlobTxMsg->blob_version = blob_version_invalid;
    startBlobTxMsg->volume_id    = amReq->io_vol_id.get();
    startBlobTxMsg->blob_mode    = blobReq->blob_mode;
    startBlobTxMsg->txId         = blobReq->tx_desc->getValue();
    startBlobTxMsg->dmt_version  = blobReq->dmt_version;

    auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::startBlobTxCb, amReq));
    auto asyncStartBlobTxReq = createMultiPrimaryRequest(amReq->io_vol_id,
                                                         blobReq->dmt_version,
                                                         startBlobTxMsg,
                                                         respCb);
    setSerialization(amReq, asyncStartBlobTxReq);
    asyncStartBlobTxReq->invoke();

    LOGDEBUG << asyncStartBlobTxReq->logString()
             << logString(*startBlobTxMsg);
}

void
AmDispatcher::startBlobTxCb(AmRequest *amReq,
                            MultiPrimarySvcRequest *svcReq,
                            const Error &error,
                            boost::shared_ptr<std::string> payload) {
    // Ensure we haven't already replied to this request
    if (!amReq->isCompleted()) { return; }

    // Release DMT version if transaction did not start.
    if (!error.ok()) {
        dmtMgr->releaseVersion(amReq->dmt_version);
    }

    // Notify upper layers that the request is done.
    amReq->proc_cb(error);
}

void
AmDispatcher::dispatchDeleteBlob(AmRequest *amReq)
{
    fiu_do_on("am.uturn.dispatcher", amReq->proc_cb(ERR_OK); return;);
    auto blobReq = static_cast<DeleteBlobReq *>(amReq);
    auto message = boost::make_shared<fpi::DeleteBlobMsg>();
    message->volume_id = amReq->io_vol_id.get();
    message->blob_name = amReq->getBlobName();
    message->blob_version = blob_version_invalid;
    message->txId = blobReq->tx_desc->getValue();

    // Create callback
    auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::deleteBlobCb, amReq));
    auto asyncReq = createMultiPrimaryRequest(amReq->io_vol_id, blobReq->dmt_version, message, respCb);
    asyncReq->onEPAppStatusCb(std::bind(&AmDispatcher::missingBlobStatusCb,
                                        this, amReq, std::placeholders::_1,
                                        std::placeholders::_2));

    setSerialization(amReq, asyncReq);
    asyncReq->invoke();
}

void
AmDispatcher::deleteBlobCb(AmRequest* amReq,
                           MultiPrimarySvcRequest* svcReq,
                           const Error& error,
                           boost::shared_ptr<std::string> payload)
{
    // Ensure we haven't already replied to this request
    if (!amReq->isCompleted()) { return; }
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
    updCatMsg->volume_id    = amReq->io_vol_id.get();
    updCatMsg->txId         = blobReq->tx_desc->getValue();

    // Setup blob offset updates
    // TODO(Andrew): Today we only expect one offset update
    // TODO(Andrew): Remove lastBuf when we have real transactions
    FDS_ProtocolInterface::FDSP_BlobObjectInfo updBlobInfo;
    updBlobInfo.offset   = amReq->blob_offset;
    updBlobInfo.size     = amReq->data_len;
    updBlobInfo.data_obj_id.digest = std::string(
        reinterpret_cast<const char*>(blobReq->obj_id.GetId()),
        blobReq->obj_id.GetLen());
    // Add the offset info to the DM message
    updCatMsg->obj_list.push_back(updBlobInfo);

    auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::updateCatalogCb, amReq));
    auto asyncUpdateCatReq = createMultiPrimaryRequest(amReq->io_vol_id,
                                                       blobReq->dmt_version,
                                                       updCatMsg,
                                                       respCb,
                                                       message_timeout_io);

    fds::PerfTracer::tracePointBegin(amReq->dm_perf_ctx);
    setSerialization(amReq, asyncUpdateCatReq);
    asyncUpdateCatReq->invoke();

    LOGDEBUG << asyncUpdateCatReq->logString() << fds::logString(*updCatMsg);
}

void
AmDispatcher::dispatchUpdateCatalogOnce(AmRequest *amReq) {
    auto blobReq = static_cast<PutBlobReq *>(amReq);
    fiu_do_on("am.uturn.dispatcher",
              blobReq->notifyResponse(ERR_OK); \
              return;);

    blobReq->dmt_version = dmtMgr->getAndLockCurrentVersion();

    auto updCatMsg(boost::make_shared<fpi::UpdateCatalogOnceMsg>());
    updCatMsg->blob_name    = amReq->getBlobName();
    updCatMsg->blob_version = blob_version_invalid;
    updCatMsg->volume_id    = amReq->io_vol_id.get();
    updCatMsg->txId         = blobReq->tx_desc->getValue();
    updCatMsg->blob_mode    = blobReq->blob_mode;
    updCatMsg->dmt_version  = blobReq->dmt_version;
    updCatMsg->sequence_id  = blobReq->vol_sequence;

    // Setup blob offset updates
    // TODO(Andrew): Today we only expect one offset update
    // TODO(Andrew): Remove lastBuf when we have real transactions
    fpi::FDSP_BlobObjectInfo updBlobInfo;
    updBlobInfo.offset   = amReq->blob_offset;
    updBlobInfo.size     = amReq->data_len;
    updBlobInfo.data_obj_id.digest = std::string(
        reinterpret_cast<const char*>(blobReq->obj_id.GetId()),
        blobReq->obj_id.GetLen());
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
    auto asyncUpdateCatReq = createMultiPrimaryRequest(amReq->io_vol_id,
                                                       blobReq->dmt_version,
                                                       updCatMsg,
                                                       respCb,
                                                       message_timeout_io);

    PerfTracer::tracePointBegin(amReq->dm_perf_ctx);
    setSerialization(amReq, asyncUpdateCatReq);
    asyncUpdateCatReq->invoke();

    LOGDEBUG << asyncUpdateCatReq->logString() << logString(*updCatMsg);
}

void
AmDispatcher::updateCatalogOnceCb(AmRequest* amReq,
                              MultiPrimarySvcRequest* svcReq,
                              const Error& error,
                              boost::shared_ptr<std::string> payload) {
    // Ensure we haven't already replied to this request
    if (!amReq->isCompleted()) { return; }
    dmtMgr->releaseVersion(amReq->dmt_version);
    PerfTracer::tracePointEnd(amReq->dm_perf_ctx);
    auto updCatRsp = deserializeFdspMsg<fpi::UpdateCatalogOnceRspMsg>(const_cast<Error&>(error), payload);

    auto blobReq = static_cast<PutBlobReq *>(amReq);
    if (error != ERR_OK) {
        LOGERROR << "Obj ID: " << blobReq->obj_id
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
                              MultiPrimarySvcRequest* svcReq,
                              const Error& error,
                              boost::shared_ptr<std::string> payload) {
    // Ensure we haven't already replied to this request
    if (!amReq->isCompleted()) { return; }
    PerfTracer::tracePointEnd(amReq->dm_perf_ctx);
    auto updCatRsp = deserializeFdspMsg<fpi::UpdateCatalogRspMsg>(const_cast<Error&>(error), payload);

    auto blobReq = static_cast<PutBlobReq *>(amReq);
    if (error != ERR_OK) {
        LOGERROR << "Obj ID: " << blobReq->obj_id
                 << " blob name: " << amReq->getBlobName()
                 << " offset: " << amReq->blob_offset
                 << " Error: " << error;
    } else {
        LOGDEBUG << svcReq->logString() << fds::logString(*updCatRsp);
    }
    blobReq->notifyResponse(error);
}

void
AmDispatcher::dispatchPutObject(AmRequest *amReq) {
    auto blobReq = static_cast<PutBlobReq *>(amReq);
    fds_assert(amReq->data_len > 0);

    fiu_do_on("am.uturn.dispatcher",
              static_cast<PutBlobReq*>(amReq)->notifyResponse(ERR_OK); \
              return;);

    auto putObjMsg(boost::make_shared<fpi::PutObjectMsg>());
    putObjMsg->volume_id        = amReq->io_vol_id.get();
    putObjMsg->data_obj.assign(blobReq->dataPtr->c_str(), amReq->data_len);
    putObjMsg->data_obj_len     = amReq->data_len;
    putObjMsg->data_obj_id.digest = std::string(
        reinterpret_cast<const char*>(blobReq->obj_id.GetId()),
        blobReq->obj_id.GetLen());

    // get DLT and add refcnt to account for in-flight IO sent with
    // this DLT version; when DLT version changes, we don't ack to OM
    // until all in-flight IO for the previous version complete
    auto const dlt = dltMgr->getAndLockCurrentVersion();
    amReq->dlt_version = dlt->getVersion();
    auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::putObjectCb, amReq));
    auto asyncPutReq = createMultiPrimaryRequest(blobReq->obj_id,
                                                 dlt,
                                                 putObjMsg,
                                                 respCb,
                                                 message_timeout_io);

    PerfTracer::tracePointBegin(amReq->sm_perf_ctx);
    setSerialization(amReq, asyncPutReq);
    asyncPutReq->invoke();

    LOGDEBUG << asyncPutReq->logString() << logString(*putObjMsg)
             << " DLT version " << dlt->getVersion();
}

void
AmDispatcher::putObjectCb(AmRequest* amReq,
                          MultiPrimarySvcRequest* svcReq,
                          const Error& error,
                          boost::shared_ptr<std::string> payload) {
    // Ensure we haven't already replied to this request
    if (!amReq->isCompleted()) { return; }

    // notify DLT manager that request completed, so we can decrement refcnt
    dltMgr->releaseVersion(amReq->dlt_version);

    auto blobReq = static_cast<fds::PutBlobReq*>(amReq);
    PerfTracer::tracePointEnd(amReq->sm_perf_ctx);
    auto putObjRsp = deserializeFdspMsg<fpi::PutObjectRspMsg>(const_cast<Error&>(error), payload);

    if (error != ERR_OK) {
        LOGERROR << "Obj ID: " << blobReq->obj_id
            << " blob name: " << amReq->getBlobName()
            << " offset: " << amReq->blob_offset << " Error: " << error;
    } else {
        LOGDEBUG << svcReq->logString() << logString(*putObjRsp)
                 << " DLT version " << amReq->dlt_version;
    }

    blobReq->notifyResponse(error);
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
    // TODO(bszmyd): Thu 28 May 2015 09:14:07 PM MDT
    // This needs to handle multiple ids or be made into a sub-request
    auto blobReq = static_cast<GetObjectReq *>(amReq);
    ObjectID const& objId = *blobReq->obj_id;

    auto getObjMsg(boost::make_shared<fpi::GetObjectMsg>());
    getObjMsg->volume_id = volId.get();
    getObjMsg->data_obj_id.digest = std::string(
        reinterpret_cast<const char*>(objId.GetId()),
        objId.GetLen());

    // get DLT and add refcnt to account for in-flight IO sent with
    // this DLT version; when DLT version changes, we don't ack to OM
    // until all in-flight IO for the previous version complete
    auto const dlt = dltMgr->getAndLockCurrentVersion();
    amReq->dlt_version = dlt->getVersion();
    auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::getObjectCb, amReq));
    auto asyncGetReq = createFailoverRequest(objId,
                                             dlt,
                                             getObjMsg,
                                             respCb,
                                             message_timeout_io);

    PerfTracer::tracePointBegin(amReq->sm_perf_ctx);
    asyncGetReq->invoke();

    LOGDEBUG << asyncGetReq->logString() << logString(*getObjMsg);
}

void
AmDispatcher::getObjectCb(AmRequest* amReq,
                          FailoverSvcRequest* svcReq,
                          const Error& error,
                          boost::shared_ptr<std::string> payload)
{
    // Ensure we haven't already replied to this request
    if (!amReq->isCompleted()) { return; }
    PerfTracer::tracePointEnd(amReq->sm_perf_ctx);

    // notify DLT manager that request completed, so we can decrement refcnt
    dltMgr->releaseVersion(amReq->dlt_version);

    auto getObjRsp = deserializeFdspMsg<fpi::GetObjectResp>(const_cast<Error&>(error), payload);

    if (error == ERR_OK) {
        LOGDEBUG << svcReq->logString() << logString(*getObjRsp)
                 << " DLT version " << amReq->dlt_version;
        auto blobReq = static_cast<GetObjectReq*>(amReq);
        blobReq->obj_data = boost::make_shared<std::string>(std::move(getObjRsp->data_obj));
    } else {
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
    auto start_offset = amReq->blob_offset;
    auto end_offset = amReq->blob_offset_end;
    auto volId = amReq->io_vol_id;

    LOGDEBUG << "blob name: " << amReq->getBlobName()
             << " start offset: 0x" << std::hex << start_offset
             << " end offset: 0x" << end_offset
             << " volid: " << volId;
    /*
     * TODO(Andrew): We should eventually specify the offset in the blob
     * we want...all objects won't work well for large blobs.
     */
    auto queryMsg = boost::make_shared<fpi::QueryCatalogMsg>();
    queryMsg->volume_id    = volId.get();
    queryMsg->blob_name    = amReq->getBlobName();
    queryMsg->start_offset = start_offset;
    queryMsg->end_offset   = end_offset;
    // We don't currently specify a version
    queryMsg->blob_version = blob_version_invalid;
    queryMsg->obj_list.clear();
    queryMsg->meta_list.clear();

    amReq->dmt_version = dmtMgr->getAndLockCurrentVersion();
    auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::getQueryCatalogCb, amReq));
    auto asyncQueryReq = createFailoverRequest(amReq->io_vol_id,
                                               amReq->dmt_version,
                                               queryMsg,
                                               respCb,
                                               message_timeout_io);

    asyncQueryReq->onEPAppStatusCb(std::bind(&AmDispatcher::missingBlobStatusCb,
                                             this, amReq, std::placeholders::_1,
                                             std::placeholders::_2));
    asyncQueryReq->invoke();
    LOGDEBUG << asyncQueryReq->logString() << logString(*queryMsg);
}

fds_bool_t
AmDispatcher::missingBlobStatusCb(AmRequest* amReq,
                                  const Error& error,
                                  boost::shared_ptr<std::string> payload) {
    // Tell service layer that it's OK to see these errors. These
    // could mean we're just reading something we haven't written
    // before.
    if ((ERR_CAT_ENTRY_NOT_FOUND == error) ||
        (ERR_BLOB_NOT_FOUND == error) ||
        (ERR_BLOB_OFFSET_INVALID == error)) {
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
    // Ensure we haven't already replied to this request
    if (!amReq->isCompleted()) { return; }
    dmtMgr->releaseVersion(amReq->dmt_version);
    PerfTracer::tracePointEnd(amReq->dm_perf_ctx);

    Error err = ERR_OK;
    if (error != ERR_OK && error != ERR_BLOB_OFFSET_INVALID) {
        // TODO(Andrew): We should consider logging this error at a
        // higher level when the volume is not block
        LOGDEBUG << "blob name: " << amReq->getBlobName() << " offset: "
                 << amReq->blob_offset << " Error: " << error;
        err = (error == ERR_CAT_ENTRY_NOT_FOUND ? ERR_BLOB_NOT_FOUND : error);
    }

    auto qryCatRsp = deserializeFdspMsg<fpi::QueryCatalogMsg>(err, payload);

    if (err.ok()) {
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

        auto new_ids = std::vector<ObjectID::ptr>();

        for (fpi::FDSP_BlobObjectList::const_iterator it = qryCatRsp->obj_list.cbegin();
             it != qryCatRsp->obj_list.cend();
             ++it) {
            fds_uint64_t cur_offset = it->offset;
            if (cur_offset >= amReq->blob_offset || cur_offset <= amReq->blob_offset_end) {
                // found offset!!!
                // TODO(bszmyd): Thu 21 May 2015 12:36:15 PM MDT
                // Fix this when we support unaligned reads.
                // Number of objects required to request given data length
                auto objId = new ObjectID((*it).data_obj_id.digest);
                GLOGTRACE << "Found object id: " << *objId
                          << " for offset: 0x" << std::hex << cur_offset << std::dec;
                new_ids.emplace_back(objId);
            }
        }

        // TODO(bszmyd): Mon 23 Mar 2015 02:49:01 AM PDT
        // This is the matching error scenario from the trickery
        // in AmProcessor::getBlobCb due to the write/read race
        // between DM/SM. If this is a retry then the object id should be
        // anything but what it was...or we should be able to get the object
        if (blobReq->retry && !std::equal(new_ids.begin(),
                                          new_ids.end(),
                                          blobReq->object_ids.begin(),
                                          [](ObjectID::ptr const& a, ObjectID::ptr const& b)
                                            { return *a == *b; })) {
            blobReq->retry = false; // We've gotten a new Id we're not insane
        }
        blobReq->object_ids.swap(new_ids);
        blobReq->object_ids.shrink_to_fit();
    }
    amReq->proc_cb(err);
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
    message->volume_id = amReq->io_vol_id.get();
    message->blob_name = amReq->getBlobName();
    message->metaDataList.clear();

    amReq->dmt_version = dmtMgr->getAndLockCurrentVersion();
    auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::statBlobCb, amReq));
    auto asyncReq = createFailoverRequest(amReq->io_vol_id,
                                          amReq->dmt_version,
                                          message,
                                          respCb);
    asyncReq->onEPAppStatusCb(std::bind(&AmDispatcher::missingBlobStatusCb,
                                        this, amReq, std::placeholders::_1,
                                        std::placeholders::_2));
    asyncReq->invoke();
}

void
AmDispatcher::dispatchRenameBlob(AmRequest *amReq) {
    fiu_do_on("am.uturn.dispatcher",
              auto cb = SHARED_DYN_CAST(RenameBlobCallback, amReq->cb); \
              cb->blobDesc = boost::make_shared<BlobDescriptor>(); \
              cb->blobDesc->setBlobName(amReq->getBlobName()); \
              cb->blobDesc->setBlobSize(0); \
              amReq->proc_cb(ERR_OK); \
              return;);

    auto blobReq = static_cast<RenameBlobReq *>(amReq);
    blobReq->dmt_version = dmtMgr->getAndLockCurrentVersion();

    auto message = boost::make_shared<fpi::RenameBlobMsg>();
    message->volume_id = amReq->io_vol_id.get();
    message->source_blob = amReq->getBlobName();
    message->destination_blob = blobReq->new_blob_name;
    message->source_tx_id = blobReq->tx_desc->getValue();
    message->destination_tx_id = blobReq->dest_tx_desc->getValue();
    message->dmt_version  = blobReq->dmt_version;
    message->sequence_id  = blobReq->vol_sequence;

    auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::renameBlobCb, amReq));
    auto asyncReq = createMultiPrimaryRequest(amReq->io_vol_id,
                                              blobReq->dmt_version,
                                              message,
                                              respCb);
    asyncReq->onEPAppStatusCb(std::bind(&AmDispatcher::missingBlobStatusCb,
                                        this, amReq, std::placeholders::_1,
                                        std::placeholders::_2));
    asyncReq->invoke();
}

void
AmDispatcher::renameBlobCb(AmRequest *amReq,
                           MultiPrimarySvcRequest* svcReq,
                           const Error& error,
                           boost::shared_ptr<std::string> payload) {
    // Ensure we haven't already replied to this request
    if (!amReq->isCompleted()) { return; }
    dmtMgr->releaseVersion(amReq->dmt_version);
    // Deserialize if all OK
    if (ERR_OK == error) {
        auto blobReq = static_cast<RenameBlobReq *>(amReq);
        // using the same structure for input and output
        auto response = MSG_DESERIALIZE(RenameBlobRespMsg, error, payload);

        auto cb = SHARED_DYN_CAST(RenameBlobCallback, amReq->cb);
        // Fill in the data here
        cb->blobDesc = boost::make_shared<BlobDescriptor>();
        cb->blobDesc->setBlobName(blobReq->new_blob_name);
        cb->blobDesc->setBlobSize(response->byteCount);
        for (const auto& meta : response->metaDataList) {
            cb->blobDesc->addKvMeta(meta.key,  meta.value);
        }
    }
    amReq->proc_cb(error);
}

void
AmDispatcher::dispatchSetBlobMetadata(AmRequest *amReq) {
    auto vol_id = amReq->io_vol_id;

    auto blobReq = static_cast<SetBlobMetaDataReq *>(amReq);
    auto setMDMsg = boost::make_shared<fpi::SetBlobMetaDataMsg>();
    setMDMsg->blob_name = amReq->getBlobName();
    setMDMsg->blob_version = blob_version_invalid;
    setMDMsg->volume_id = vol_id.get();
    setMDMsg->txId = blobReq->tx_desc->getValue();

    setMDMsg->metaDataList = std::move(*blobReq->getMetaDataListPtr());

    // Create callback
    auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::setBlobMetadataCb, amReq));
    auto asyncSetMDReq = createMultiPrimaryRequest(vol_id,
                                                   blobReq->dmt_version,
                                                   setMDMsg,
                                                   respCb);
    setSerialization(amReq, asyncSetMDReq);
    asyncSetMDReq->invoke();
}

void
AmDispatcher::setBlobMetadataCb(AmRequest *amReq,
                                MultiPrimarySvcRequest *svcReq,
                                const Error &error,
                                boost::shared_ptr<std::string> payload) {
    // Ensure we haven't already replied to this request
    if (!amReq->isCompleted()) { return; }
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
    // Ensure we haven't already replied to this request
    if (!amReq->isCompleted()) { return; }
    dmtMgr->releaseVersion(amReq->dmt_version);
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
    auto blobReq = static_cast<CommitBlobTxReq *>(amReq);
    // Create network message
    auto commitBlobTxMsg = boost::make_shared<fpi::CommitBlobTxMsg>();
    commitBlobTxMsg->blob_name    = amReq->getBlobName();
    commitBlobTxMsg->blob_version = blob_version_invalid;
    commitBlobTxMsg->volume_id    = amReq->io_vol_id.get();
    commitBlobTxMsg->txId         = blobReq->tx_desc->getValue();
    commitBlobTxMsg->dmt_version  = blobReq->dmt_version;
    commitBlobTxMsg->sequence_id  = blobReq->vol_sequence;

    // Create callback
    auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::commitBlobTxCb, amReq));
    auto asyncCommitBlobTxReq = createMultiPrimaryRequest(amReq->io_vol_id,
                                                          blobReq->dmt_version,
                                                          commitBlobTxMsg,
                                                          respCb);
    setSerialization(amReq, asyncCommitBlobTxReq);
    asyncCommitBlobTxReq->invoke();

    LOGDEBUG << asyncCommitBlobTxReq->logString()
             << logString(*commitBlobTxMsg);
}

void
AmDispatcher::commitBlobTxCb(AmRequest *amReq,
                            MultiPrimarySvcRequest *svcReq,
                            const Error &error,
                            boost::shared_ptr<std::string> payload) {
    // Ensure we haven't already replied to this request
    if (!amReq->isCompleted()) { return; }
    dmtMgr->releaseVersion(amReq->dmt_version);
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
    message->volume_id = amReq->io_vol_id.get();
    message->startPos  = static_cast<VolumeContentsReq *>(amReq)->offset;
    message->count   = static_cast<VolumeContentsReq *>(amReq)->count;
    message->pattern = static_cast<VolumeContentsReq *>(amReq)->pattern;
    message->orderBy = static_cast<VolumeContentsReq *>(amReq)->orderBy;
    message->descending = static_cast<VolumeContentsReq *>(amReq)->descending;

    amReq->dmt_version = dmtMgr->getAndLockCurrentVersion();
    auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::volumeContentsCb, amReq));
    auto asyncReq = createFailoverRequest(amReq->io_vol_id,
                                          amReq->dmt_version,
                                          message,
                                          respCb);
    asyncReq->invoke();
}

void
AmDispatcher::volumeContentsCb(AmRequest* amReq,
                               FailoverSvcRequest* svcReq,
                               const Error& error,
                               boost::shared_ptr<std::string> payload)
{
    // Ensure we haven't already replied to this request
    if (!amReq->isCompleted()) { return; }
    dmtMgr->releaseVersion(amReq->dmt_version);
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
