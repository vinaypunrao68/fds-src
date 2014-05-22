/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 * Classes for DM requests that go through QoS queues
 */

#ifndef SOURCE_DATA_MGR_INCLUDE_DMIOREQ_H_
#define SOURCE_DATA_MGR_INCLUDE_DMIOREQ_H_

#include <fds_error.h>
#include <fds_types.h>
#include <fds_volume.h>
#include <fdsp/FDSP_types.h>

namespace fds {

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
        long srcIp;
        long dstIp;
        fds_uint32_t srcPort;
        fds_uint32_t dstPort;
        std::string session_uuid;
        fds_uint32_t reqCookie;
        FDS_ProtocolInterface::FDSP_UpdateCatalogTypePtr fdspUpdCatReqPtr;

  dmCatReq(fds_volid_t _volId,
           long  _srcIp,
           long  _dstIp,
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

        dmCatReq(fds_volid_t        _volId,
                 std::string        _blob_name,
                 fds_uint32_t 	_transId,
                 fds_uint32_t 	_transOp,
                 long 	 	_srcIp,
                 long 	 	_dstIp,
                 fds_uint32_t 	_srcPort,
                 fds_uint32_t 	_dstPort,
                 std::string        _session_uuid,
                 fds_uint32_t 	_reqCookie,
                 fds_io_op_t        _ioType,
                 FDS_ProtocolInterface::FDSP_UpdateCatalogTypePtr _updCatReq) {
            volId             = _volId;
            blob_name         = _blob_name;
            blob_version      = blob_version_invalid;
            transId           = _transId;
            transOp           = _transOp;
            srcIp 	           = _srcIp;
            dstIp 	           = _dstIp;
            srcPort           = _srcPort;
            dstPort           = _dstPort;
            session_uuid     = _session_uuid;
            reqCookie         = _reqCookie;
            FDS_IOType::io_type = _ioType;
            FDS_IOType::io_vol_id = _volId;
            fdspUpdCatReqPtr = _updCatReq;
            if (_ioType !=   FDS_CAT_UPD) {
                fds_verify(_updCatReq == (FDS_ProtocolInterface::FDSP_UpdateCatalogTypePtr)NULL);
            }
        }

        dmCatReq() {
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

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DMIOREQ_H_
