/*
 * Copyright 2014-2016 Formation Data Systems, Inc.
 */

#include <algorithm>
#include <vector>

#include "fds_process.h"
#include "fdsp/dm_api_types.h"
#include "fdsp/sm_api_types.h"

#include <AmDispatcher.h>
#include <AmDispatcher.tcc>
#include <net/SvcRequestPool.h>
#include <net/SvcMgr.h>
#include <fiu-control.h>
#include <util/fiu_util.h>

#include "AmAsyncDataApi.h"
#include "requests/requests.h"
#include "requests/GetObjectReq.h"
#include "requests/PutObjectReq.h"
#include "requests/UpdateCatalogReq.h"
#include <net/MockSvcHandler.h>
#include "net/VolumeGroupHandle.h"
#include <AmDispatcherMocks.hpp>
#include "lib/StatsCollector.h"

namespace fds {

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

AmDispatcher::AmDispatcher(AmDataProvider* prev)
        : AmDataProvider(prev, nullptr)
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

void
AmDispatcher::removeVolume(VolumeDesc const& volDesc) {
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
AmDispatcher::addToVolumeGroup(const fpi::AddToVolumeGroupCtrlMsgPtr &addMsg,
                               const AddToVolumeGroupCb &cb)
{
    auto vol_id = fds_volid_t(addMsg->groupId);

    {
        ReadGuard rg(volumegroup_lock);
        auto it = volumegroup_map.find(vol_id);
        if (volumegroup_map.end() != it) {
            it->second->handleAddToVolumeGroupMsg(addMsg, cb);
            return;
        }
    }

    LOGERROR << "Unknown volume to AmDispatcher: " << vol_id;
    cb(ERR_VOL_NOT_FOUND, MAKE_SHARED<fpi::AddToVolumeGroupRespCtrlMsg>());
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

    auto blobReq = static_cast<AbortBlobTxReq*>(amReq);
    auto message = boost::make_shared<fpi::AbortBlobTxMsg>();
    message->blob_name      = amReq->getBlobName();
    message->blob_version   = blob_version_invalid;
    message->volume_id      = blobReq->io_vol_id.get();
    message->txId           = blobReq->tx_desc->getValue();

    /**
     * FEATURE TOGGLE: VolumeGrouping
     * Thu Jan 14 10:39:09 2016
     */
    if (!volume_grouping_support) {
        writeToDM(blobReq, message, &AmDispatcher::abortBlobTxCb);
    } else {
        volumeGroupModify(blobReq, message, &AmDispatcher::_abortBlobTxCb);
    }
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
        auto message = boost::make_shared<fpi::CloseVolumeMsg>();
        message->volume_id = amReq->io_vol_id.get();
        message->token = volReq->token;

        amReq->dmt_version = dmtMgr->getAndLockCurrentVersion();
        writeToDM(volReq, message, &AmDispatcher::closeVolumeCb);
        return;
    } else {
        WriteGuard wg(volumegroup_lock);
        auto it = volumegroup_map.find(amReq->io_vol_id);
        if (volumegroup_map.end() != it) {
            // Give ownership of the volume handle to the handle itself, we're
            // done with it
            std::shared_ptr<VolumeGroupHandle> vg = std::move(it->second);
            volumegroup_map.erase(it);
            vg->close([this, volReq, vg] () mutable -> void {
                            _closeVolumeCb(volReq);
                        });
            return;
        }
    }
    return AmDataProvider::closeVolumeCb(amReq, ERR_OK);
}

Error AmDispatcher::modifyVolumePolicy(const VolumeDesc& vdesc) {
    if (volume_grouping_support) {
        // Check to see if another AM has claimed ownership of coordinating the volume
        WriteGuard wg(volumegroup_lock);
        auto it = volumegroup_map.find(vdesc.GetID());
        if (volumegroup_map.end() != it &&
            vdesc.getCoordinatorId() != MODULEPROVIDER()->getSvcMgr()->getSelfSvcUuid() &&
            vdesc.getCoordinatorVersion() > it->second->getVersion()) {
            // Give ownership of the volume handle to the handle itself, we're
            // done with it
            std::shared_ptr<VolumeGroupHandle> vg = std::move(it->second);
            volumegroup_map.erase(it);
            vg->close([this, vg] () mutable -> void {});
        }
    }
    return AmDataProvider::modifyVolumePolicy(vdesc);
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
    auto vLock = dispatchTable.getAndLockVolumeSequence(amReq->io_vol_id, message->sequence_id);
    if (!volume_grouping_support) {
        message->dmt_version = amReq->dmt_version;
        writeToDM(blobReq, message, &AmDispatcher::commitBlobTxCb);
    } else {
        volumeGroupCommit(blobReq, message, &AmDispatcher::_commitBlobTxCb, std::move(vLock));
    }
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
        writeToDM(blobReq, message, &AmDispatcher::deleteBlobCb);
    } else {
        volumeGroupModify(blobReq, message, &AmDispatcher::_deleteBlobCb);
    }
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
    auto const& volId = amReq->io_vol_id;

    LOGTRACE << "blob name: " << amReq->getBlobName()
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
        readFromDM(blobReq, message, &AmDispatcher::getQueryCatalogCb, message_timeout_io);
    } else {
        volumeGroupRead(blobReq, message, &AmDispatcher::_getQueryCatalogCb);
    }
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
        readFromDM(volReq, message, &AmDispatcher::getVolumeMetadataCb);
    } else {
        volumeGroupRead(volReq, message, &AmDispatcher::_getVolumeMetadataCb);
    }
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
        writeToDM(volReq, volMDMsg, &AmDispatcher::openVolumeCb, message_timeout_open);
        return;
    } else {
        WriteGuard wg(volumegroup_lock);
        auto& vol_id = amReq->io_vol_id;
        bool happened {false};
        auto it = volumegroup_map.end();
        std::tie(it, happened) = volumegroup_map.emplace(vol_id, nullptr);
        if (happened) {
            auto quorum = std::max(MODULEPROVIDER()->getSvcMgr()->getCurrentDMT()->getDepth() - 1,
                                   static_cast<uint32_t>(1));
            it->second.reset(new VolumeGroupHandle(MODULEPROVIDER(), vol_id, quorum));
            if (!it->second) {
                LOGERROR << "Could not create a volume group handle for this volume: " << vol_id;
                AmDataProvider::openVolumeCb(amReq, ERR_VOLUME_ACCESS_DENIED);
                return;
            }
            it->second->setListener(volumegroup_handler.get());
        }

        volMDMsg->volume_id = vol_id.get();
        it->second->open(volMDMsg,
                         [volReq, this] (Error const& e, fpi::OpenVolumeRspMsgPtr const& p) mutable -> void {
                            _openVolumeCb(volReq, e, p);
                            });
        return;
    }
}

void
AmDispatcher::putBlobTx(AmRequest* amReq) {
    auto blobReq = static_cast<UpdateCatalogReq *>(amReq);

    auto message(boost::make_shared<fpi::UpdateCatalogMsg>());
    message->blob_name    = amReq->getBlobName();
    message->blob_version = blob_version_invalid;
    message->volume_id    = amReq->io_vol_id.get();
    message->txId         = blobReq->tx_desc->getValue();

    // Setup blob offset updates
    for (auto const& obj_upd : blobReq->object_list) {
        auto const& obj_id = obj_upd.first;
        fpi::FDSP_BlobObjectInfo updBlobInfo;
        updBlobInfo.offset   = obj_upd.second.first;
        updBlobInfo.size     = obj_upd.second.second;
        updBlobInfo.data_obj_id.digest = std::string(
            reinterpret_cast<const char*>(obj_id.GetId()),
            obj_id.GetLen());
        // Add the offset info to the DM message
        message->obj_list.push_back(updBlobInfo);
    }

    fds::PerfTracer::tracePointBegin(amReq->dm_perf_ctx);

    /**
     * FEATURE TOGGLE: VolumeGrouping
     * Thu Jan 14 10:39:09 2016
     */
    if (!volume_grouping_support) {
        writeToDM(blobReq, message, &AmDispatcher::putBlobTxCb, message_timeout_io);
    } else {
        volumeGroupModify(blobReq, message, &AmDispatcher::_putBlobTxCb);
    }
}

void
AmDispatcher::updateCatalog(AmRequest* amReq) {
    fiu_do_on("am.uturn.dispatcher", return AmDataProvider::updateCatalogCb(amReq, ERR_OK););
    auto blobReq = static_cast<UpdateCatalogReq *>(amReq);
    if (!blobReq->atomic) {
        putBlobTx(blobReq);
        return;
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
    for (auto const& obj_upd : blobReq->object_list) {
        auto const& obj_id = obj_upd.first;
        fpi::FDSP_BlobObjectInfo updBlobInfo;
        updBlobInfo.offset   = obj_upd.second.first;
        updBlobInfo.size     = obj_upd.second.second;
        updBlobInfo.data_obj_id.digest = std::string(
            reinterpret_cast<const char*>(obj_id.GetId()),
            obj_id.GetLen());
        // Add the offset info to the DM message
        message->obj_list.push_back(updBlobInfo);
    }

    // Setup blob metadata updates
    if (blobReq->metadata) {
        for (auto const& meta : *(blobReq->metadata)) {
            FDS_ProtocolInterface::FDSP_MetaDataPair metaDataPair;
            metaDataPair.key   = meta.first;
            metaDataPair.value = meta.second;
            message->meta_list.push_back(metaDataPair);
        }
    }

    PerfTracer::tracePointBegin(amReq->dm_perf_ctx);

    /**
     * FEATURE TOGGLE: VolumeGrouping
     * Thu Jan 14 10:39:09 2016
     */
    auto vLock = dispatchTable.getAndLockVolumeSequence(amReq->io_vol_id, message->sequence_id);
    if (!volume_grouping_support) {
        blobReq->dmt_version = dmtMgr->getAndLockCurrentVersion();
        message->dmt_version = blobReq->dmt_version;
        writeToDM(blobReq, message, &AmDispatcher::updateCatalogCb, message_timeout_io);
    } else {
        volumeGroupCommit(blobReq, message, &AmDispatcher::_updateCatalogCb, std::move(vLock));
    }
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
    auto vLock = dispatchTable.getAndLockVolumeSequence(amReq->io_vol_id, message->sequence_id);
    if (!volume_grouping_support) {
        blobReq->dmt_version = dmtMgr->getAndLockCurrentVersion();
        message->dmt_version = blobReq->dmt_version;
        writeToDM(blobReq, message, &AmDispatcher::renameBlobCb, message_timeout_io);
    } else {
        volumeGroupCommit(blobReq, message, &AmDispatcher::_renameBlobCb, std::move(vLock));
    }
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
        writeToDM(blobReq, message, &AmDispatcher::setBlobMetadataCb);
    } else {
        volumeGroupModify(blobReq, message, &AmDispatcher::_setBlobMetadataCb);
    }
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
    auto vLock = dispatchTable.getAndLockVolumeSequence(amReq->io_vol_id, message->sequence_id);
    if (!volume_grouping_support) {
        volReq->dmt_version = dmtMgr->getAndLockCurrentVersion();
        writeToDM(volReq, message, &AmDispatcher::setVolumeMetadataCb);
    } else {
        volumeGroupCommit(volReq, message, &AmDispatcher::_setVolumeMetadataCb, std::move(vLock));
    }
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
        message->dmt_version = blobReq->dmt_version;
        writeToDM(blobReq, message, &AmDispatcher::startBlobTxCb);
    } else {
        volumeGroupModify(blobReq, message, &AmDispatcher::_startBlobTxCb);
    }
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
        readFromDM(blobReq, message, &AmDispatcher::statBlobCb);
    } else {
        volumeGroupRead(blobReq, message, &AmDispatcher::_statBlobCb);
    }
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
        readFromDM(volReq, message, &AmDispatcher::statVolumeCb);
    } else {
        volumeGroupRead(volReq, message, &AmDispatcher::_statVolumeCb);
    }
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
        readFromDM(volReq, message, &AmDispatcher::volumeContentsCb);
    } else {
        volumeGroupRead(volReq, message, &AmDispatcher::_volumeContentsCb);
    }
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
            LOGTRACE << "volid: " << amReq->io_vol_id
                     << " numBlobs: " << response->blob_descr_list.size();

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
AmDispatcher::updateCatalogCb(UpdateCatalogReq* amReq,
                              MultiPrimarySvcRequest* svcReq,
                              const Error& error,
                              shared_str payload) {
    dmtMgr->releaseVersion(amReq->dmt_version);
    _updateCatalogCb(amReq, error, payload);
}

void
AmDispatcher::_updateCatalogCb(UpdateCatalogReq* amReq, const Error& error, shared_str payload) {
    PerfTracer::tracePointEnd(amReq->dm_perf_ctx);

    bool done;
    Error err = error;
    if (err.ok()) {
        auto updCatRsp = deserializeFdspMsg<fpi::UpdateCatalogOnceRspMsg>(err, payload);
        if (err.ok()) {
            amReq->final_blob_size = updCatRsp->byteCount;
            BlobDescriptor::BlobKeyValue map;
            for (auto const& meta_pair : updCatRsp->meta_list) {
                amReq->final_meta_data.emplace(meta_pair.key, meta_pair.value);
            }
        }
    }
    AmDataProvider::updateCatalogCb(amReq, err);
}

void
AmDispatcher::_putBlobTxCb(UpdateCatalogReq* amReq, const Error& error, shared_str payload) {
    PerfTracer::tracePointEnd(amReq->dm_perf_ctx);
    if (ERR_OK != error) {
        LOGERROR << "Failed to update"
                 << " blob name: " << amReq->getBlobName()
                 << " offset: " << amReq->blob_offset
                 << " Error: " << error;
    }
    AmDataProvider::updateCatalogCb(amReq, error);
}

void
AmDispatcher::putObject(AmRequest* amReq) {
    auto objReq = static_cast<PutObjectReq *>(amReq);

    // Sweet, we're done!
    if (0 == objReq->data_len) {
        AmDataProvider::putObjectCb(amReq, ERR_OK);
        return;
    }

    auto message(boost::make_shared<fpi::PutObjectMsg>());
    message->volume_id          = objReq->io_vol_id.get();
    message->data_obj.assign(objReq->dataPtr->c_str(), objReq->data_len);
    message->data_obj_len       = objReq->data_len;
    message->data_obj_id.digest = std::string(
        reinterpret_cast<const char*>(objReq->obj_id.GetId()),
        objReq->obj_id.GetLen());

    writeToSM(objReq, message, &AmDispatcher::putObjectCb, message_timeout_io);
}

void
AmDispatcher::putObjectCb(PutObjectReq* amReq,
                          QuorumSvcRequest* svcReq,
                          const Error& error,
                          shared_str payload) {
    // notify DLT manager that request completed, so we can decrement refcnt
    dltMgr->releaseVersion(amReq->dlt_version);
    PerfTracer::tracePointEnd(amReq->sm_perf_ctx);

    AmDataProvider::putObjectCb(amReq, error);
}

void
AmDispatcher::getObject(AmRequest* amReq)
{
    // The connectors expect some underlying string even for empty buffers
    fiu_do_on("am.uturn.dispatcher",
              mockHandler_->schedule(mockTimeoutUs_,
                                     std::bind(&AmDispatcherMockCbs::getObjectCb, amReq)); \
                                     return;);

    auto blobReq = static_cast<GetObjectReq *>(amReq);
    ObjectID const& objId = *blobReq->obj_id;

    auto message(boost::make_shared<fpi::GetObjectMsg>());
    message->volume_id = amReq->io_vol_id.get();
    message->data_obj_id.digest = std::string(reinterpret_cast<const char*>(objId.GetId()),
                                              objId.GetLen());

    readFromSM(blobReq, message, &AmDispatcher::getObjectCb, message_timeout_io);
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
        auto blobReq = static_cast<GetObjectReq*>(amReq);
        LOGTRACE "Got object: " << *blobReq->obj_id;
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

    Error err { ERR_OK };
    switch (error.GetErrno()) {
    case ERR_CAT_ENTRY_NOT_FOUND:
        err = ERR_BLOB_NOT_FOUND;
        break;;
    case ERR_BLOB_OFFSET_INVALID:
        // This is fine, return OK
        break;;
    default:
        err = error;
    }

    auto qryCatRsp = deserializeFdspMsg<fpi::QueryCatalogMsg>(err, payload);
    if (ERR_OK == err) {
        amReq->blobDesc = boost::make_shared<BlobDescriptor>();
        amReq->blobDesc->setBlobName(amReq->getBlobName());
        amReq->blobDesc->setBlobSize(qryCatRsp->byteCount);
        for (const auto& meta : qryCatRsp->meta_list) {
            amReq->blobDesc->addKvMeta(meta.key,  meta.value);
        }

        for (fpi::FDSP_BlobObjectList::const_iterator it = qryCatRsp->obj_list.cbegin();
             it != qryCatRsp->obj_list.cend();
             ++it) {
            fds_uint64_t cur_offset = it->offset;
            if (cur_offset >= amReq->blob_offset || cur_offset <= amReq->blob_offset_end) {
                auto objectOffset = (cur_offset - amReq->blob_offset) / amReq->object_size;
                amReq->object_ids[objectOffset].reset(new ObjectID((*it).data_obj_id.digest));
            }
        }
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
