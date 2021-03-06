/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 * Classes for DM requests that go through QoS queues
 */

#ifndef SOURCE_DATA_MGR_INCLUDE_DMIOREQ_H_
#define SOURCE_DATA_MGR_INCLUDE_DMIOREQ_H_

#include <string>
#include <fds_error.h>
#include <fds_types.h>
#include <fds_volume.h>
#include <fds_resource.h>
#include <fdsp/FDSP_types.h>
#include <fds_typedefs.h>
#include <blob/BlobTypes.h>
#include <fdsp/dm_api_types.h>
#include <fdsp/vc_api_types.h>
#include <fdsp/fds_stream_types.h>
#include <net/PlatNetSvcHandler.h>
#include <net/volumegroup_extensions.h>
#include <PerfTrace.h>

#define FdsDmSysTaskId      fds_volid_t(0x8fffffff)
#define FdsDmSysTaskPrio    5

namespace fds {

// Some logging routines have external linkage
// ======
extern std::string logString(const FDS_ProtocolInterface::AbortBlobTxMsg& abortBlobTx);
extern std::string logString(const FDS_ProtocolInterface::CommitBlobTxMsg& commitBlobTx);
extern std::string logString(const FDS_ProtocolInterface::DeleteBlobMsg& msg);
extern std::string logString(const FDS_ProtocolInterface::GetBlobMetaDataMsg& msg);
extern std::string logString(const FDS_ProtocolInterface::GetBucketMsg& msg);
extern std::string logString(const FDS_ProtocolInterface::GetBucketRspMsg& msg);
extern std::string logString(const FDS_ProtocolInterface::StatVolumeMsg& msg);
extern std::string logString(const FDS_ProtocolInterface::SetVolumeMetadataMsg& msg);
extern std::string logString(const FDS_ProtocolInterface::RenameBlobMsg& msg);
extern std::string logString(const FDS_ProtocolInterface::RenameBlobRespMsg& msg);
extern std::string logString(const FDS_ProtocolInterface::GetVolumeMetadataMsgRsp& msg);
extern std::string logString(const FDS_ProtocolInterface::QueryCatalogMsg& qryCat);
extern std::string logString(const FDS_ProtocolInterface::SetBlobMetaDataMsg& msg);
extern std::string logString(const FDS_ProtocolInterface::StartBlobTxMsg& stBlobTx);
extern std::string logString(const FDS_ProtocolInterface::UpdateCatalogMsg& updCat);
extern std::string logString(const FDS_ProtocolInterface::UpdateCatalogRspMsg& updCat);
extern std::string logString(const FDS_ProtocolInterface::UpdateCatalogOnceMsg& updCat);
extern std::string logString(const FDS_ProtocolInterface::UpdateCatalogOnceRspMsg& updCat);
extern std::string logString(const FDS_ProtocolInterface::OpenVolumeMsg& msg);
extern std::string logString(const FDS_ProtocolInterface::VolumeGroupInfoUpdateCtrlMsg& msg);
extern std::string logString(const FDS_ProtocolInterface::CloseVolumeMsg& msg);
extern std::string logString(const FDS_ProtocolInterface::ReloadVolumeMsg& msg);
extern std::string logString(const FDS_ProtocolInterface::CtrlNotifyDMStartMigrationMsg& msg);
extern std::string logString(const FDS_ProtocolInterface::CtrlNotifyInitialBlobFilterSetMsg& msg);
extern std::string logString(const fpi::CtrlNotifyDeltaBlobDescMsg &msg);
extern std::string logString(const fpi::CtrlNotifyFinishMigrationMsg &msg);
extern std::string logString(const fpi::CheckVolumeMetaDataMsg &msg);
// ======

class DmRequest : public FDS_IOType {
  public:
    fds_volid_t  volId;
    std::string blob_name;
    blob_version_t blob_version;
    std::string session_uuid;
    BlobTxId::const_ptr blobTxId;
    int64_t             opId;
    int32_t             version;
    std::function<void(const Error &e, DmRequest *dmRequest)> cb = NULL;
    std::function<void(DmRequest * req)> proc = NULL;

    fpi::FDSPMsgTypeId          reqMessageType; 
    Error                       respStatus;
    fpi::FDSPMsgTypeId          respMessageType; 

    DmRequest(fds_volid_t  _volId,
             const std::string &_blobName,
             std::string  _session_uuid,
             blob_version_t _blob_version,
             fds_io_op_t  _ioType)
            : volId(_volId),
              blob_name(_blobName),
              session_uuid(_session_uuid),
              version(VolumeGroupConstants::VERSION_INVALID) {
        io_req_id = 0;
        io_type = _ioType;
        io_vol_id = _volId;
        blob_version = _blob_version;

        // Set QoS wait latency counter
        opQoSWaitCtx.reset_volid(io_vol_id);
        opQoSWaitCtx.type = PerfEventType::DM_QOS_REQ_WAIT;

        // Set end-to-end latency counter.
        // The specific latency request type will be set by
        // the derived type.
        opReqLatencyCtx.reset_volid(volId);
        PerfTracer::tracePointBegin(opReqLatencyCtx);
    }

    DmRequest() {
        io_req_id = 0;
    }

    fds_volid_t getVolId() const {
        return volId;
    }

    virtual ~DmRequest() {
        // End the end-to-end request latency timer
        // now that the request is being destroyed.
        // This assumes that the request is destroyed
        // only when and right when the request is completed.
        PerfTracer::tracePointEnd(opReqLatencyCtx);
    }

    void setBlobVersion(blob_version_t version) {
        blob_version = version;
    }
    void setBlobTxId(BlobTxId::const_ptr id) {
        blobTxId = id;
    }

    BlobTxId::const_ptr getBlobTxId() const {
        return blobTxId;
    }
    void setOpId(const int64_t &id) {
        opId = id;
    }
    int64_t getOpId() const {
        return opId;
    }
    virtual void sendSvcResponse(PlatNetSvcHandler *handler,
                                 const SHPTR<fpi::AsyncHdr>& asyncHdr) {
        /* Once necessary subclasses implement this function we can make
         * this pure virtual 
         */
        fds_panic("This function must be overridden inorder to be used");
    }

    // Why is this not a ostream operator?
    virtual std::string log_string() const {
        std::stringstream ret;
        ret << "DmRequest for vol " << std::hex << volId
            << std::dec << " io_type " << io_type;
        return ret.str();
    }
};

/**
 * Handler that process DmRequest
 */
class DmIoReqHandler {
  public:
    virtual Error enqueueMsg(fds_volid_t volId, DmRequest* ioReq) = 0;
};

/**
 * Request to make a snapshot of a volume db
 */
class DmIoSnapVolCat : public DmRequest {
  public:
    // TODO(xxx) what other params do we need?
    typedef std::function<void (fds_volid_t volid,
                                const Error& error)> CbType;

  public:
    DmIoSnapVolCat() {
    }

    // Why is this not a ostream operator?
    virtual std::string log_string() const override {
        std::stringstream ret;
        ret << "dmIoSnapVolCat for vol "
            << std::hex << volId << std::dec << " first rsync? "
            << (io_type == FDS_DM_SNAP_VOLCAT);
        return ret.str();
    }

    // volume is part of base class: use getVolId()
    /* response callback */
    CbType dmio_snap_vcat_cb;
    NodeUuid node_uuid;
};

/**
 * Request that marks close DMT for a volume
 * Will send PushMeta done msg to destination DM
 */
class DmIoPushMetaDone : public DmRequest {
  public:
    explicit DmIoPushMetaDone(fds_volid_t _volId)
            : DmRequest(_volId, "", "", blob_version_invalid,
                        FDS_DM_PUSH_META_DONE) {
    }

    // Why is this not a ostream operator?
    virtual std::string log_string() const override {
        std::stringstream ret;
        ret << "dmIoPushMetaDone for vol "
            << std::hex << volId << std::dec;
        return ret.str();
    }

    // volume is part of base class: use getVolId()
};

/**
 * Request that marks push meta done on receiving side
 */
class DmIoMetaRecvd : public DmRequest {
  public:
    explicit DmIoMetaRecvd(fds_volid_t _volId)
            : DmRequest(_volId, "", "", blob_version_invalid,
                        FDS_DM_META_RECVD) {
    }
    friend std::ostream& operator<<(std::ostream& out, const DmIoMetaRecvd& io) {
        return out << "DmIoMetaRecvd vol " << std::hex << io.volId << std::dec;
    }
    // volume is part of base class: use getVolId()
};

/**
 * Request to Commit Blob Tx
 */
class DmIoCommitBlobTx : public DmRequest {
  public:
    typedef std::function<void (const Error &e, DmIoCommitBlobTx *blobTx)> CbType;
    fpi::CommitBlobTxRspMsg rspMsg;

  public:
    DmIoCommitBlobTx(const fds_volid_t  &_volId,
                     const std::string &_blobName,
                     const blob_version_t &_blob_version,
                     fds_uint64_t _dmt_version,
                     const sequence_id_t _seq_id,
                     int64_t opId)
            : DmRequest(_volId, _blobName, "", _blob_version, FDS_COMMIT_BLOB_TX) {
        dmt_version = _dmt_version;
        sequence_id = _seq_id;

        // perf-trace related data
        opReqFailedPerfEventType = PerfEventType::DM_TX_COMMIT_REQ_ERR;
        opReqLatencyCtx.type = PerfEventType::DM_TX_COMMIT_REQ;
        setOpId(opId);
    }
    virtual ~DmIoCommitBlobTx() {}

    std::function<void(const Error &e)> localCb;

    friend std::ostream& operator<<(std::ostream& out, const DmIoCommitBlobTx& io) {
        return out << "DmIoCommitBlobTx vol " << std::hex << io.volId << std::dec
                   << " blob " << io.blob_name
                   << ", dmt_version " << io.dmt_version << " TxId: " << *(io.ioBlobTxDesc)
                   << " opid: " << io.opId
                   << " sequenceid: " << io.sequence_id;
    }

    virtual std::string log_string() const override {
        std::stringstream ret;
        ret << *this;
        return ret.str();
    }

    std::string dump_meta() const {
        std::stringstream ret;
        ret << "Meta list byte-count = " << rspMsg.byteCount;
        for (auto cit : rspMsg.meta_list) {
            ret << "Meta list key: " << cit.key << " value: " << cit.value;
        }
        return ret.str();
    }

    BlobTxId::const_ptr ioBlobTxDesc;
    fds_uint64_t dmt_version;
    sequence_id_t sequence_id;
    bool usedForMigration {false}; // is this used for migration? If so, keep count.
    std::mutex migrClientCntMtx; // Used to ensure atomicity for dealing with counts
    fds_uint64_t migrClientCnt; // Keep track of the outstanding clients still forwarding
    CbType dmio_commit_blob_tx_resp_cb; // response callback
    bool orig_request {true}; // is this the original request?
};

template <typename T>
class DmIoCommitBlobOnce : public  DmIoCommitBlobTx {
  public:
    using DmIoCommitBlobTx::DmIoCommitBlobTx;
    T *parent;
};

/**
 * Request to Abort Blob Tx
 */
class DmIoAbortBlobTx : public DmRequest {
  public:
    typedef std::function<void (const Error &e, DmIoAbortBlobTx *blobTx)> CbType;

  public:
    DmIoAbortBlobTx(const fds_volid_t  &_volId,
                    const std::string &_blobName,
                    const blob_version_t &_blob_version,
                    int64_t opId)
            : DmRequest(_volId, _blobName, "", _blob_version, FDS_ABORT_BLOB_TX) {
        // perf-trace related data
        opReqFailedPerfEventType = PerfEventType::DM_TX_OP_REQ_ERR;
        opReqLatencyCtx.type = PerfEventType::DM_TX_ABORT_REQ;
        setOpId(opId);
    }

    virtual std::string log_string() const override {
        std::stringstream ret;
        ret << "DmIoAbortBlobTx vol "
            << std::hex << volId << std::dec
            << " opid: " << opId;
        return ret.str();
    }

    BlobTxId::const_ptr ioBlobTxDesc;
    /* response callback */
    CbType dmio_abort_blob_tx_resp_cb;
};

/**
 * Request to Start Blob Tx
 */
class DmIoStartBlobTx : public DmRequest {
  public:
    typedef std::function<void (const Error &e, DmIoStartBlobTx *blobTx)> CbType;

    DmIoStartBlobTx(const fds_volid_t  &_volId,
                    const std::string &_blobName,
                    const blob_version_t &_blob_version,
                    const fds_int32_t _blob_mode,
                    const fds_uint64_t _dmt_ver,
                    int64_t opId)
            : DmRequest(_volId, _blobName,
                        "", _blob_version, FDS_START_BLOB_TX), blob_mode(_blob_mode),
              dmt_version(_dmt_ver) {
        // perf-trace related data
        opReqFailedPerfEventType = PerfEventType::DM_TX_OP_REQ_ERR;
        opReqLatencyCtx.type = PerfEventType::DM_TX_START_REQ;
        setOpId(opId);
    }

    friend std::ostream& operator<<(std::ostream& out, const DmIoStartBlobTx& io) {
        return out << "DmIoStartBlobTx vol " << std::hex << io.volId << std::dec
                   << " blob " << io.blob_name << " blob mode " << io.blob_mode
                   << ", dmt_version " << io.dmt_version << " TxId: " << *(io.ioBlobTxDesc)
                   << " opid: " << io.opId;
    }

    virtual std::string log_string() const override {
        std::stringstream ret;
        ret << *this;
        return ret.str();
    }

    BlobTxId::const_ptr ioBlobTxDesc;
    fds_uint64_t dmt_version;
    fds_int32_t blob_mode;
    /* response callback */
    CbType dmio_start_blob_tx_resp_cb;
};

/**
 * Request to query catalog
 */
class DmIoQueryCat : public DmRequest {
  public:
    typedef std::function<void (const Error &e, DmIoQueryCat *req)> CbType;
    boost::shared_ptr<fpi::QueryCatalogMsg> queryMsg;

  public:
    explicit DmIoQueryCat(boost::shared_ptr<fpi::QueryCatalogMsg>& qMsg)
            : DmRequest(fds_volid_t(qMsg->volume_id),
                        qMsg->blob_name,
                        "",
                        qMsg->blob_version,
                        FDS_CAT_QRY),
              queryMsg(qMsg) {
        // Set the error event type
        opReqFailedPerfEventType = PerfEventType::DM_QUERY_REQ_ERR;
        // Set the end-to-end latency counter type
        opReqLatencyCtx.type = PerfEventType::DM_QUERY_REQ;
    }

    virtual std::string log_string() const override {
        std::stringstream ret;
        ret << "DmIoQueryCat "
            << std::hex << volId << std::dec;
        return ret.str();
    }

    /* response callback */
    CbType dmio_querycat_resp_cb;
};

/**
 * New request to update catalog
 */
class DmIoFwdCat : public DmRequest {
  public:
    NodeUuid srcUuid;

    explicit DmIoFwdCat(NodeUuid _src, boost::shared_ptr<fpi::ForwardCatalogMsg>& fwdMsg, bool use_sys_queue)
            : DmRequest(use_sys_queue ? FdsDmSysTaskId : fds_volid_t(fwdMsg->volume_id), "", "",
                        0, FDS_DM_FWD_CAT_UPD),
              srcUuid(_src),
              fwdCatMsg(fwdMsg) {}

    virtual std::string log_string() const override {
        std::stringstream ret;
        ret << "DmIoFwdCat vol " << std::hex << fwdCatMsg->volume_id << std::dec;
        return ret.str();
    }

    friend std::ostream& operator<<(std::ostream& out, const DmIoFwdCat& io) {
        return out << "DmIoFwdCat vol " << std::hex << io.fwdCatMsg->volume_id << std::dec
                   << " blob " << io.fwdCatMsg->blob_name
                   << " blob version " << io.fwdCatMsg->blob_version
                   << " final?: " << io.fwdCatMsg->lastForward;
    }

    boost::shared_ptr<fpi::ForwardCatalogMsg> fwdCatMsg;
};

/**
 * Request to update catalog
 */
class DmIoUpdateCat : public DmRequest {
  public:
    typedef std::function<void (const Error &e, DmIoUpdateCat *req)> CbType;

  public:
    explicit DmIoUpdateCat(boost::shared_ptr<fpi::UpdateCatalogMsg>& _updcatMsg)
            : DmRequest(fds_volid_t(_updcatMsg->volume_id), _updcatMsg->blob_name, "", _updcatMsg->blob_version,
                        FDS_CAT_UPD), ioBlobTxDesc(new BlobTxId(_updcatMsg->txId)),
              obj_list(_updcatMsg->obj_list), updcatMsg(_updcatMsg) {
        // perf-trace related data
        opReqFailedPerfEventType = PerfEventType::DM_TX_OP_REQ_ERR;
        opReqLatencyCtx.type = PerfEventType::DM_TX_UPDATE_REQ;
        setOpId(_updcatMsg->opId);
    }

    friend std::ostream& operator<<(std::ostream& out, const DmIoUpdateCat& io) {
        return out << "DmIoUpdateCat vol " << std::hex << io.volId << std::dec
                   << " blob " << io.blob_name <<  " TxId: " << *(io.ioBlobTxDesc)
                   << " opid: " << io.opId;
    }
    virtual std::string log_string() const override {
        std::stringstream ret;
        ret << *this;
        return ret.str();
    }

    BlobTxId::const_ptr ioBlobTxDesc;
    const fpi::FDSP_BlobObjectList & obj_list;
    boost::shared_ptr<fpi::UpdateCatalogMsg> updcatMsg;

    /* response callback */
    CbType dmio_updatecat_resp_cb;
};

/**
 * Request to update catalog in a single request.
 */
class DmIoUpdateCatOnce : public DmRequest {
  public:
    explicit DmIoUpdateCatOnce(
        boost::shared_ptr<fpi::UpdateCatalogOnceMsg>& _updcatMsg,
        DmIoCommitBlobTx *_commitReq)
            : DmRequest(fds_volid_t(_updcatMsg->volume_id),
                        _updcatMsg->blob_name,
                        "",
                        _updcatMsg->blob_version,
                        FDS_CAT_UPD_ONCE),
              ioBlobTxDesc(new BlobTxId(_updcatMsg->txId)),
              commitBlobReq(_commitReq),
              updcatMsg(_updcatMsg) {
        // perf-trace related data
        opReqFailedPerfEventType = PerfEventType::DM_TX_COMMIT_REQ_ERR;
        opReqLatencyCtx.type = PerfEventType::DM_UPDATE_ONCE_REQ;
        setOpId(_updcatMsg->opId);
    }

    friend std::ostream& operator<<(std::ostream& out, const DmIoUpdateCatOnce& io) {
        return out << "DmIoUpdateCatOnce vol " << std::hex << io.volId << std::dec
                   << " blob " << io.blob_name <<  " TxId: " << *(io.ioBlobTxDesc)
                   << " opid: " << io.opId
                   << " sequenceid: " << io.updcatMsg->sequence_id;
    }
    virtual std::string log_string() const override {
        std::stringstream ret;
        ret << *this;
        return ret.str();
    }

    BlobTxId::const_ptr ioBlobTxDesc;
    DmIoCommitBlobTx *commitBlobReq;
    boost::shared_ptr<fpi::UpdateCatalogOnceMsg> updcatMsg;

    // response callback
    typedef std::function<void (const Error &e, DmIoUpdateCatOnce *req)> CbType;
    CbType dmio_updatecat_resp_cb;
};

/**
 * Request to set blob metadata
 */
class DmIoSetBlobMetaData : public DmRequest {
  public:
    typedef std::function<void (const Error &e, DmIoSetBlobMetaData *req)> CbType;

  public:
    explicit DmIoSetBlobMetaData(boost::shared_ptr<fpi::SetBlobMetaDataMsg> & _setMDMsg)
            : DmRequest(fds_volid_t(_setMDMsg->volume_id), _setMDMsg->blob_name, "", _setMDMsg->blob_version,
                        FDS_SET_BLOB_METADATA), ioBlobTxDesc(new BlobTxId(_setMDMsg->txId)),
              md_list(_setMDMsg->metaDataList), setMDMsg(_setMDMsg) {
        // perf-trace related data
        opReqFailedPerfEventType = PerfEventType::DM_TX_OP_REQ_ERR;
        opReqLatencyCtx.type = PerfEventType::DM_TX_SET_BLOB_META_REQ;
        setOpId(_setMDMsg->opId);
    }

    friend std::ostream& operator<<(std::ostream& out, const DmIoSetBlobMetaData& io) {
        return out << "DmIoSetBlobMetaData vol " << std::hex << io.volId << std::dec
                   << " blob " << io.blob_name <<  " TxId: " << *(io.ioBlobTxDesc)
                   << " opid: " << io.opId;
    }
    virtual std::string log_string() const override {
        std::stringstream ret;
        ret << *this;
        return ret.str();
    }
    BlobTxId::const_ptr ioBlobTxDesc;
    const fpi::FDSP_MetaDataList & md_list;
    boost::shared_ptr<const fpi::SetBlobMetaDataMsg> setMDMsg;  // keep the reference
    /* response callback */
    CbType dmio_setmd_resp_cb;
};

/**
 * Request to delete catalog
 */
class DmIoDeleteCat : public DmRequest {
  public:
    typedef std::function<void (const Error &e, DmIoDeleteCat *req)> CbType;
  public:
    DmIoDeleteCat(const fds_volid_t  &_volId,
                  const std::string &_blobName,
                  const blob_version_t &_blob_version)
            : DmRequest(_volId, _blobName, "", _blob_version, FDS_DELETE_BLOB_SVC) {
    }

    virtual std::string log_string() const override {
        std::stringstream ret;
        ret << "DmIoDeleteCat vol "
            << std::hex << volId << std::dec;
        return ret.str();
    }

    /* response callback */
    CbType dmio_deletecat_resp_cb;
};

class DmIoGetBlobMetaData : public DmRequest {
  public:
    typedef std::function<void (const Error &e, DmIoGetBlobMetaData *req)> CbType;

  public:
    DmIoGetBlobMetaData(const fds_volid_t  &_volId,
                        const std::string &_blobName,
                        const blob_version_t &_blob_version,
                        boost::shared_ptr<fpi::GetBlobMetaDataMsg> message)
            : message(message), DmRequest(_volId, _blobName,
                                          "", _blob_version, FDS_GET_BLOB_METADATA) {
        // perf-trace related data
        opReqFailedPerfEventType = PerfEventType::DM_QUERY_REQ_ERR;
        opReqLatencyCtx.type = PerfEventType::DM_GET_BLOB_META_REQ;
    }

    ~DmIoGetBlobMetaData() {
    }

    virtual std::string log_string() const override {
        std::stringstream ret;
        ret << "DmIoGetBlobMetaData vol "
            << std::hex << volId << std::dec;
        return ret.str();
    }
    boost::shared_ptr<fpi::GetBlobMetaDataMsg> message;
    /* response callback */
    CbType dmio_getmd_resp_cb;
};

class DmIoRenameBlob : public DmRequest {
  public:
    typedef std::function<void (const Error &e, DmIoRenameBlob *req)> CbType;

    DmIoRenameBlob(const fds_volid_t& _volId,
                   const std::string& _oldName,
                   const fds_uint64_t _seqId,
                   boost::shared_ptr<fpi::RenameBlobMsg> _message)
            : message(_message), DmRequest(_volId, _oldName, "", blob_version_invalid, FDS_RENAME_BLOB),
              seq_id(_seqId)
    {
        // perf-trace related data
        opReqFailedPerfEventType = PerfEventType::DM_QUERY_REQ_ERR;
        opReqLatencyCtx.type = PerfEventType::DM_RENAME_BLOB_REQ;
        setOpId(_message->opId);
    }

    ~DmIoRenameBlob() {
    }

    virtual std::string log_string() const override {
        std::stringstream ret;
        ret << "DmIoRenameBlob vol "
            << std::hex << volId << std::dec
            << " old name: " << blob_name
            << " new name: " << message->destination_blob
            << " opid: " << opId
            << " sequenceid: " << message->sequence_id;
        return ret.str();
    }
    DmIoCommitBlobOnce<DmIoRenameBlob> *commitReq;
    boost::shared_ptr<fpi::RenameBlobMsg> message;
    boost::shared_ptr<fpi::RenameBlobRespMsg> response;
    fds_uint64_t const seq_id;
    /* response callback */
    CbType dmio_getmd_resp_cb;
};

struct DmIoStatVolume : DmRequest {
    typedef std::function<void (const Error &e, DmIoStatVolume *req)> CbType;

    explicit DmIoStatVolume(boost::shared_ptr<fpi::StatVolumeMsg> message)
            : DmRequest(fds_volid_t(message->volume_id), "", "", 0, FDS_STAT_VOLUME), msg(message) {
        // perf-trace related data
        opReqFailedPerfEventType = PerfEventType::DM_QUERY_REQ_ERR;
        opReqLatencyCtx.type = PerfEventType::DM_STAT_VOL_REQ;
    }

    boost::shared_ptr<fpi::StatVolumeMsg> msg;
    // response callback
    CbType dmio_get_volmd_resp_cb;
};

struct DmIoSetVolumeMetaData : DmRequest {
    typedef std::function<void (const Error &e, DmIoSetVolumeMetaData *req)> CbType;

    explicit DmIoSetVolumeMetaData(boost::shared_ptr<fpi::SetVolumeMetadataMsg> message)
            : DmRequest(fds_volid_t(message->volumeId), "", "", 0, FDS_SET_VOLUME_METADATA),
              msg(message)
    {
        opReqFailedPerfEventType = PerfEventType::DM_QUERY_REQ_ERR;
        opReqLatencyCtx.type = PerfEventType::DM_SET_VOL_META_REQ;
        setOpId(msg->opId);
    }

    boost::shared_ptr<fpi::SetVolumeMetadataMsg> msg;
    // response callback
    CbType dmio_set_volmd_resp_cb;
};

struct DmIoGetVolumeMetadata : DmRequest {
    typedef std::function<void (const Error &e, DmIoGetVolumeMetadata *req)> CbType;

    explicit DmIoGetVolumeMetadata(fds_volid_t volId, boost::shared_ptr<fpi::GetVolumeMetadataMsgRsp> message)
            : DmRequest(volId, "", "", 0, FDS_GET_VOLUME_METADATA), msg(message) {
        opReqFailedPerfEventType = PerfEventType::DM_QUERY_REQ_ERR;
        opReqLatencyCtx.type = PerfEventType::DM_GET_VOL_META_REQ;
    }

    boost::shared_ptr<fpi::GetVolumeMetadataMsgRsp> msg;
    // response callback
    CbType dmio_get_volmd_resp_cb;
};

struct DmIoVolumeOpen : DmRequest {
    typedef std::function<void (const Error &e, DmIoVolumeOpen *req)> CbType;

    boost::shared_ptr<fpi::OpenVolumeMsg> msg;
    fpi::SvcUuid const client_uuid_;

    explicit DmIoVolumeOpen(boost::shared_ptr<fpi::OpenVolumeMsg> message,
                            fpi::SvcUuid const& client_uuid)
            : DmRequest(fds_volid_t(message->volume_id), "", "", 0, FDS_OPEN_VOLUME),
              msg(message),
              client_uuid_(client_uuid),
              token(message->token),
              access_mode(message->mode) {
        opReqFailedPerfEventType = PerfEventType::DM_QUERY_REQ_ERR;
        opReqLatencyCtx.type = PerfEventType::DM_OPEN_VOL_REQ;
    }

    fds_int64_t             token;
    fpi::VolumeAccessMode   access_mode;
    sequence_id_t           sequence_id;
    int32_t                 version;
    fpi::VolumeGroupCoordinatorInfo coordinator; 

    // response callback
    CbType                  dmio_get_volmd_resp_cb;
};

struct DmIoVolumeClose : DmRequest {
    typedef std::function<void (const Error &e, DmIoVolumeClose *req)> CbType;

    boost::shared_ptr<fpi::CloseVolumeMsg> msg;

    explicit DmIoVolumeClose(boost::shared_ptr<fpi::CloseVolumeMsg> message)
            : DmRequest(fds_volid_t(message->volume_id), "", "", 0, FDS_CLOSE_VOLUME),
              msg(message),
              token(message->token) {
        opReqFailedPerfEventType = PerfEventType::DM_QUERY_REQ_ERR;
        opReqLatencyCtx.type = PerfEventType::DM_CLOSE_VOL_REQ;
    }

    fds_int64_t token;
    // response callback
    CbType dmio_get_volmd_resp_cb;
};

/**
 * stat from a single module for volume(s)
 */
class DmIoStatStream : public DmRequest {
  public:
    typedef std::function<void (const Error &e, DmIoStatStream *req)> CbType;
    DmIoStatStream(fds_volid_t volid, boost::shared_ptr<fpi::StatStreamMsg>& statMsg)
            : DmRequest(volid, "", "", 1,
                        FDS_DM_STAT_STREAM), statStreamMsg(statMsg) {}

    friend std::ostream& operator<<(std::ostream& out, const DmIoStatStream& io) {
        return out << "DmIoStatStream timestamp " << (io.statStreamMsg)->start_timestamp;
    }

    boost::shared_ptr<fpi::StatStreamMsg> statStreamMsg;
    CbType dmio_statstream_resp_cb;
};


struct DmIoGetSysStats : DmRequest {
    boost::shared_ptr<fpi::GetDmStatsMsg> message;
    explicit DmIoGetSysStats(boost::shared_ptr<fpi::GetDmStatsMsg> message)
            : message(message) , DmRequest(fds_volid_t(message->volume_id), "", "", 0, FDS_DM_SYS_STATS) {}
};

struct DmIoGetBucket : DmRequest {
    boost::shared_ptr<fpi::GetBucketMsg> message;
    boost::shared_ptr<fpi::GetBucketRspMsg> response;
    explicit DmIoGetBucket(boost::shared_ptr<fpi::GetBucketMsg> message)
            : message(message),
              response(new fpi::GetBucketRspMsg()),
              DmRequest(fds_volid_t(message->volume_id), "", "", 0, FDS_LIST_BLOB) {
        opReqFailedPerfEventType = PerfEventType::DM_QUERY_REQ_ERR;
        opReqLatencyCtx.type = PerfEventType::DM_GET_VOL_CONTENTS_REQ;
    }
};

struct DmIoDeleteBlob: DmRequest {
    boost::shared_ptr<fpi::DeleteBlobMsg> message;
    explicit DmIoDeleteBlob(boost::shared_ptr<fpi::DeleteBlobMsg> message)
            : message(message) , DmRequest(fds_volid_t(message->volume_id),
                                           message->blob_name,
                                           "",
                                           message->blob_version,
                                           FDS_DELETE_BLOB) {
        opReqFailedPerfEventType = PerfEventType::DM_QUERY_REQ_ERR;
        opReqLatencyCtx.type = PerfEventType::DM_DELETE_BLOB_REQ;
        setOpId(message->opId);
    }
};

struct DmIoReloadVolume : DmRequest {
    boost::shared_ptr<fpi::ReloadVolumeMsg> message;
    boost::shared_ptr<fpi::ReloadVolumeRspMsg> response;
    explicit DmIoReloadVolume(boost::shared_ptr<fpi::ReloadVolumeMsg> msg)
            : message(msg),
              response(new fpi::ReloadVolumeRspMsg()),
              DmRequest(fds_volid_t(msg->volume_id), "", "", 0, FDS_DM_RELOAD_VOLUME) {
    }

    friend std::ostream& operator<<(std::ostream& out, const DmIoReloadVolume& io) {
        return out << "DmIoReloadVolume vol " << std::hex << io.volId << std::dec;
    }

};

struct DmIoLoadFromArchive : DmRequest {
    boost::shared_ptr<fpi::LoadFromArchiveMsg> message;
    explicit DmIoLoadFromArchive(boost::shared_ptr<fpi::LoadFromArchiveMsg> msg)
            : message(msg),
              DmRequest(fds_volid_t(msg->volId), "", "", 0, FDS_DM_LOAD_FROM_ARCHIVE) {
    }

    friend std::ostream& operator<<(std::ostream& out, const DmIoLoadFromArchive& io) {
        return out << "DmIoLoadFromArchive vol:" << io.volId;
    }
};


struct DmIoMigration : DmRequest {
    boost::shared_ptr<fpi::CtrlNotifyDMStartMigrationMsg> message;
    std::function<void(const Error& e)> localCb = NULL;

    explicit DmIoMigration(fds_volid_t volid, boost::shared_ptr<fpi::CtrlNotifyDMStartMigrationMsg> msg)
            : message(msg),
              DmRequest(fds_volid_t(volid), "", "", 0, FDS_DM_MIGRATION) {
    }

    friend std::ostream& operator<<(std::ostream& out, const DmIoMigration& io) {
        return out << "DmIoMigration vol " << io.volId.get();
    }

};

struct DmIoResyncInitialBlob : DmRequest {
    boost::shared_ptr<fpi::CtrlNotifyInitialBlobFilterSetMsg> message;
    NodeUuid destNodeUuid;
    std::function<void(const Error& e)> localCb = NULL;
    explicit DmIoResyncInitialBlob(fds_volid_t volid, boost::shared_ptr<fpi::CtrlNotifyInitialBlobFilterSetMsg> msg,
                                   NodeUuid &_destNodeUuid)
            : message(msg),
              DmRequest(fds_volid_t(volid), "", "", 0, FDS_DM_RESYNC_INIT_BLOB),
              destNodeUuid(_destNodeUuid) {
    }

    friend std::ostream& operator<<(std::ostream& out, const DmIoResyncInitialBlob& io) {
        return out << "DmIoResyncInitialBlob vol " << io.volId.get();
    }
};

struct DmIoMigrationDeltaBlobs : public DmRequest {
  public:
    typedef std::function<void (const Error &e, DmIoMigrationDeltaBlobs *req)> CbType;
    NodeUuid srcUuid;
    explicit DmIoMigrationDeltaBlobs(NodeUuid _src, boost::shared_ptr<fpi::CtrlNotifyDeltaBlobsMsg>& msg, bool use_sys_queue)
            : DmRequest(use_sys_queue ? FdsDmSysTaskId : fds_volid_t(msg->volume_id), "", "", 0, FDS_DM_MIG_DELTA_BLOB),
              srcUuid(_src),
              deltaBlobsMsg(msg) {}

    virtual std::string log_string() const override {
        std::stringstream ret;
        ret << "DmIoMigrationDeltaBlobs vol " << std::hex << volId << std::dec;
        return ret.str();
    }

    friend std::ostream& operator<<(std::ostream& out, const DmIoMigrationDeltaBlobs& io) {
        return out << "DmIoMigrationDeltaBlobs vol="
                   << std::hex << io.deltaBlobsMsg->volume_id << std::dec;
    }

    boost::shared_ptr<fpi::CtrlNotifyDeltaBlobsMsg> deltaBlobsMsg;
    CbType dmio_fwdcat_resp_cb;
    inline ~DmIoMigrationDeltaBlobs() { LOGDEBUG << "Calling destructor for DmIoMigrationDeltaBlobs"; }
};

struct DmIoMigrationDeltaBlobDesc : DmRequest {
    std::function<void(const Error& e)> localCb = NULL;
    NodeUuid srcUuid;
    explicit DmIoMigrationDeltaBlobDesc(NodeUuid _src, const fpi::CtrlNotifyDeltaBlobDescMsgPtr &msg, bool use_sys_queue)
            : DmRequest(use_sys_queue ? FdsDmSysTaskId : fds_volid_t(msg->volume_id), "", "", 0, FDS_DM_MIG_DELTA_BLOBDESC),
              srcUuid(_src),
              deltaBlobDescMsg(msg)
    {
    }

    friend std::ostream& operator<<(std::ostream& out, const DmIoMigrationDeltaBlobDesc& io) {
        return out << "DmIoMigrationDeltaBlobDesc vol:"
                   << std::hex << io.deltaBlobDescMsg->volume_id << std::dec;
    }

    fpi::CtrlNotifyDeltaBlobDescMsgPtr deltaBlobDescMsg;
};

struct DmIoMigrationTxState : DmRequest {
    NodeUuid destUuid;
    explicit DmIoMigrationTxState(NodeUuid _dest, const fpi::CtrlNotifyTxStateMsgPtr &msg, bool use_sys_queue)
            : DmRequest(use_sys_queue ? FdsDmSysTaskId : fds_volid_t(msg->volume_id), "", "", 0, FDS_DM_MIG_TX_STATE),
              destUuid(_dest),
              txStateMsg(msg)
    {
    }

    friend std::ostream& operator<<(std::ostream& out, const DmIoMigrationTxState& io) {
        return out << "DmIoMigrationTxState vol:"
                   << std::hex << io.volId.get() << std::dec;
    }

    fpi::CtrlNotifyTxStateMsgPtr txStateMsg;
};

struct DmIoMigrationRequestTxState : DmRequest {
    explicit DmIoMigrationRequestTxState(fds_volid_t volId, const fpi::CtrlNotifyRequestTxStateMsgPtr &msg)
            : DmRequest(volId, "", "", 0, FDS_DM_MIG_REQ_TX_STATE),
              reqTxStateMsg(msg)
    {
    }

    friend std::ostream& operator<<(std::ostream& out, const DmIoMigrationRequestTxState& io) {
        return out << "DmIoMigrationRequestTxState vol:"
                   << std::hex << io.volId.get() << std::dec;
    }

    fpi::CtrlNotifyRequestTxStateMsgPtr reqTxStateMsg;
    fpi::CtrlNotifyRequestTxStateRspMsg rspMsg;
};

struct DmFunctor : DmRequest {
    DmFunctor(const fds_volid_t &volId, const std::function<void()>& f)
            : DmRequest(volId, "", "", 0, FDS_DM_FUNCTOR),
              func(f)
    {}

    std::function<void()>       func;
};

/**
 * Generic request holder for all volume related messages
 */
template <fds_io_op_t QosIoT,
          class ReqT,
          fpi::FDSPMsgTypeId ReqTypeId,
          class RespT,
          fpi::FDSPMsgTypeId RespTypeId>
struct DmVolumeReq : DmRequest {
    using ReqMsgT = ReqT;
    using RespMsgT = RespT;
    static const fpi::FDSPMsgTypeId reqMsgTypeId = ReqTypeId;

    DmVolumeReq(const fds_volid_t &volId, const SHPTR<ReqMsgT> &msg)
            : DmRequest(volId, "", "", 0, QosIoT)
    {
        reqMessageType = ReqTypeId;
        reqMessage = msg;
        respMessageType = RespTypeId;
        if (respMessageType != FDSP_MSG_TYPEID(fpi::EmptyMsg)) {
            respMessage = boost::make_shared<RespT>();
        }
    }

    std::string log_string() const
    {
        std::stringstream ss;
        ss << DmRequest::log_string() << fds::logString(*reqMessage);
        return ss.str();
    }
    virtual void sendSvcResponse(PlatNetSvcHandler *handler,
                                 const SHPTR<fpi::AsyncHdr>& asyncHdr) {
        asyncHdr->msg_code = static_cast<int32_t>(respStatus.GetErrno());
        if (respStatus == ERR_OK) {
            if (respMessage) {
                handler->sendAsyncResp(*asyncHdr, RespTypeId, *respMessage);
            } else {
                handler->sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::EmptyMsg), fpi::EmptyMsg());
            }
        } else {
            GLOGWARN << "Returning error response: " << respStatus
                << " header: " << fds::logString(*asyncHdr);
            handler->sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::EmptyMsg), fpi::EmptyMsg());
        }
    }

    SHPTR<ReqMsgT>              reqMessage;
    SHPTR<RespMsgT>             respMessage;
};
template <fds_io_op_t QosIoT,
          class ReqT,
          fpi::FDSPMsgTypeId ReqTypeId,
          class RespT,
          fpi::FDSPMsgTypeId RespTypeId>
const fpi::FDSPMsgTypeId DmVolumeReq<QosIoT, ReqT, ReqTypeId, RespT, RespTypeId>::reqMsgTypeId;

/**
 * NOTE: For these handlers, see actual redirecting calls at:
 * DataMgr::dmQosCtrl::processIO(FDS_IOType* _io)
 */

#define DECLARE_DM_VOLUMEREQ(QosIoT, ReqT, RespT) \
    DmVolumeReq<QosIoT, ReqT, FDSP_MSG_TYPEID(ReqT), RespT, FDSP_MSG_TYPEID(RespT)>
using DmIoVolumegroupUpdate = DECLARE_DM_VOLUMEREQ(FDS_DM_VOLUMEGROUP_UPDATE, \
                                                   fpi::VolumeGroupInfoUpdateCtrlMsg, \
                                                   fpi::EmptyMsg);

using DmIoFinishStaticMigration = DECLARE_DM_VOLUMEREQ(FDS_DM_MIG_FINISH_STATIC_MIGRATION, \
                                                   fpi::CtrlNotifyFinishMigrationMsg, \
                                                   fpi::EmptyMsg);

using DmIoVolumeCheck = DECLARE_DM_VOLUMEREQ(FDS_DM_VOLUME_CHK_MSG, \
                                             fpi::CheckVolumeMetaDataMsg, \
                                             fpi::CheckVolumeMetaDataRspMsg);

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DMIOREQ_H_
