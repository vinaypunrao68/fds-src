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
#include <fdsp/FDSP_types.h>
#include <fds_typedefs.h>
#include <blob/BlobTypes.h>

#define FdsDmSysTaskId      0x8fffffff
#define FdsDmSysTaskPrio    5

namespace fds {
    /* Forward declarations */
    class BlobNode;

    struct RequestHeader {
        fds_volid_t volId;
        long srcIp;  //NOLINT
        long dstIp;  //NOLINT
        fds_uint32_t srcPort;
        fds_uint32_t dstPort;
        std::string session_uuid;
        fds_uint32_t reqCookie;

        explicit RequestHeader(const FDSP_MsgHdrTypePtr &hdr) {
            volId = hdr->glob_volume_id;
            srcIp = hdr->src_ip_lo_addr;
            dstIp = hdr->dst_ip_lo_addr;
            srcPort = hdr->src_port;
            dstPort = hdr->dst_port;
            session_uuid = hdr->session_uuid;
            reqCookie = hdr->req_cookie;
        }
    };

    /*
     * TODO: Make more generic name than catalog request
     */
    class dmCatReq : public FDS_IOType {
      public:
        fds_volid_t  volId;
        std::string blob_name;
        blob_version_t blob_version;
        fds_uint32_t transId;
        fds_uint32_t transOp;
        fds_uint32_t srcIp;
        fds_uint32_t dstIp;
        fds_uint32_t srcPort;
        fds_uint32_t dstPort;
        std::string session_uuid;
        fds_uint32_t reqCookie;
        BlobTxId::const_ptr blobTxId;
        fpi::FDSP_UpdateCatalogTypePtr fdspUpdCatReqPtr;
        boost::shared_ptr<fpi::FDSP_MetaDataList> metadataList;
        std::string session_cache;

        dmCatReq(fds_volid_t  _volId,
                 fds_uint32_t _srcIp,
                 fds_uint32_t _dstIp,
                 fds_uint32_t _srcPort,
                 fds_uint32_t _dstPort,
                 std::string  _session_uuid,
                 fds_uint32_t _reqCookie,
                 fds_io_op_t  _ioType)
                : volId(_volId), srcIp(_srcIp), dstIp(_dstIp),
                  srcPort(_srcPort), dstPort(_dstPort), session_uuid(_session_uuid),
                  reqCookie(_reqCookie), fdspUpdCatReqPtr(NULL) {
            io_type = _ioType;
            io_vol_id = _volId;
            blob_version = blob_version_invalid;
        }

        dmCatReq(fds_volid_t  _volId,
                 const std::string &blobName,
                 fds_uint32_t _srcIp,
                 fds_uint32_t _dstIp,
                 fds_uint32_t _srcPort,
                 fds_uint32_t _dstPort,
                 std::string  _session_uuid,
                 fds_uint32_t _reqCookie,
                 fds_io_op_t  _ioType)
                : volId(_volId), blob_name(blobName), srcIp(_srcIp), dstIp(_dstIp),
                  srcPort(_srcPort), dstPort(_dstPort), session_uuid(_session_uuid),
            reqCookie(_reqCookie), fdspUpdCatReqPtr(NULL) {
            io_type = _ioType;
            io_vol_id = _volId;
            blob_version = blob_version_invalid;
        }

        dmCatReq(const RequestHeader& hdr,
                 fds_io_op_t ioType)
                : volId(hdr.volId), srcIp(hdr.srcIp), dstIp(hdr.dstIp),
                  srcPort(hdr.srcPort), dstPort(hdr.dstPort), session_uuid(hdr.session_uuid),
                  reqCookie(hdr.reqCookie),
                  blob_version(blob_version_invalid) {
            io_type = ioType;
            io_vol_id = hdr.volId;
        }

        dmCatReq(fds_volid_t        _volId,
                 std::string        _blob_name,
                 fds_uint32_t _transId,
                 fds_uint32_t _transOp,
                 fds_uint32_t _srcIp,
                 fds_uint32_t _dstIp,
                 fds_uint32_t _srcPort,
                 fds_uint32_t _dstPort,
                 std::string _session_uuid,
                 fds_uint32_t _reqCookie,
                 fds_io_op_t _ioType,
                 FDS_ProtocolInterface::FDSP_UpdateCatalogTypePtr _updCatReq) {
            volId             = _volId;
            blob_name         = _blob_name;
            blob_version      = blob_version_invalid;
            transId           = _transId;
            transOp           = _transOp;
            srcIp             = _srcIp;
            dstIp             = _dstIp;
            srcPort           = _srcPort;
            dstPort           = _dstPort;
            session_uuid     = _session_uuid;
            reqCookie         = _reqCookie;
            FDS_IOType::io_type = _ioType;
            FDS_IOType::io_vol_id = _volId;
            fdspUpdCatReqPtr = _updCatReq;
            if ((_ioType != FDS_CAT_UPD) && (_ioType != FDS_DM_FWD_CAT_UPD)) {
                fds_verify(_updCatReq == (FDS_ProtocolInterface::FDSP_UpdateCatalogTypePtr)NULL);
            }
        }

        dmCatReq(const fds_volid_t  &_volId,
                 const std::string &_blobName,
                 const blob_version_t &_blob_version,
                 const fds_io_op_t  &_ioType)

        : dmCatReq(_volId,
                 _blobName,
                 0,
                 0,
                 0,
                 0,
                 "",
                 0,
                 _ioType)
        {
            blob_version = _blob_version;
        }

        // TODO(Andrew): Remove this...
        dmCatReq() {
        }

        void fillResponseHeader(fpi::FDSP_MsgHdrTypePtr& msg_hdr) const{
            msg_hdr->src_ip_lo_addr =  dstIp;
            msg_hdr->dst_ip_lo_addr =  srcIp;
            msg_hdr->src_port =  dstPort;
            msg_hdr->dst_port =  srcPort;
            msg_hdr->glob_volume_id =  volId;
            msg_hdr->req_cookie =  reqCookie;
        }

        fds_volid_t getVolId() const {
            return volId;
        }

        virtual ~dmCatReq() {
            fdspUpdCatReqPtr = NULL;
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

        // Why is this not a ostream operator?
        virtual std::string log_string() const {
            std::stringstream ret;
            ret << "dmCatReq for vol " << std::hex << volId
                << std::dec << " io_type " << io_type;
            return ret.str();
        }
    };

    /**
     * Handler that process dmCatReq
     */
    class DmIoReqHandler {
  public:
        virtual Error enqueueMsg(fds_volid_t volId, dmCatReq* ioReq) = 0;
    };

    /**
     * Request to make a snapshot of a volume db
     */
    class DmIoSnapVolCat: public dmCatReq {
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
 * Request to Commit Blob Tx
 */
class DmIoCommitBlobTx: public dmCatReq {
  public:
    typedef std::function<void (const Error &e, DmIoCommitBlobTx *blobTx)> CbType;
  public:
    DmIoCommitBlobTx(const fds_volid_t  &_volId,
                    const std::string &_blobName,
                    const blob_version_t &_blob_version)
            : dmCatReq(_volId, _blobName, _blob_version, FDS_COMMIT_BLOB_TX) {
    }

    virtual std::string log_string() const override {
        std::stringstream ret;
        ret << "DmIoCommitBlobTx vol "
            << std::hex << volId << std::dec;
        return ret.str();
    }

    BlobTxId::const_ptr ioBlobTxDesc;
    /* response callback */
    CbType dmio_commit_blob_tx_resp_cb;
};

/**
 * Request to Abort Blob Tx
 */
class DmIoAbortBlobTx: public dmCatReq {
  public:
    typedef std::function<void (const Error &e, DmIoAbortBlobTx *blobTx)> CbType;
  public:
    DmIoAbortBlobTx(const fds_volid_t  &_volId,
                    const std::string &_blobName,
                    const blob_version_t &_blob_version)
            : dmCatReq(_volId, _blobName, _blob_version, FDS_ABORT_BLOB_TX) {
    }

    virtual std::string log_string() const override {
        std::stringstream ret;
        ret << "DmIoAbortBlobTx vol "
            << std::hex << volId << std::dec;
        return ret.str();
    }

    BlobTxId::const_ptr ioBlobTxDesc;
    /* response callback */
    CbType dmio_abort_blob_tx_resp_cb;
};

/**
 * Request to Start Blob Tx
 */
class DmIoStartBlobTx: public dmCatReq {
  public:
    typedef std::function<void (const Error &e, DmIoStartBlobTx *blobTx)> CbType;
  public:
    DmIoStartBlobTx(const fds_volid_t  &_volId,
                    const std::string &_blobName,
                    const blob_version_t &_blob_version)
            : dmCatReq(_volId, _blobName, _blob_version, FDS_START_BLOB_TX_SVC) {
    }

    virtual std::string log_string() const override {
        std::stringstream ret;
        ret << "DmIoStartBlobTx vol "
            << std::hex << volId << std::dec;
        return ret.str();
    }

    BlobTxId::const_ptr ioBlobTxDesc;
    /* response callback */
    CbType dmio_start_blob_tx_resp_cb;
};
/**
 * Request to query catalog
 */
class DmIoQueryCat: public dmCatReq {
 public:
    typedef std::function<void (const Error &e, DmIoQueryCat *req,
                                BlobNode *bnode)> CbType;
  public:
    DmIoQueryCat(const fds_volid_t  &_volId,
                 const std::string &_blobName,
                 const blob_version_t &_blob_version)
            : dmCatReq(_volId, _blobName, _blob_version, FDS_CAT_QRY_SVC) {
    }

    virtual std::string log_string() const override {
        std::stringstream ret;
        ret << "DmIoQueryCat"
            << std::hex << volId << std::dec;
        return ret.str();
    }

    /* response callback */
    CbType dmio_querycat_resp_cb;
};

/**
 * Request to update catalog
 */
class DmIoUpdateCat: public dmCatReq {
  public:
    typedef std::function<void (const Error &e, DmIoUpdateCat *req)> CbType;
  public:
    DmIoUpdateCat(const fds_volid_t  &_volId,
                  const std::string &_blobName,
                  const blob_version_t &_blob_version)
            : dmCatReq(_volId, _blobName, _blob_version, FDS_CAT_UPD_SVC) {
    }

    virtual std::string log_string() const override {
        std::stringstream ret;
        ret << "DmIoUpdateCat vol "
            << std::hex << volId << std::dec;
        return ret.str();
    }

    FDSP_BlobObjectList obj_list;
    /* response callback */
    CbType dmio_updatecat_resp_cb;
};

/**
 * Request to delete catalog
 */
class DmIoDeleteCat: public dmCatReq {
  public:
    typedef std::function<void (const Error &e, DmIoDeleteCat *req)> CbType;
  public:
    DmIoDeleteCat(const fds_volid_t  &_volId,
                  const std::string &_blobName,
                  const blob_version_t &_blob_version)
            : dmCatReq(_volId, _blobName, _blob_version, FDS_DELETE_BLOB_SVC) {
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

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DMIOREQ_H_
