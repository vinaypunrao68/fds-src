/*
 * Copyright 2014-2016 Formation Data Systems, Inc.
 */

#include <algorithm>
#include <vector>

#include "fds_process.h"
#include "fdsp/dm_api_types.h"
#include "fdsp/sm_api_types.h"

#include <AmDispatcher.h>
#include <net/SvcRequestPool.h>
#include <net/SvcMgr.h>
#include <fiu-control.h>
#include <util/fiu_util.h>

#include "AmAsyncDataApi.h"
#include "requests/requests.h"
#include "requests/GetObjectReq.h"
#include <net/MockSvcHandler.h>
#include "net/VolumeGroupHandle.h"
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

struct ErrorHandler : public VolumeGroupHandleListener {
    bool isError(fpi::FDSPMsgTypeId const& msgType, Error const& e) override;
};

AmDispatcher::AmDispatcher(AmDataProvider* prev, CommonModuleProviderIf *modProvider)
        : HasModuleProvider(modProvider),
          AmDataProvider(prev, nullptr)
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
        message_timeout_open = conf.get_abs<fds_uint32_t>("fds.am.svc.timeout.open_message", message_timeout_open);
        message_timeout_default = conf.get_abs<fds_uint32_t>("fds.am.svc.timeout.thrift_message", message_timeout_default);
        LOGNOTIFY << "AM Thrift timeout: " << message_timeout_default << " ms"
                  << " IO timeout: " << message_timeout_io  << " ms";
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
    FdsConfigAccessor ft_conf(g_fdsprocess->get_fds_config(), "fds.feature_toggle.");
    int serialSelection;
    auto serializationString = ft_conf.get<std::string>("am.serialization", DefaultSerialization);

    // This determines if we can or cannot dispatch updatecatlog and putobject
    // concurrently.
    safe_atomic_write = ft_conf.get<bool>("am.safe_atomic_write", safe_atomic_write);
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

    /**
     * FEATURE TOGGLE: Enable/Disable volume grouping support
     * Thu Jan 14 10:45:14 2016
     */
    volume_grouping_support = ft_conf.get<bool>("common.enable_volumegrouping", volume_grouping_support);
    if (volume_grouping_support && !volumegroup_handler) {
        volumegroup_handler.reset(new ErrorHandler());
    }
}

bool
AmDispatcher::done() {
    std::lock_guard<std::mutex> g(tx_map_lock);
    if (!tx_map_barrier.empty()) {
        return false;
    }
    return AmDataProvider::done();
}

void
AmDispatcher::stop() {
    if (StatsCollector::singleton()->isStreaming()) {
        StatsCollector::singleton()->stopStreaming();
    }
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
AmDispatcher::updateDmt(bool dmt_type, std::string& dmt_data, FDS_Table::callback_type const& cb) {
    auto err = MODULEPROVIDER()->getSvcMgr()->updateDmt(dmt_type, dmt_data, cb);
    if (ERR_DUPLICATE == err) {
        LOGWARN << "Received duplicate DMT version, ignoring";
        err = ERR_OK;
    }
    return err;
}

Error
AmDispatcher::getDMT() {
	if (noNetwork) {
		// Standalone mode
		return (ERR_OK);
	} else {
		return (MODULEPROVIDER()->getSvcMgr()->getDMT());
	}
}

Error
AmDispatcher::getDLT() {
	if (noNetwork) {
		// Standalone mode
		return (ERR_OK);
	} else {
		return (MODULEPROVIDER()->getSvcMgr()->getDLT());
	}
}

void
AmDispatcher::registerVolume(VolumeDesc const& volDesc) {
    auto& vol_id = volDesc.volUUID;
    if (volume_grouping_support) {
        WriteGuard wg(volumegroup_lock);
        auto it = volumegroup_map.find(vol_id);
        if (volumegroup_map.end() != it) {
            return;
        }
        volumegroup_map[vol_id].reset(new VolumeGroupHandle(MODULEPROVIDER(), vol_id, DmDefaultPrimaryCnt));
        volumegroup_map[vol_id]->setListener(volumegroup_handler.get());
    }
}

void
AmDispatcher::removeVolume(VolumeDesc const& volDesc) {
    auto& vol_id = volDesc.volUUID;
    /**
     * FEATURE TOGGLE: Enable/Disable volume grouping support
     * Thu Jan 14 10:45:14 2016
     */
    if (volume_grouping_support) {
        WriteGuard wg(volumegroup_lock);
        auto it = volumegroup_map.find(vol_id);
        // Give ownership of the volume handle to the handle itself, we're
        // done with it
        std::shared_ptr<VolumeGroupHandle> vg = std::move(it->second);
        vg->close([this, vg] () mutable -> void { });
    }
    // We need to remove any barriers for this volume as we don't expect to
    // see any aborts/commits for them now
    std::lock_guard<std::mutex> g(tx_map_lock);
    for (auto it = tx_map_barrier.begin(); tx_map_barrier.end() != it; ) {
        if (it->first.first == volDesc.volUUID) {
            // Drain as errors
            auto queue = std::move(std::get<2>(it->second));
            it = tx_map_barrier.erase(it);
            for (auto& req : queue) {
                AmDataProvider::startBlobTxCb(req, ERR_VOLUME_ACCESS_DENIED);
            }
        } else {
            ++it;
        }
    }
}

void
AmDispatcher::lookupVolume(std::string const volume_name) {
    if (!noNetwork) {
        // We need valid DLT and DMTs before we can start issuing IO,
        // block attachments here until this is true.
        if (DMT_VER_INVALID == dmtMgr->getCommittedVersion() ||
            nullptr == dltMgr->getDLT()) {
            LOGWARN << "Could not attach to volume before receiving domain tables.";
            VolumeDesc dummy(volume_name, invalid_vol_id);
            return AmDataProvider::lookupVolumeCb(dummy, ERR_NOT_READY);
        }
        try {
            auto req =  gSvcRequestPool->newEPSvcRequest(MODULEPROVIDER()->getSvcMgr()->getOmSvcUuid());
            fpi::GetVolumeDescriptorPtr msg(new fpi::GetVolumeDescriptor());
            msg->volume_name = volume_name;
            req->setPayload(FDSP_MSG_TYPEID(fpi::GetVolumeDescriptor), msg);
            req->onResponseCb(RESPONSE_MSG_HANDLER(AmDispatcher::lookupVolumeCb, volume_name));
            req->invoke();
            LOGNOTIFY << " retrieving volume descriptor from OM for " << volume_name;
        } catch(...) {
            LOGERROR << "OMClient unable to request volume descriptor from OM. Check if OM is up and restart.";
            VolumeDesc dummy(volume_name, invalid_vol_id);
            return AmDataProvider::lookupVolumeCb(dummy, ERR_NOT_READY);
        }
    }
}

void
AmDispatcher::lookupVolumeCb(std::string const& volume_name,
                             EPSvcRequest* svcReq,
                             const Error& error,
                             shared_str payload) {
    // Deserialize if all OK
    VolumeDesc desc(volume_name, invalid_vol_id);
    if (ERR_OK == error) {
        // using the same structure for input and output
        auto response = MSG_DESERIALIZE(GetVolumeDescriptorResp, error, payload);
        desc = VolumeDesc(response->vol_desc);
    }
    AmDataProvider::lookupVolumeCb(desc, error);
}

void
AmDispatcher::getVolumes(std::vector<VolumeDesc>& volumes) {
    volumes.clear();
    Error err(ERR_OK);
    fpi::GetAllVolumeDescriptors list;
    err = MODULEPROVIDER()->getSvcMgr()->getAllVolumeDescriptors(list);
    if (err.ok()) {
        for (auto const& volume : list.volumeList) {
            if (fpi::Active == volume.vol_desc.state) {
                volumes.emplace_back(volume.vol_desc);
            }
        }
    }
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
    auto dm_group = dmtMgr->getVersionNodeGroup(volId, dmt_ver);

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

    auto multiReq = gSvcRequestPool->newMultiPrimarySvcRequest(primaries, secondaries, dmt_ver);
    // TODO(bszmyd): Mon 22 Jun 2015 12:08:25 PM MDT
    // Need to also set a onAllRespondedCb
    multiReq->onPrimariesRespondedCb(cb);
    multiReq->setTimeoutMs((0 < timeout) ? timeout : message_timeout_default);
    multiReq->setPayload(message_type_id(*payload), payload);
    return multiReq;
}

/**
 * MultiPrimary request with object id will always be sent to Storage Manager.
 * Look at the dmt to find possible destination SMs.
 */
template<typename Msg>
QuorumSvcRequestPtr
AmDispatcher::createQuorumRequest(ObjectID const& objId,
                                  DLT const* dlt,
                                  boost::shared_ptr<Msg> const& payload,
                                  QuorumSvcRequestRespCb cb,
                                  uint32_t timeout) const {
    auto token_group = boost::make_shared<DltObjectIdEpProvider>(dlt->getNodes(objId));
    auto num_nodes = token_group->getEps().size();
    auto dlt_version = dlt->getVersion();

    auto quorumReq = gSvcRequestPool->newQuorumSvcRequest(token_group, dlt_version);
    quorumReq->setTimeoutMs((0 < timeout) ? timeout : message_timeout_default);
    quorumReq->setPayload(message_type_id(*payload), payload);
    quorumReq->onResponseCb(cb);
    quorumReq->setQuorumCnt(std::min(num_nodes, 2ul));
    return quorumReq;
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
    auto dm_group = dmtMgr->getVersionNodeGroup(volId, dmt_ver);
    auto numNodes = std::min(dm_group->getLength(), numPrimaries);
    DmtColumnPtr dmPrimariesForVol = boost::make_shared<DmtColumn>(numNodes);
    for (uint32_t i = 0; numNodes > i; ++i) {
        dmPrimariesForVol->set(i, dm_group->get(i));
    }
    auto primary = boost::make_shared<DmtVolumeIdEpProvider>(dmPrimariesForVol);
    auto failoverReq = gSvcRequestPool->newFailoverSvcRequest(primary, dmt_ver);
    failoverReq->onResponseCb(cb);
    failoverReq->setTimeoutMs((0 < timeout) ? timeout : message_timeout_default);
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
    auto numNodes = token_group->getLength();
    auto nodes = boost::make_shared<DltTokenGroup>(numNodes);
    for (uint32_t i = 0; numNodes > i; ++i) {
       nodes->set(i, token_group->get(i));
    }
    auto provider = boost::make_shared<DltObjectIdEpProvider>(nodes);
    auto failoverReq = gSvcRequestPool->newFailoverSvcRequest(provider, dlt_version);
    failoverReq->onResponseCb(cb);
    failoverReq->setTimeoutMs((0 < timeout) ? timeout : message_timeout_default);
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
 * Dispatcher Requests
 * The following are the asynchronous requests Dispatchers makes to other
 * services.
 */

void
AmDispatcher::abortBlobTx(AmRequest* amReq) {
    fiu_do_on("am.uturn.dispatcher", return AmDataProvider::abortBlobTxCb(amReq, ERR_OK););

    fds_volid_t volId = amReq->io_vol_id;

    auto blobReq = static_cast<AbortBlobTxReq*>(amReq);
    auto message = boost::make_shared<fpi::AbortBlobTxMsg>();
    message->blob_name      = amReq->getBlobName();
    message->blob_version   = blob_version_invalid;
    message->volume_id      = volId.get();
    message->txId           = blobReq->tx_desc->getValue();

    /**
     * FEATURE TOGGLE: VolumeGrouping
     * Thu Jan 14 10:39:09 2016
     */
    if (!volume_grouping_support) {
        auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::abortBlobTxCb, blobReq));
        auto asyncAbortBlobTxReq = createMultiPrimaryRequest(volId,
                                                             blobReq->dmt_version,
                                                             message,
                                                             respCb);
        setSerialization(amReq, asyncAbortBlobTxReq);
        asyncAbortBlobTxReq->invoke();
        return;
    } else {
        ReadGuard rg(volumegroup_lock);
        auto it = volumegroup_map.find(amReq->io_vol_id);
        if (volumegroup_map.end() != it) {
            it->second->sendCommitMsg(message_type_id(*message),
                                      message,
                                      [blobReq, this] (Error const& e, shared_str p) mutable -> void {
                                          _abortBlobTxCb(blobReq, e, p);
                                      });
            return;
        }
    }
    LOGERROR << "Unknown volume to AmDispatcher: " << amReq->io_vol_id;
    AmDataProvider::abortBlobTxCb(amReq, ERR_VOLUME_ACCESS_DENIED);
}

/**
 * Dispatch a request to DM asking for permission to access this volume.
 */
void
AmDispatcher::closeVolume(AmRequest * amReq) {
    fiu_do_on("am.uturn.dispatcher", return AmDataProvider::closeVolumeCb(amReq, ERR_OK););

    LOGDEBUG << "Attempting to close volume: " << amReq->io_vol_id;
    auto volReq = static_cast<DetachVolumeReq*>(amReq);
    /**
     * FEATURE TOGGLE: VolumeGrouping
     * Thu Jan 14 10:39:09 2016
     */
    if (!volume_grouping_support) {
        auto volMDMsg = boost::make_shared<fpi::CloseVolumeMsg>();
        volMDMsg->volume_id = amReq->io_vol_id.get();
        volMDMsg->token = volReq->token;

        amReq->dmt_version = dmtMgr->getAndLockCurrentVersion();
        auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::closeVolumeCb, volReq));
        auto asyncCloseVolReq = createMultiPrimaryRequest(amReq->io_vol_id,
                                                          amReq->dmt_version,
                                                          volMDMsg,
                                                          respCb);

        setSerialization(amReq, asyncCloseVolReq);
        asyncCloseVolReq->invoke();
        return;
    } else {
        WriteGuard wg(volumegroup_lock);
        auto it = volumegroup_map.find(amReq->io_vol_id);
        if (volumegroup_map.end() != it) {
            // Give ownership of the volume handle to the handle itself, we're
            // done with it
            std::shared_ptr<VolumeGroupHandle> vg = std::move(it->second);
            vg->close([this, volReq, vg] () mutable -> void {
                            _closeVolumeCb(volReq);
                        });
            return;
        }
    }
    return AmDataProvider::closeVolumeCb(amReq, ERR_OK);
}

void
AmDispatcher::commitBlobTx(AmRequest* amReq) {
    fiu_do_on("am.uturn.dispatcher", return AmDataProvider::commitBlobTxCb(amReq, ERR_OK););
    auto blobReq = static_cast<CommitBlobTxReq *>(amReq);
    // Create network message
    auto message = boost::make_shared<fpi::CommitBlobTxMsg>();
    message->blob_name    = amReq->getBlobName();
    message->blob_version = blob_version_invalid;
    message->volume_id    = amReq->io_vol_id.get();
    message->txId         = blobReq->tx_desc->getValue();

    /**
     * FEATURE TOGGLE: VolumeGrouping
     * Thu Jan 14 10:39:09 2016
     */
    {
        auto vLock = dispatchTable.getAndLockVolumeSequence(amReq->io_vol_id, message->sequence_id);
        if (!volume_grouping_support) {
            // Create callback
            message->dmt_version  = blobReq->dmt_version;
            auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::commitBlobTxCb, blobReq));
            auto asyncCommitBlobTxReq = createMultiPrimaryRequest(amReq->io_vol_id,
                                                                  blobReq->dmt_version,
                                                                  message,
                                                                  respCb);
            setSerialization(amReq, asyncCommitBlobTxReq);
            asyncCommitBlobTxReq->invoke();
            return;
        } else {
            ReadGuard rg(volumegroup_lock);
            auto it = volumegroup_map.find(amReq->io_vol_id);
            if (volumegroup_map.end() != it) {
                it->second->sendCommitMsg(message_type_id(*message),
                                          message,
                                          [blobReq, this] (Error const& e, shared_str p) mutable -> void {
                                              _commitBlobTxCb(blobReq, e, p);
                                          });
                return;
            }
        }
    }
    LOGERROR << "Unknown volume to AmDispatcher: " << amReq->io_vol_id;
    AmDataProvider::commitBlobTxCb(amReq, ERR_VOLUME_ACCESS_DENIED);
}

void
AmDispatcher::deleteBlob(AmRequest* amReq)
{
    fiu_do_on("am.uturn.dispatcher", return AmDataProvider::deleteBlobCb(amReq, ERR_OK););
    auto blobReq = static_cast<DeleteBlobReq *>(amReq);
    auto message = boost::make_shared<fpi::DeleteBlobMsg>();
    message->volume_id = amReq->io_vol_id.get();
    message->blob_name = amReq->getBlobName();
    message->blob_version = blob_version_invalid;
    message->txId = blobReq->tx_desc->getValue();

    /**
     * FEATURE TOGGLE: VolumeGrouping
     * Thu Jan 14 10:39:09 2016
     */
    if (!volume_grouping_support) {
        auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::deleteBlobCb, blobReq));
        auto asyncReq = createMultiPrimaryRequest(amReq->io_vol_id, blobReq->dmt_version, message, respCb);
        asyncReq->onEPAppStatusCb(std::bind(&AmDispatcher::missingBlobStatusCb,
                                            this, amReq, std::placeholders::_1,
                                            std::placeholders::_2));
        setSerialization(amReq, asyncReq);
        asyncReq->invoke();
        return;
    } else {
        ReadGuard rg(volumegroup_lock);
        auto it = volumegroup_map.find(amReq->io_vol_id);
        if (volumegroup_map.end() != it) {
            it->second->sendModifyMsg(message_type_id(*message),
                                      message,
                                      [blobReq, this] (Error const& e, shared_str p) mutable -> void {
                                          _deleteBlobCb(blobReq, e, p);
                                      });
            return;
        }
    }
    LOGERROR << "Unknown volume to AmDispatcher: " << amReq->io_vol_id;
    AmDataProvider::deleteBlobCb(amReq, ERR_VOLUME_ACCESS_DENIED);
}

void
AmDispatcher::getOffsets(AmRequest* amReq) {
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
    auto message = boost::make_shared<fpi::QueryCatalogMsg>();
    message->volume_id    = volId.get();
    message->blob_name    = amReq->getBlobName();
    message->start_offset = start_offset;
    message->end_offset   = end_offset;
    // We don't currently specify a version
    message->blob_version = blob_version_invalid;
    message->obj_list.clear();
    message->meta_list.clear();

    auto blobReq = static_cast<GetBlobReq*>(amReq);
    /**
     * FEATURE TOGGLE: VolumeGrouping
     * Thu Jan 14 10:39:09 2016
     */
    if (!volume_grouping_support) {
        amReq->dmt_version = dmtMgr->getAndLockCurrentVersion();
        auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::getQueryCatalogCb, blobReq));
        auto asyncQueryReq = createFailoverRequest(amReq->io_vol_id,
                                                   amReq->dmt_version,
                                                   message,
                                                   respCb,
                                                   message_timeout_io);

        asyncQueryReq->onEPAppStatusCb(std::bind(&AmDispatcher::missingBlobStatusCb,
                                                 this, amReq, std::placeholders::_1,
                                                 std::placeholders::_2));
        asyncQueryReq->invoke();
        return;
    } else {
        ReadGuard rg(volumegroup_lock);
        auto it = volumegroup_map.find(amReq->io_vol_id);
        if (volumegroup_map.end() != it) {
            it->second->sendReadMsg(message_type_id(*message),
                                    message,
                                    [blobReq, this] (Error const& e, shared_str p) mutable -> void {
                                        _getQueryCatalogCb(blobReq, e, p);
                                    });
            return;
        }
    }
    LOGERROR << "Unknown volume to AmDispatcher: " << amReq->io_vol_id;
    AmDataProvider::getOffsetsCb(amReq, ERR_VOLUME_ACCESS_DENIED);
}

void
AmDispatcher::getVolumeMetadata(AmRequest* amReq) {
    fiu_do_on("am.uturn.dispatcher", return AmDataProvider::getVolumeMetadataCb(amReq, ERR_OK););

    auto message = boost::make_shared<fpi::GetVolumeMetadataMsg>();
    message->volumeId = amReq->io_vol_id.get();
    auto volReq = static_cast<GetVolumeMetadataReq*>(amReq);

    /**
     * FEATURE TOGGLE: VolumeGrouping
     * Thu Jan 14 10:39:09 2016
     */
    if (!volume_grouping_support) {
        amReq->dmt_version = dmtMgr->getAndLockCurrentVersion();
        auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::getVolumeMetadataCb, volReq));
        auto asyncGetVolMetadataReq = createFailoverRequest(amReq->io_vol_id,
                                                            amReq->dmt_version,
                                                            message,
                                                            respCb);
        asyncGetVolMetadataReq->invoke();
        return;
    } else {
        ReadGuard rg(volumegroup_lock);
        auto it = volumegroup_map.find(amReq->io_vol_id);
        if (volumegroup_map.end() != it) {
            it->second->sendReadMsg(message_type_id(*message),
                                    message,
                                    [volReq, this] (Error const& e, shared_str p) mutable -> void {
                                        _getVolumeMetadataCb(volReq, e, p);
                                    });
            return;
        }
    }
    LOGERROR << "Unknown volume to AmDispatcher: " << amReq->io_vol_id;
    AmDataProvider::getVolumeMetadataCb(amReq, ERR_VOLUME_ACCESS_DENIED);
}

/**
 * Dispatch a request to DM asking for permission to access this volume.
 */
void
AmDispatcher::openVolume(AmRequest* amReq) {
    fiu_do_on("am.uturn.dispatcher", return AmDataProvider::openVolumeCb(amReq, ERR_OK););
    auto volReq = static_cast<fds::AttachVolumeReq*>(amReq);

    LOGDEBUG << "Attempting to open volume: " << std::hex << amReq->io_vol_id
             << " with token: " << volReq->token;

    auto volMDMsg = boost::make_shared<fpi::OpenVolumeMsg>();
    volMDMsg->volume_id = amReq->io_vol_id.get();
    volMDMsg->token = volReq->token;
    volMDMsg->mode = volReq->mode;

    /**
     * FEATURE TOGGLE: VolumeGrouping
     * Thu Jan 14 10:39:09 2016
     */
    if (!volume_grouping_support) {
        volReq->dmt_version = dmtMgr->getAndLockCurrentVersion();
        auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::openVolumeCb, volReq));
        auto asyncOpenVolReq = createMultiPrimaryRequest(amReq->io_vol_id,
                                                         volReq->dmt_version,
                                                         volMDMsg,
                                                         respCb,
                                                         message_timeout_open);
        setSerialization(amReq, asyncOpenVolReq);
        asyncOpenVolReq->invoke();
        return;
    } else {
        ReadGuard rg(volumegroup_lock);
        auto it = volumegroup_map.find(amReq->io_vol_id);
        if (volumegroup_map.end() != it) {
            it->second->open(volMDMsg,
                             [volReq, this] (Error const& e, fpi::OpenVolumeRspMsgPtr const& p) mutable -> void {
                                _openVolumeCb(volReq, e, p);
                                });
            return;
        }
    }
    LOGERROR << "Unknown volume to AmDispatcher: " << amReq->io_vol_id;
    AmDataProvider::openVolumeCb(amReq, ERR_VOLUME_ACCESS_DENIED);
}

void
AmDispatcher::putBlob(AmRequest* amReq) {
    auto blobReq = static_cast<PutBlobReq *>(amReq);
    fiu_do_on("am.uturn.dispatcher", return AmDataProvider::putBlobCb(amReq, ERR_OK););

    // We can safely dispatch to SM before DM using transactions
    putObject(amReq);

    auto message(boost::make_shared<fpi::UpdateCatalogMsg>());
    message->blob_name    = amReq->getBlobName();
    message->blob_version = blob_version_invalid;
    message->volume_id    = amReq->io_vol_id.get();
    message->txId         = blobReq->tx_desc->getValue();

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
    message->obj_list.push_back(updBlobInfo);

    fds::PerfTracer::tracePointBegin(amReq->dm_perf_ctx);

    /**
     * FEATURE TOGGLE: VolumeGrouping
     * Thu Jan 14 10:39:09 2016
     */
    if (!volume_grouping_support) {
        auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::putBlobCb, blobReq));
        auto asyncUpdateCatReq = createMultiPrimaryRequest(amReq->io_vol_id,
                                                           blobReq->dmt_version,
                                                           message,
                                                           respCb,
                                                           message_timeout_io);

        setSerialization(amReq, asyncUpdateCatReq);
        asyncUpdateCatReq->invoke();
        return;
    } else {
        ReadGuard rg(volumegroup_lock);
        auto it = volumegroup_map.find(amReq->io_vol_id);
        if (volumegroup_map.end() != it) {
            it->second->sendModifyMsg(message_type_id(*message),
                                      message,
                                      [blobReq, this] (Error const& e, shared_str p) mutable -> void {
                                          _putBlobCb(blobReq, e, p);
                                      });
            return;
        }
    }
    LOGERROR << "Unknown volume to AmDispatcher: " << amReq->io_vol_id;
    AmDataProvider::putBlobCb(amReq, ERR_VOLUME_ACCESS_DENIED);
}

void
AmDispatcher::putBlobOnce(AmRequest* amReq) {
    auto blobReq = static_cast<PutBlobReq *>(amReq);
    fiu_do_on("am.uturn.dispatcher", return AmDataProvider::putBlobOnceCb(amReq, ERR_OK););

    if (!safe_atomic_write) {
        putObject(amReq);
    }


    auto message(boost::make_shared<fpi::UpdateCatalogOnceMsg>());
    message->blob_name    = amReq->getBlobName();
    message->blob_version = blob_version_invalid;
    message->volume_id    = amReq->io_vol_id.get();
    message->txId         = blobReq->tx_desc->getValue();
    message->blob_mode    = blobReq->blob_mode;

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
    message->obj_list.push_back(updBlobInfo);

    // Setup blob metadata updates
    FDS_ProtocolInterface::FDSP_MetaDataPair metaDataPair;
    for (auto& meta : *(blobReq->metadata)) {
        metaDataPair.key   = meta.first;
        metaDataPair.value = meta.second;
        message->meta_list.push_back(metaDataPair);
    }

    PerfTracer::tracePointBegin(amReq->dm_perf_ctx);

    /**
     * FEATURE TOGGLE: VolumeGrouping
     * Thu Jan 14 10:39:09 2016
     */
    {
        auto vLock = dispatchTable.getAndLockVolumeSequence(amReq->io_vol_id, message->sequence_id);
        if (!volume_grouping_support) {
            blobReq->dmt_version = dmtMgr->getAndLockCurrentVersion();
            message->dmt_version  = blobReq->dmt_version;
            auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::putBlobOnceCb, blobReq));
            // Always use the current DMT version since we're updating in a single request
            auto asyncUpdateCatReq = createMultiPrimaryRequest(amReq->io_vol_id,
                                                               blobReq->dmt_version,
                                                               message,
                                                               respCb,
                                                               message_timeout_io);

            setSerialization(amReq, asyncUpdateCatReq);
            asyncUpdateCatReq->invoke();
            return;
        } else {
            ReadGuard rg(volumegroup_lock);
            auto it = volumegroup_map.find(amReq->io_vol_id);
            if (volumegroup_map.end() != it) {
                it->second->sendCommitMsg(message_type_id(*message),
                                          message,
                                          [blobReq, this] (Error const& e, shared_str p) mutable -> void {
                                              _putBlobOnceCb(blobReq, e, p);
                                          });
                return;
            }
        }
    }
    LOGERROR << "Unknown volume to AmDispatcher: " << amReq->io_vol_id;
    AmDataProvider::putBlobOnceCb(amReq, ERR_VOLUME_ACCESS_DENIED);
}

void
AmDispatcher::renameBlob(AmRequest* amReq) {
    fiu_do_on("am.uturn.dispatcher",
              auto cb = std::dynamic_pointer_cast<RenameBlobCallback>(amReq->cb); \
              cb->blobDesc = boost::make_shared<BlobDescriptor>(); \
              cb->blobDesc->setBlobName(amReq->getBlobName()); \
              cb->blobDesc->setBlobSize(0); \
              return AmDataProvider::renameBlobCb(amReq, ERR_OK););

    auto blobReq = static_cast<RenameBlobReq *>(amReq);

    auto message = boost::make_shared<fpi::RenameBlobMsg>();
    message->volume_id = amReq->io_vol_id.get();
    message->source_blob = amReq->getBlobName();
    message->destination_blob = blobReq->new_blob_name;
    message->source_tx_id = blobReq->tx_desc->getValue();
    message->destination_tx_id = blobReq->dest_tx_desc->getValue();

    /**
     * FEATURE TOGGLE: VolumeGrouping
     * Thu Jan 14 10:39:09 2016
     */
    {
        auto vLock = dispatchTable.getAndLockVolumeSequence(amReq->io_vol_id, message->sequence_id);
        if (!volume_grouping_support) {
            blobReq->dmt_version = dmtMgr->getAndLockCurrentVersion();
            message->dmt_version  = blobReq->dmt_version;
            auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::renameBlobCb, blobReq));
            auto asyncReq = createMultiPrimaryRequest(amReq->io_vol_id,
                                                      blobReq->dmt_version,
                                                      message,
                                                      respCb);
            asyncReq->onEPAppStatusCb(std::bind(&AmDispatcher::missingBlobStatusCb,
                                                this, amReq, std::placeholders::_1,
                                                std::placeholders::_2));
            setSerialization(amReq, asyncReq);
            asyncReq->invoke();
            return;
        } else {
            ReadGuard rg(volumegroup_lock);
            auto it = volumegroup_map.find(amReq->io_vol_id);
            if (volumegroup_map.end() != it) {
                it->second->sendCommitMsg(message_type_id(*message),
                                          message,
                                          [blobReq, this] (Error const& e, shared_str p) mutable -> void {
                                          _renameBlobCb(blobReq, e, p);
                                          });
                return;
            }
        }
    }
    LOGERROR << "Unknown volume to AmDispatcher: " << amReq->io_vol_id;
    AmDataProvider::renameBlobCb(amReq, ERR_VOLUME_ACCESS_DENIED);
}

void
AmDispatcher::setBlobMetadata(AmRequest* amReq) {
    auto vol_id = amReq->io_vol_id;

    auto blobReq = static_cast<SetBlobMetaDataReq *>(amReq);
    auto message = boost::make_shared<fpi::SetBlobMetaDataMsg>();
    message->blob_name = amReq->getBlobName();
    message->blob_version = blob_version_invalid;
    message->volume_id = vol_id.get();
    message->txId = blobReq->tx_desc->getValue();

    message->metaDataList = std::move(*blobReq->getMetaDataListPtr());

    /**
     * FEATURE TOGGLE: VolumeGrouping
     * Thu Jan 14 10:39:09 2016
     */
    if (!volume_grouping_support) {
        auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::setBlobMetadataCb, blobReq));
        auto asyncSetMDReq = createMultiPrimaryRequest(vol_id,
                                                       blobReq->dmt_version,
                                                       message,
                                                       respCb);
        setSerialization(amReq, asyncSetMDReq);
        asyncSetMDReq->invoke();
        return;
    } else {
        ReadGuard rg(volumegroup_lock);
        auto it = volumegroup_map.find(amReq->io_vol_id);
        if (volumegroup_map.end() != it) {
            it->second->sendModifyMsg(message_type_id(*message),
                                      message,
                                      [blobReq, this] (Error const& e, shared_str p) mutable -> void {
                                          _setBlobMetadataCb(blobReq, e, p);
                                      });
            return;
        }
    }
    LOGERROR << "Unknown volume to AmDispatcher: " << amReq->io_vol_id;
    AmDataProvider::setBlobMetadataCb(amReq, ERR_VOLUME_ACCESS_DENIED);
}

void
AmDispatcher::setVolumeMetadata(AmRequest* amReq) {
    fiu_do_on("am.uturn.dispatcher", return AmDataProvider::setVolumeMetadataCb(amReq, ERR_OK););

    auto message = boost::make_shared<fpi::SetVolumeMetadataMsg>();
    auto volReq = static_cast<SetVolumeMetadataReq*>(amReq);

    message->volumeId = amReq->io_vol_id.get();
    // Copy api structure into fdsp structure.
    // TODO(Andrew): Make these calls use the same structure.
    fpi::FDSP_MetaDataPair metaPair;
    for (auto const &meta : *(volReq->metadata)) {
        metaPair.key = meta.first;
        metaPair.value = meta.second;
        message->metadataList.push_back(metaPair);
    }

    /**
     * FEATURE TOGGLE: VolumeGrouping
     * Thu Jan 14 10:39:09 2016
     */
    if (!volume_grouping_support) {
        volReq->dmt_version = dmtMgr->getAndLockCurrentVersion();
        auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::setVolumeMetadataCb, volReq));
        auto asyncSetVolMetadataReq = createMultiPrimaryRequest(amReq->io_vol_id,
                                                                amReq->dmt_version,
                                                                message,
                                                                respCb);
        setSerialization(amReq, asyncSetVolMetadataReq);
        asyncSetVolMetadataReq->invoke();
        return;
    } else {
        ReadGuard rg(volumegroup_lock);
        auto it = volumegroup_map.find(amReq->io_vol_id);
        if (volumegroup_map.end() != it) {
            it->second->sendCommitMsg(message_type_id(*message),
                                      message,
                                      [volReq, this] (Error const& e, shared_str p) mutable -> void {
                                          _setVolumeMetadataCb(volReq, e, p);
                                      });
            return;
        }
    }
    LOGERROR << "Unknown volume to AmDispatcher: " << amReq->io_vol_id;
    AmDataProvider::setVolumeMetadataCb(amReq, ERR_VOLUME_ACCESS_DENIED);
}

void
AmDispatcher::startBlobTx(AmRequest* amReq) {
    fiu_do_on("am.uturn.dispatcher", return AmDataProvider::startBlobTxCb(amReq, ERR_OK););
    auto *blobReq = static_cast<StartBlobTxReq *>(amReq);

    /**
     * FEATURE TOGGLE: VolumeGrouping
     * Thu Jan 14 10:39:09 2016
     */
    if (!volume_grouping_support) {
        // Update DMT version in request
        blobReq->dmt_version = dmtMgr->getAndLockCurrentVersion();

        // Check if we already have outstanding tx's on this blob
        blob_id_type blob_id = std::make_pair(amReq->io_vol_id, amReq->getBlobName());
        {
            std::lock_guard<std::mutex> g(tx_map_lock);
            auto it = tx_map_barrier.find(blob_id);
            if (tx_map_barrier.end() == it) {
                bool happened {false};
                std::tie(it, happened) =
                    tx_map_barrier.emplace(std::make_pair(blob_id,
                                                          std::make_tuple(blobReq->dmt_version,
                                                                          0,
                                                                          std::deque<StartBlobTxReq*>())));
            } else if (std::get<0>(it->second) != blobReq->dmt_version) {
                // Delay the request
                LOGDEBUG << "Delaying Tx start while old tx's clean up.";
                dmtMgr->releaseVersion(amReq->dmt_version);
                return std::get<2>(it->second).push_back(blobReq);
            }
            ++std::get<1>(it->second);
        }
        // This needs to stay here so the lock is still held.
        _startBlobTx(amReq);
        return;
    }
    _startBlobTx(amReq);
}

void
AmDispatcher::_startBlobTx(AmRequest *amReq) {
    // Create network message
    auto message = boost::make_shared<fpi::StartBlobTxMsg>();
    auto *blobReq = static_cast<StartBlobTxReq *>(amReq);
    message->blob_name    = amReq->getBlobName();
    message->blob_version = blob_version_invalid;
    message->volume_id    = amReq->io_vol_id.get();
    message->blob_mode    = blobReq->blob_mode;
    message->txId         = blobReq->tx_desc->getValue();

    /**
     * FEATURE TOGGLE: VolumeGrouping
     * Thu Jan 14 10:39:09 2016
     */
    if (!volume_grouping_support) {
        message->dmt_version  = blobReq->dmt_version;
        auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::startBlobTxCb, blobReq));
        auto asyncStartBlobTxReq = createMultiPrimaryRequest(amReq->io_vol_id,
                                                             blobReq->dmt_version,
                                                             message,
                                                             respCb);
        setSerialization(amReq, asyncStartBlobTxReq);
        asyncStartBlobTxReq->invoke();
        return;
    } else {
        ReadGuard rg(volumegroup_lock);
        auto it = volumegroup_map.find(amReq->io_vol_id);
        if (volumegroup_map.end() != it) {
            it->second->sendModifyMsg(message_type_id(*message),
                                      message,
                                      [blobReq, this] (Error const& e, shared_str p) mutable -> void {
                                          _startBlobTxCb(blobReq, e, p);
                                      });
            return;
        }
    }
    LOGERROR << "Unknown volume to AmDispatcher: " << amReq->io_vol_id;
    AmDataProvider::startBlobTxCb(amReq, ERR_VOLUME_ACCESS_DENIED);
}

void
AmDispatcher::statBlob(AmRequest* amReq)
{
    fiu_do_on("am.uturn.dispatcher",
              auto cb = std::dynamic_pointer_cast<StatBlobCallback>(amReq->cb); \
              cb->blobDesc = boost::make_shared<BlobDescriptor>(); \
              cb->blobDesc->setBlobName(amReq->getBlobName()); \
              cb->blobDesc->setBlobSize(0); \
              return AmDataProvider::statBlobCb(amReq, ERR_OK););

    auto message = boost::make_shared<fpi::GetBlobMetaDataMsg>();
    message->volume_id = amReq->io_vol_id.get();
    message->blob_name = amReq->getBlobName();
    message->metaDataList.clear();
    auto blobReq = static_cast<StatBlobReq*>(amReq);

    /**
     * FEATURE TOGGLE: VolumeGrouping
     * Thu Jan 14 10:39:09 2016
     */
    if (!volume_grouping_support) {
        amReq->dmt_version = dmtMgr->getAndLockCurrentVersion();
        auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::statBlobCb, blobReq));
        auto asyncReq = createFailoverRequest(amReq->io_vol_id,
                                              amReq->dmt_version,
                                              message,
                                              respCb);
        asyncReq->onEPAppStatusCb(std::bind(&AmDispatcher::missingBlobStatusCb,
                                            this, amReq, std::placeholders::_1,
                                            std::placeholders::_2));
        asyncReq->invoke();
        return;
    } else {
        ReadGuard rg(volumegroup_lock);
        auto it = volumegroup_map.find(amReq->io_vol_id);
        if (volumegroup_map.end() != it) {
            it->second->sendReadMsg(message_type_id(*message),
                                    message,
                                    [blobReq, this] (Error const& e, shared_str p) mutable -> void {
                                        _statBlobCb(blobReq, e, p);
                                    });
            return;
        }
    }
    LOGERROR << "Unknown volume to AmDispatcher: " << amReq->io_vol_id;
    AmDataProvider::statBlobCb(amReq, ERR_VOLUME_ACCESS_DENIED);
}

void
AmDispatcher::statVolume(AmRequest* amReq) {
    auto message = boost::make_shared<fpi::StatVolumeMsg>();
    message->volume_id = amReq->io_vol_id.get();
    auto volReq = static_cast<StatVolumeReq*>(amReq);

    /**
     * FEATURE TOGGLE: VolumeGrouping
     * Thu Jan 14 10:39:09 2016
     */
    if (!volume_grouping_support) {
        amReq->dmt_version = dmtMgr->getAndLockCurrentVersion();
        auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::statVolumeCb, volReq));
        auto asyncStatVolReq = createFailoverRequest(amReq->io_vol_id,
                                                     amReq->dmt_version,
                                                     message,
                                                     respCb);
        asyncStatVolReq->invoke();
        return;
    } else {
        ReadGuard rg(volumegroup_lock);
        auto it = volumegroup_map.find(amReq->io_vol_id);
        if (volumegroup_map.end() != it) {
            it->second->sendReadMsg(message_type_id(*message),
                                    message,
                                    [volReq, this] (Error const& e, shared_str p) mutable -> void {
                                        _statVolumeCb(volReq, e, p);
                                    });
            return;
        }
    }
    LOGERROR << "Unknown volume to AmDispatcher: " << amReq->io_vol_id;
    AmDataProvider::statVolumeCb(amReq, ERR_VOLUME_ACCESS_DENIED);
}


void
AmDispatcher::volumeContents(AmRequest* amReq)
{
    fiu_do_on("am.uturn.dispatcher",
              auto cb = std::dynamic_pointer_cast<GetBucketCallback>(amReq->cb); \
              cb->vecBlobs = boost::make_shared<std::vector<fds::BlobDescriptor>>(); \
              cb->skippedPrefixes = boost::make_shared<std::vector<std::string>>(); \
              return AmDataProvider::volumeContentsCb(amReq, ERR_OK););

    auto volReq = static_cast<VolumeContentsReq*>(amReq);
    auto message = boost::make_shared<fpi::GetBucketMsg>();
    message->volume_id = amReq->io_vol_id.get();
    message->startPos  = volReq->offset;
    message->count   = volReq->count;
    message->pattern = volReq->pattern;
    message->patternSemantics = volReq->patternSemantics;
    message->delimiter = volReq->delimiter;
    message->orderBy = volReq->orderBy;
    message->descending = volReq->descending;

    /**
     * FEATURE TOGGLE: VolumeGrouping
     * Thu Jan 14 10:39:09 2016
     */
    if (!volume_grouping_support) {
        amReq->dmt_version = dmtMgr->getAndLockCurrentVersion();
        auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::volumeContentsCb, volReq));
        auto asyncReq = createFailoverRequest(amReq->io_vol_id,
                                              amReq->dmt_version,
                                              message,
                                              respCb);
        asyncReq->invoke();
        return;
    } else {
        ReadGuard rg(volumegroup_lock);
        auto it = volumegroup_map.find(amReq->io_vol_id);
        if (volumegroup_map.end() != it) {
            it->second->sendReadMsg(message_type_id(*message),
                                    message,
                                    [volReq, this] (Error const& e, shared_str p) mutable -> void {
                                        _volumeContentsCb(volReq, e, p);
                                    });
            return;
        }
    }
    LOGERROR << "Unknown volume to AmDispatcher: " << amReq->io_vol_id;
    AmDataProvider::volumeContentsCb(amReq, ERR_VOLUME_ACCESS_DENIED);
}


/**
 * Dispatcher Callbacks
 * The following are the callbacks for all the asynchronous requests above.
 */
void
AmDispatcher::abortBlobTxCb(AbortBlobTxReq *amReq,
                            MultiPrimarySvcRequest* svcReq,
                            const Error& error,
                            shared_str payload)
{ 
    dmtMgr->releaseVersion(amReq->dmt_version);
    blob_id_type blob_id = std::make_pair(amReq->io_vol_id, amReq->getBlobName());
    releaseTx(blob_id);

    _abortBlobTxCb(amReq, error, payload);
}

/**
 * FEATURE TOGGLE: This is the *new* callback using the VolumeGroup
 * Thu Jan 14 10:38:42 2016
 */
void
AmDispatcher::_abortBlobTxCb(AbortBlobTxReq *amReq, const Error& error, shared_str payload) {
    AmDataProvider::abortBlobTxCb(amReq, error);
}

void
AmDispatcher::closeVolumeCb(DetachVolumeReq * amReq,
                            MultiPrimarySvcRequest* svcReq,
                            const Error& error,
                            shared_str payload) {
    dmtMgr->releaseVersion(amReq->dmt_version);
    AmDataProvider::closeVolumeCb(amReq, error);
}

void
AmDispatcher::_closeVolumeCb(DetachVolumeReq* amReq) {
    AmDataProvider::closeVolumeCb(amReq, ERR_OK);
}

void
AmDispatcher::commitBlobTxCb(CommitBlobTxReq* amReq,
                             MultiPrimarySvcRequest* svcReq,
                             const Error& error,
                             shared_str payload) {
    dmtMgr->releaseVersion(amReq->dmt_version);
    blob_id_type blob_id = std::make_pair(amReq->io_vol_id, amReq->getBlobName());
    releaseTx(blob_id);

    _commitBlobTxCb(amReq, error, payload);
}

void
AmDispatcher::_commitBlobTxCb(CommitBlobTxReq* amReq, const Error& error, shared_str payload) {
    auto err = error;
    if (err.ok()) {
        auto response = deserializeFdspMsg<fpi::CommitBlobTxRspMsg>(err, payload);
        if (err.ok()) {
            amReq->final_blob_size = response->byteCount;
            amReq->final_meta_data.swap(response->meta_list);
        }
    }

    AmDataProvider::commitBlobTxCb(amReq, err);
}

void
AmDispatcher::_deleteBlobCb(DeleteBlobReq *amReq, const Error& error, shared_str payload) {
    // Errors from a delete are okay
    auto err = error;
    if ((ERR_BLOB_NOT_FOUND == err) || (ERR_CAT_ENTRY_NOT_FOUND == err)) {
        err = ERR_OK;
    }
    AmDataProvider::deleteBlobCb(amReq, err);
}

void
AmDispatcher::getVolumeMetadataCb(GetVolumeMetadataReq* amReq,
                                  FailoverSvcRequest* svcReq,
                                  const Error& error,
                                  shared_str payload) {
    dmtMgr->releaseVersion(amReq->dmt_version);
    _getVolumeMetadataCb(amReq, error, payload);
}

void
AmDispatcher::_getVolumeMetadataCb(GetVolumeMetadataReq* amReq, const Error& error, shared_str payload) {
    auto err = error;
    if (err.ok()) {
        auto response = deserializeFdspMsg<fpi::GetVolumeMetadataMsgRsp>(err, payload);
        if (err.ok()) {
            auto cb = std::dynamic_pointer_cast<GetVolumeMetadataCallback>(amReq->cb);
            cb->metadata = boost::make_shared<std::map<std::string, std::string>>();
            // Copy the FDSP structure into the API structure
            for (auto const &meta : response->metadataList) {
                cb->metadata->emplace(std::pair<std::string, std::string>(meta.key, meta.value));
            }
        }
    }

    AmDataProvider::getVolumeMetadataCb(amReq, err);
}

void
AmDispatcher::openVolumeCb(AttachVolumeReq* amReq,
                           MultiPrimarySvcRequest* svcReq,
                           const Error& error,
                           shared_str payload) {
    dmtMgr->releaseVersion(amReq->dmt_version);
    fpi::OpenVolumeRspMsgPtr msg {nullptr};
    auto err = error;
    if (err.ok()) {
        msg = deserializeFdspMsg<fpi::OpenVolumeRspMsg>(err, payload);
    }
    _openVolumeCb(amReq, err, msg);
}

void
AmDispatcher::_openVolumeCb(AttachVolumeReq* amReq, const Error& error, fpi::OpenVolumeRspMsgPtr const& msg) {
    if (error.ok() && msg) {
        // Set the token and volume sequence returned by the DM
        amReq->token = msg->token;
        dispatchTable.registerVolumeSequence(amReq->io_vol_id, msg->sequence_id);
    }
    AmDataProvider::openVolumeCb(amReq, error);
}

void
AmDispatcher::renameBlobCb(RenameBlobReq *amReq,
                           MultiPrimarySvcRequest* svcReq,
                           const Error& error,
                           shared_str payload) {
    // Ensure we haven't already replied to this request
    dmtMgr->releaseVersion(amReq->dmt_version);
    _renameBlobCb(amReq, error, payload);
}

void
AmDispatcher::_renameBlobCb(RenameBlobReq *amReq, const Error& error, shared_str payload) {
    // Deserialize if all OK
    auto err = error;
    if (err.ok()) {
        auto cb = std::dynamic_pointer_cast<RenameBlobCallback>(amReq->cb);
        cb->blobDesc = boost::make_shared<BlobDescriptor>();
        cb->blobDesc->setBlobName(amReq->new_blob_name);

        // using the same structure for input and output
        auto response = deserializeFdspMsg<fpi::RenameBlobRespMsg>(err, payload);
        if (err.ok()) {
        // Fill in the data here
            cb->blobDesc->setBlobSize(response->byteCount);
            for (const auto& meta : response->metaDataList) {
                cb->blobDesc->addKvMeta(meta.key,  meta.value);
            }
        }
    }
    AmDataProvider::renameBlobCb(amReq, err);
}

void
AmDispatcher::_setBlobMetadataCb(SetBlobMetaDataReq *amReq, const Error& error, shared_str payload) {
    AmDataProvider::setBlobMetadataCb(amReq, error);
}

void
AmDispatcher::setVolumeMetadataCb(SetVolumeMetadataReq* amReq,
                                  MultiPrimarySvcRequest* svcReq,
                                  const Error& error,
                                  shared_str payload) {
    dmtMgr->releaseVersion(amReq->dmt_version);
    _setVolumeMetadataCb(amReq, error, payload);
}

void
AmDispatcher::_setVolumeMetadataCb(SetVolumeMetadataReq* amReq, const Error& error, shared_str payload) {
    AmDataProvider::setVolumeMetadataCb(amReq, error);
}

void
AmDispatcher::startBlobTxCb(StartBlobTxReq *amReq,
                            MultiPrimarySvcRequest *svcReq,
                            const Error &error,
                            shared_str payload) {
    // Release DMT version if transaction did not start.
    if (!error.ok()) {
        dmtMgr->releaseVersion(amReq->dmt_version);
        blob_id_type blob_id = std::make_pair(amReq->io_vol_id, amReq->getBlobName());
        releaseTx(blob_id);
    }
    _startBlobTxCb(amReq, error, payload);
}

void
AmDispatcher::_startBlobTxCb(StartBlobTxReq* amReq, const Error& error, shared_str payload) {
    AmDataProvider::startBlobTxCb(amReq, error);
}

void
AmDispatcher::statVolumeCb(StatVolumeReq* amReq,
                           FailoverSvcRequest* svcReq,
                           const Error& error,
                           shared_str payload) {
    dmtMgr->releaseVersion(amReq->dmt_version);
    _statVolumeCb(amReq, error, payload);
}

void
AmDispatcher::_statVolumeCb(StatVolumeReq* amReq, const Error& error, shared_str payload) {

    auto err = error;
    if (err.ok()) {
        auto volMDMsg = deserializeFdspMsg<fpi::StatVolumeMsg>(err, payload);
        if (err.ok()) {
            amReq->size = volMDMsg->volumeStatus.size;
            amReq->blob_count = volMDMsg->volumeStatus.blobCount;
        }
    }
    // Notify upper layers that the request is done.
    AmDataProvider::statVolumeCb(amReq, err);
}

void
AmDispatcher::volumeContentsCb(VolumeContentsReq* amReq,
                               FailoverSvcRequest* svcReq,
                               const Error& error,
                               shared_str payload)
{
    dmtMgr->releaseVersion(amReq->dmt_version);
    _volumeContentsCb(amReq, error, payload);
}

void
AmDispatcher::_volumeContentsCb(VolumeContentsReq *amReq, const Error& error, shared_str payload) {
    auto err = error;
    if (err.ok()) {
        // using the same structure for input and output
        auto response = deserializeFdspMsg<fpi::GetBucketRspMsg>(err, payload);

        if (err.ok()) {
            LOGDEBUG << " volid: " << amReq->io_vol_id << " numBlobs: " <<
                    response->blob_descr_list.size();

            auto cb = std::dynamic_pointer_cast<GetBucketCallback>(amReq->cb);
            cb->vecBlobs = boost::make_shared<std::vector<fds::BlobDescriptor>>();
            for (auto const& descriptor : response->blob_descr_list) {
                cb->vecBlobs->emplace_back(descriptor.name,
                                           amReq->io_vol_id.get(),
                                           descriptor.byteCount,
                                           descriptor.metadata);
            }
            cb->skippedPrefixes = boost::make_shared<std::vector<std::string>>();
            cb->skippedPrefixes->swap(response->skipped_prefixes);
        }
    }
    AmDataProvider::volumeContentsCb(amReq, err);
}


void
AmDispatcher::putBlobOnceCb(PutBlobReq* amReq,
                            MultiPrimarySvcRequest* svcReq,
                            const Error& error,
                            shared_str payload) {
    dmtMgr->releaseVersion(amReq->dmt_version);
    _putBlobOnceCb(amReq, error, payload);
}

void
AmDispatcher::_putBlobOnceCb(PutBlobReq* amReq, const Error& error, shared_str payload) {
    PerfTracer::tracePointEnd(amReq->dm_perf_ctx);

    bool done;
    Error err;
    std::tie(done, err) = amReq->notifyResponse(error);

    if (err.ok()) {
        auto updCatRsp = deserializeFdspMsg<fpi::UpdateCatalogOnceRspMsg>(err, payload);
        if (err.ok()) {
            amReq->final_blob_size = updCatRsp->byteCount;
            amReq->final_meta_data.swap(updCatRsp->meta_list);
        }
        if (safe_atomic_write) {
            return putObject(amReq);
        }
    }

    if (safe_atomic_write || done) {
        if (ERR_OK != err) {
            LOGERROR << "Failed to update"
                     << " blob name: " << amReq->getBlobName()
                     << " offset: " << amReq->blob_offset
                     << " Error: " << err;
        }
        AmDataProvider::putBlobOnceCb(amReq, err);
    }
}

void
AmDispatcher::_putBlobCb(PutBlobReq* amReq, const Error& error, shared_str payload) {
    PerfTracer::tracePointEnd(amReq->dm_perf_ctx);

    bool done;
    Error err;
    std::tie(done, err) = amReq->notifyResponse(error);

    if (done) {
        if (ERR_OK != error) {
            LOGERROR << "Failed to update"
                     << " blob name: " << amReq->getBlobName()
                     << " offset: " << amReq->blob_offset
                     << " Error: " << err;
        }
        AmDataProvider::putBlobCb(amReq, err);
    }
}

void
AmDispatcher::putObject(AmRequest* amReq) {
    auto blobReq = static_cast<PutBlobReq *>(amReq);

    // Sweet, we're done!
    if (0 == amReq->data_len) {
        return putObjectCb(amReq, ERR_OK);
    }

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


    auto respCb(RESPONSE_MSG_HANDLER(AmDispatcher::dispatchObjectCb, amReq));
    auto asyncPutReq = createQuorumRequest(blobReq->obj_id,
                                           dlt,
                                           putObjMsg,
                                           respCb,
                                           message_timeout_io);

    setSerialization(amReq, asyncPutReq);
    PerfTracer::tracePointBegin(amReq->sm_perf_ctx);
    asyncPutReq->invoke();

    LOGDEBUG << asyncPutReq->logString() << logString(*putObjMsg)
             << " DLT version " << dlt->getVersion();
}

void
AmDispatcher::dispatchObjectCb(AmRequest* amReq,
                          QuorumSvcRequest* svcReq,
                          const Error& error,
                          shared_str payload) {
    // notify DLT manager that request completed, so we can decrement refcnt
    dltMgr->releaseVersion(amReq->dlt_version);
    PerfTracer::tracePointEnd(amReq->sm_perf_ctx);

    putObjectCb(amReq, error);
}

void
AmDispatcher::putObjectCb(AmRequest* amReq, const Error error) {
    bool done;
    Error err;
    std::tie(done, err) = static_cast<AmMultiReq*>(amReq)->notifyResponse(error);
    if (done) {
        if (ERR_OK != err) {
            LOGERROR << "Failed to update"
                     << " blob name: " << amReq->getBlobName()
                     << " offset: " << amReq->blob_offset
                     << " Error: " << err;
        }
        if (fds::FDS_PUT_BLOB == amReq->io_type) {
            return AmDataProvider::putBlobCb(amReq, err);
        } else {
            return AmDataProvider::putBlobOnceCb(amReq, err);
        }
    }
}

void
AmDispatcher::getObject(AmRequest* amReq)
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
                          shared_str payload)
{
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

    AmDataProvider::getObjectCb(amReq, error);
}

fds_bool_t
AmDispatcher::missingBlobStatusCb(AmRequest* amReq,
                                  const Error& error,
                                  shared_str payload) {
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
AmDispatcher::getQueryCatalogCb(GetBlobReq* amReq,
                                FailoverSvcRequest* svcReq,
                                const Error& error,
                                shared_str payload) {
    dmtMgr->releaseVersion(amReq->dmt_version);
    _getQueryCatalogCb(amReq, error, payload);
}

void
AmDispatcher::_getQueryCatalogCb(GetBlobReq* amReq, const Error& error, shared_str payload)
{
    PerfTracer::tracePointEnd(amReq->dm_perf_ctx);

    Error err = error;
    if (err != ERR_OK && err != ERR_BLOB_OFFSET_INVALID) {
        // TODO(Andrew): We should consider logging this error at a
        // higher level when the volume is not block
        LOGDEBUG << "blob name: " << amReq->getBlobName() << " offset: "
                 << amReq->blob_offset << " Error: " << error;
        err = (err == ERR_CAT_ENTRY_NOT_FOUND ? ERR_BLOB_NOT_FOUND : err);
    }

    auto qryCatRsp = deserializeFdspMsg<fpi::QueryCatalogMsg>(err, payload);

    if (err.ok()) {
        // Copy the metadata into the callback, if needed
        if (true == amReq->get_metadata) {
            auto cb = std::dynamic_pointer_cast<GetObjectWithMetadataCallback>(amReq->cb);
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

        amReq->object_ids.swap(new_ids);
        amReq->object_ids.shrink_to_fit();
    }
    AmDataProvider::getOffsetsCb(amReq, err);
}

void
AmDispatcher::statBlobCb(StatBlobReq *amReq,
                         FailoverSvcRequest* svcReq,
                         const Error& error,
                         shared_str payload) {
    dmtMgr->releaseVersion(amReq->dmt_version);
    _statBlobCb(amReq, error, payload);
}

void
AmDispatcher::_statBlobCb(StatBlobReq* amReq, const Error& error, shared_str payload)
{
    // Deserialize if all OK
    if (ERR_OK == error) {
        // using the same structure for input and output
        auto response = MSG_DESERIALIZE(GetBlobMetaDataMsg, error, payload);

        auto cb = std::dynamic_pointer_cast<StatBlobCallback>(amReq->cb);
        // Fill in the data here
        cb->blobDesc = boost::make_shared<BlobDescriptor>();
        cb->blobDesc->setBlobName(amReq->getBlobName());
        cb->blobDesc->setBlobSize(response->byteCount);
        for (const auto& meta : response->metaDataList) {
            cb->blobDesc->addKvMeta(meta.key,  meta.value);
        }
    }
    AmDataProvider::statBlobCb(amReq, error);
}

void
AmDispatcher::releaseTx(blob_id_type const& blob_id) {
        std::lock_guard<std::mutex> g(tx_map_lock);
        auto it = tx_map_barrier.find(blob_id);
        if (tx_map_barrier.end() == it) {
            // Probably already implicitly aborted
            GLOGDEBUG << "Woah, missing map entry!";
            return;
        }
        GLOGDEBUG << "Draining any needed pending tx's";
        if (0 == --std::get<1>(it->second)) {
            // Drain
            auto queue = std::move(std::get<2>(it->second));
            tx_map_barrier.erase(it);
            for (auto& req : queue) {
                req->dmt_version = dmtMgr->getAndLockCurrentVersion();
                _startBlobTx(req);
            }
        }
}

bool
ErrorHandler::isError(fpi::FDSPMsgTypeId const& msgType, Error const& e) {
    // A big white list
    switch (e.GetErrno()) {
    // duh.
    case ERR_OK:

    /***
     * Logical Blob errors are not really errors, ignore these the connector
     * will deal with it most likely by padding or otherwise.
     */
    case ERR_CAT_ENTRY_NOT_FOUND:
    case ERR_BLOB_NOT_FOUND:
    case ERR_BLOB_OFFSET_INVALID:

    /***
     * Neither are errors that _we_ caused by sending a bad
     * request
     */
    case ERR_INVALID_ARG:
    case ERR_DM_OP_NOT_ALLOWED:
    case ERR_VOLUME_ACCESS_DENIED:
        return false;
        break;;
    default:
        return true;;
    }
}

}  // namespace fds
