/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_DMHANDLER_H_
#define SOURCE_DATA_MGR_INCLUDE_DMHANDLER_H_

#include <string>
#include <fds_error.h>
#include <fds_types.h>
#include <util/Log.h>
#include <net/PlatNetSvcHandler.h>
#include <net/SvcRequestPool.h>
#include <net/SvcMgr.h>
#include <fds_module_provider.h>
#include <DmIoReq.h>
#include <DmBlobTypes.h>

#define DMHANDLER(CLASS, IOTYPE) \
    static_cast<CLASS*>(dataMgr->handlers.at(IOTYPE))

#define REGISTER_DM_MSG_HANDLER(FDSPMsgT, func) \
    REGISTER_FDSP_MSG_HANDLER_GENERIC(dataManager.getModuleProvider()->getSvcMgr()->getSvcRequestHandler(), \
            FDSPMsgT, func)

#define DM_SEND_ASYNC_RESP(...) \
    dataManager.getModuleProvider()->getSvcMgr()->getSvcRequestHandler()->sendAsyncResp(__VA_ARGS__)

#define HANDLE_INVALID_TX_ID_VAL(val) \
    if (BlobTxId::txIdInvalid == (val)) { \
        LOGWARN << "Received invalid tx id with" << logString(*message); \
        handleResponse(asyncHdr, message, ERR_DM_INVALID_TX_ID, nullptr); \
        return; \
    }

#define HANDLE_INVALID_TX_ID() HANDLE_INVALID_TX_ID_VAL(message->txId)

#define HANDLE_FILTER_OLD_DMT_DURING_RESYNC_VAL(__vol,__dmt)
/*    if (dataManager.dmMigrationMgr->shouldFilterDmt(__vol, __dmt)) {  \
        LOGDEBUG << "Disregarding message " << logString(*message) << " for DMT: " << __dmt; \
        handleResponse(asyncHdr, message, ERR_IO_DMT_MISMATCH, nullptr); \
        return; \
        }*/

#define HANDLE_FILTER_OLD_DMT_DURING_RESYNC() HANDLE_FILTER_OLD_DMT_DURING_RESYNC_VAL(volId, asyncHdr->dlt_version)

#define HANDLE_U_TURN() \
    if (dataManager.testUturnAll) { \
        LOGNOTIFY << "Uturn testing" << logString(*message); \
        handleResponse(asyncHdr, message, ERR_OK, nullptr); \
        return; \
    }

#define VOLUME_IO_VERSION_CHECK(io, helper)

/* Ensure IO is ordered */
#define ENSURE_IO_ORDER(bSequenceIdCheck, seqId, io, helper) \
do { \
    if (!dataManager.features.isVolumegroupingEnabled()) { \
        break; \
    } \
    auto volMeta = dataManager.getVolumeMeta(io->getVolId()); \
    if (io->opId != volMeta->getOpId()+1) { \
        LOGWARN << "volid: " << io->getVolId() << ". OpId mismatch.  Current opId: " \
            << volMeta->getOpId() << " incoming opId: " << io->opId; \
        fds_assert(!"opid mismatch"); \
        helper.err = ERR_IO_OPID_MISMATCH; \
        return; \
    } \
    if (bSequenceIdCheck && seqId != volMeta->getSequenceId()+1) { \
        LOGWARN << "volid: " << io->getVolId() << ". sequenceid mismatch.  Current sequenceid: " \
            << volMeta->getSequenceId() << " incoming sequenceid: " << seqId; \
        fds_assert(!"sequenceid mismatch"); \
        helper.err = ERR_IO_SEQUENCEID_MISMATCH; \
        return; \
    } \
    /* Ideally we should increment this op id after write makes it to commit log \
     * In the current code base commit log is modified for non-coordinator issued \
     * io as well.  This makes it difficult do it there.  Since OpId is only \
     * in memory state and during resync/restarts is discarded, incrementing here \
     * should be ok \
     */ \
    volMeta->incrementOpId(); \
} while (false)

#define ENSURE_OPID_ORDER(io, helper) ENSURE_IO_ORDER(false, 0, io, helper)
#define ENSURE_SEQUENCEID_ORDER(seqId, io, helper) ENSURE_IO_ORDER(true, seqId, io, helper)

namespace fds {

struct DataMgr;

namespace dm {
/**
 * ------ NOTE :: IMPORTANT ---
 * do NOT store any state in these classes for now.
 * handler functions should be reentrant
 */
struct RequestHelper {
    DmRequest *dmRequest;
    RequestHelper(DataMgr& dataManager, DmRequest *dmRequest);
    ~RequestHelper();
private:
    DataMgr& _dataManager;
};

struct QueueHelper {
    fds_bool_t ioIsMarkedAsDone;
    fds_bool_t cancelled;
    fds_bool_t skipImplicitCb;
    DmRequest *dmRequest;
    Error err = ERR_OK;
    QueueHelper(DataMgr& dataManager, DmRequest *dmRequest);
    ~QueueHelper();
    void markIoDone();
    void cancel();
private:
    DataMgr& dataManager_;
};

struct Handler: HasLogger {
    explicit Handler(DataMgr& dataManager);
    // providing a default empty implmentation to support requests that
    // do not need queuing
    virtual void handleQueueItem(DmRequest *dmRequest);
    virtual void addToQueue(DmRequest *dmRequest);
    Error preEnqueueWriteOpHandling(const fds_volid_t &volId,
                                    const int64_t &opId,
                                    const fpi::AsyncHdrPtr &hdr,
                                    const SHPTR<std::string> &payload);

    virtual ~Handler();
protected:
    DataMgr& dataManager;
};

struct GetBucketHandler : Handler {
    explicit GetBucketHandler(DataMgr& dataManager);
    void handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                       boost::shared_ptr<fpi::GetBucketMsg>& message);
    void handleQueueItem(DmRequest *dmRequest);
    void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                        boost::shared_ptr<fpi::GetBucketRspMsg>& message,
                        const Error &e, DmRequest *dmRequest);
};

struct DmSysStatsHandler : Handler {
    explicit DmSysStatsHandler(DataMgr& dataManager);
    void handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                       boost::shared_ptr<fpi::GetDmStatsMsg>& message);
    void handleQueueItem(DmRequest *dmRequest);
    void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                        boost::shared_ptr<fpi::GetDmStatsMsg>& message,
                        const Error &e, DmRequest *dmRequest);
};

struct DeleteBlobHandler : Handler {
    explicit DeleteBlobHandler(DataMgr& dataManager);
    void handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                       boost::shared_ptr<fpi::DeleteBlobMsg>& message);
    void handleQueueItem(DmRequest *dmRequest);
    void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                        boost::shared_ptr<fpi::DeleteBlobMsg>& message,
                        const Error &e, DmRequest *dmRequest);
};

struct UpdateCatalogHandler : Handler {
    explicit UpdateCatalogHandler(DataMgr& dataManager);
    void handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                       boost::shared_ptr<fpi::UpdateCatalogMsg> & message);
    void handleQueueItem(DmRequest * dmRequest);
    void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                        boost::shared_ptr<fpi::UpdateCatalogMsg> & message,
                        const Error &e, DmRequest *dmRequest);
};

struct RenameBlobHandler : Handler {
    explicit RenameBlobHandler(DataMgr& dataManager);
    void handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                       boost::shared_ptr<fpi::RenameBlobMsg>& message);
    void handleQueueItem(DmRequest* dmRequest);
    void handleCommitNewBlob(Error const& e, DmRequest* dmRequest);
    void handleDeleteOldBlob(Error const& e, DmRequest* dmRequest);
    void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                        boost::shared_ptr<fpi::RenameBlobMsg> & message,
                        Error const& e, DmRequest* dmRequest);
    void handleResponseCleanUp(Error const& e, DmRequest* dmRequest);
};

struct GetBlobMetaDataHandler : Handler {
    explicit GetBlobMetaDataHandler(DataMgr& dataManager);
    void handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                       boost::shared_ptr<fpi::GetBlobMetaDataMsg>& message);
    void handleQueueItem(DmRequest* dmRequest);
    void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                        boost::shared_ptr<fpi::GetBlobMetaDataMsg> & message,
                        Error const& e, DmRequest* dmRequest);
};

struct QueryCatalogHandler : Handler {
    explicit QueryCatalogHandler(DataMgr& dataManager);
    void handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                       boost::shared_ptr<fpi::QueryCatalogMsg>& message);
    void handleQueueItem(DmRequest* dmRequest);
    void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                        boost::shared_ptr<fpi::QueryCatalogMsg>& message,
                        Error const& e, DmRequest* dmRequest);
};

struct StartBlobTxHandler : Handler {
    explicit StartBlobTxHandler(DataMgr& dataManager);
    void handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                       boost::shared_ptr<fpi::StartBlobTxMsg>& message);
    void handleQueueItem(DmRequest* dmRequest);
    void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                        boost::shared_ptr<fpi::StartBlobTxMsg>& message,
                        Error const& e, DmRequest* dmRequest);
};

struct StatStreamHandler : Handler {
    explicit StatStreamHandler(DataMgr& dataManager);
    void handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                       boost::shared_ptr<fpi::StatStreamMsg>& message);
    void handleQueueItem(DmRequest* dmRequest);
    void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                        boost::shared_ptr<fpi::StatStreamMsg>& message,
                        Error const& e, DmRequest* dmRequest);
};

struct CommitBlobTxHandler : Handler {
    explicit CommitBlobTxHandler(DataMgr& dataManager);
    void handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                       boost::shared_ptr<fpi::CommitBlobTxMsg>& message);
    void handleQueueItem(DmRequest* dmRequest);
    void volumeCatalogCb(Error const& e, blob_version_t blob_version,
                         BlobObjList::const_ptr const& blob_obj_list,
                         MetaDataList::const_ptr const& meta_list,
                         fds_uint64_t const blobSize,
                         DmRequest* dmRequest);
    void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                        boost::shared_ptr<fpi::CommitBlobTxMsg>& message,
                        Error const& e, DmRequest* dmRequest);
    void handleResponseCleanUp(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                        boost::shared_ptr<fpi::CommitBlobTxMsg>& message,
                        Error const& e, DmRequest* dmRequest);
};

struct UpdateCatalogOnceHandler : Handler {
    explicit UpdateCatalogOnceHandler(DataMgr& dataManager);
    void handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                       boost::shared_ptr<fpi::UpdateCatalogOnceMsg>& message);
    void handleQueueItem(DmRequest* dmRequest);
    void handleCommitBlobOnceResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                      Error const& e, DmRequest* dmRequest);
    virtual void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                boost::shared_ptr<fpi::UpdateCatalogOnceMsg>& message,
                                Error const& e, DmRequest* dmRequest);
    virtual void handleResponseCleanUp(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                       boost::shared_ptr<fpi::UpdateCatalogOnceMsg>& message,
                                       Error const& e, DmRequest* dmRequest);
};

struct SetBlobMetaDataHandler : Handler {
    explicit SetBlobMetaDataHandler(DataMgr& dataManager);
    void handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                       boost::shared_ptr<fpi::SetBlobMetaDataMsg>& message);
    void handleQueueItem(DmRequest* dmRequest);
    void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                        boost::shared_ptr<fpi::SetBlobMetaDataMsg>& message,
                        Error const& e, DmRequest* dmRequest);
};

struct AbortBlobTxHandler : Handler {
    explicit AbortBlobTxHandler(DataMgr& dataManager);
    void handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                       boost::shared_ptr<fpi::AbortBlobTxMsg>& message);
    void handleQueueItem(DmRequest* dmRequest);
    void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                        boost::shared_ptr<fpi::AbortBlobTxMsg>& message,
                        Error const& e, DmRequest* dmRequest);
};

struct ForwardCatalogUpdateHandler : Handler {
    explicit ForwardCatalogUpdateHandler(DataMgr& dataManager);
    void handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                       boost::shared_ptr<fpi::ForwardCatalogMsg>& message);
    void handleQueueItem(DmRequest* dmRequest);
};

struct StatVolumeHandler : Handler {
    explicit StatVolumeHandler(DataMgr& dataManager);
    void handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                       boost::shared_ptr<fpi::StatVolumeMsg>& message);
    void handleQueueItem(DmRequest* dmRequest);
    void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                        boost::shared_ptr<fpi::StatVolumeMsg>& message,
                        Error const& e, DmRequest* dmRequest);
};

struct SetVolumeMetadataHandler : Handler {
    explicit SetVolumeMetadataHandler(DataMgr& dataManager);
    void handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                       boost::shared_ptr<fpi::SetVolumeMetadataMsg>& message);
    void handleQueueItem(DmRequest* dmRequest);
    void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                        boost::shared_ptr<fpi::SetVolumeMetadataMsg>& message,
                        Error const& e, DmRequest* dmRequest);
};

struct GetVolumeMetadataHandler : Handler {
    explicit GetVolumeMetadataHandler(DataMgr& dataManager);
    void handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                       boost::shared_ptr<fpi::GetVolumeMetadataMsg>& message);
    void handleQueueItem(DmRequest* dmRequest);
    void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                        boost::shared_ptr<fpi::GetVolumeMetadataMsgRsp>& message,
                        Error const& e, DmRequest* dmRequest);
};

/**
 * Attempt to retrieve an access token for the given volume
 */
struct VolumeOpenHandler : Handler {
    explicit VolumeOpenHandler(DataMgr& dataManager);
    void handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                       boost::shared_ptr<fpi::OpenVolumeMsg>& message);
    void handleQueueItem(DmRequest* dmRequest);
    void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                        boost::shared_ptr<fpi::OpenVolumeMsg>& message,
                        Error const& e, DmRequest* dmRequest);
};

/**
 * Message from coordinator notifying the active state of the group
 */
struct VolumegroupUpdateHandler : Handler {
    explicit VolumegroupUpdateHandler(DataMgr& dataManager);
    void handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                       boost::shared_ptr<fpi::VolumeGroupInfoUpdateCtrlMsg>& message);
    void handleQueueItem(DmRequest* dmRequest);
    void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                        Error const& e, DmRequest* dmRequest);
};

/**
 * Close an existing access token for the given volume
 */
struct VolumeCloseHandler : Handler {
    explicit VolumeCloseHandler(DataMgr& dataManager);
    void handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                       boost::shared_ptr<fpi::CloseVolumeMsg>& message);
    void handleQueueItem(DmRequest* dmRequest);
    void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                        boost::shared_ptr<fpi::CloseVolumeMsg>& message,
                        Error const& e, DmRequest* dmRequest);
};

/**
 * Reload the volume. Close and re-instantiate leveldb for specified volume
 */
struct ReloadVolumeHandler : Handler {
    explicit ReloadVolumeHandler(DataMgr& dataManager);
    void handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                       boost::shared_ptr<fpi::ReloadVolumeMsg>& message);
    void handleQueueItem(DmRequest* dmRequest);
    void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                        boost::shared_ptr<fpi::ReloadVolumeMsg>& message,
                        Error const& e, DmRequest* dmRequest);
};

struct LoadFromArchiveHandler : Handler {
    explicit LoadFromArchiveHandler(DataMgr& dataManager);
    void handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                       boost::shared_ptr<fpi::LoadFromArchiveMsg>& message);
    void handleQueueItem(DmRequest* dmRequest);
    void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                        boost::shared_ptr<fpi::LoadFromArchiveMsg>& message,
                        Error const& e, DmRequest* dmRequest);
};


/**
 * DmMigration starting point handler from OM
 */
struct DmMigrationHandler : Handler {
    explicit DmMigrationHandler(DataMgr& dataManager);
    void handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                       boost::shared_ptr<fpi::CtrlNotifyDMStartMigrationMsg>& message);
    void handleQueueItem(DmRequest* dmRequest);
    void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                        boost::shared_ptr<fpi::CtrlNotifyDMStartMigrationMsg>& message,
                        Error const& e, DmRequest* dmRequest);
    void handleResponseReal(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                            uint64_t dmtVersion,
                            const Error& e);
};

/**
 * Handler for the initial Executor to the client
 */
struct DmMigrationBlobFilterHandler : Handler {
	explicit DmMigrationBlobFilterHandler(DataMgr &dataManager);
	void handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
			boost::shared_ptr<fpi::CtrlNotifyInitialBlobFilterSetMsg>& message);
    void handleQueueItem(DmRequest* dmRequest);
    void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                        boost::shared_ptr<fpi::CtrlNotifyInitialBlobFilterSetMsg>& message,
                        Error const& e, DmRequest* dmRequest);
    void handleResponseCleanUp(Error const& e, DmRequest* dmRequest);
};

/**
 * Handlers for the Blob and Blob Descriptor transfers
 */
struct DmMigrationDeltaBlobDescHandler : Handler {
	explicit DmMigrationDeltaBlobDescHandler(DataMgr &dataManager);
	void handleRequest(fpi::AsyncHdrPtr& asyncHdr, fpi::CtrlNotifyDeltaBlobDescMsgPtr& message);
    void handleQueueItem(DmRequest* dmRequest);
    void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                        fpi::CtrlNotifyDeltaBlobDescMsgPtr& message,
                        Error const& e, DmRequest* dmRequest);
    void handleResponseReal(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                        Error const& e);
};

struct DmMigrationDeltaBlobHandler : Handler {
    explicit DmMigrationDeltaBlobHandler(DataMgr& dataManager);
    void handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                       boost::shared_ptr<fpi::CtrlNotifyDeltaBlobsMsg>& message);
    void handleQueueItem(DmRequest* dmRequest);
    void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                        boost::shared_ptr<fpi::CtrlNotifyDeltaBlobsMsg>& message,
                        Error const& e, DmRequest* dmRequest);
    void handleCompletion(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                          boost::shared_ptr<fpi::CtrlNotifyDeltaBlobsMsg>& message,
                          Error const& e, DmRequest* dmRequest);
    void volumeCatalogCb(Error const& e, blob_version_t blob_version,
                         BlobObjList::const_ptr const& blob_obj_list,
                         MetaDataList::const_ptr const& meta_list,
                         fds_uint64_t const blobSize,
                         DmIoCommitBlobTx* commitBlobReq);
};

struct DmMigrationTxStateHandler : Handler {
    explicit DmMigrationTxStateHandler(DataMgr& dataManager);
    void handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                       boost::shared_ptr<fpi::CtrlNotifyTxStateMsg>& message);
    void handleQueueItem(DmRequest* dmRequest);
    void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                        boost::shared_ptr<fpi::CtrlNotifyTxStateMsg>& message,
                        Error const& e, DmRequest* dmRequest);
};

struct DmMigrationRequestTxStateHandler : Handler {
    explicit DmMigrationRequestTxStateHandler(DataMgr& dataManager);
    void handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                       boost::shared_ptr<fpi::CtrlNotifyRequestTxStateMsg>& message);
    void handleQueueItem(DmRequest* dmRequest);
    void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                        boost::shared_ptr<fpi::CtrlNotifyRequestTxStateMsg>& message,
                        Error const& e, DmRequest* dmRequest);
};

struct SimpleHandler : Handler {
    explicit SimpleHandler(DataMgr& dataManager);
    DECL_ASYNC_HANDLER(handleStartRefScanRequest, StartRefScanMsg);
};


}  // namespace dm
}  // namespace fds
#endif  // SOURCE_DATA_MGR_INCLUDE_DMHANDLER_H_
