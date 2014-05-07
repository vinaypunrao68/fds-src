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

#include <include/fds_qos.h>
#include <include/qos_ctrl.h>
#include <unordered_map>
#include <string>


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

        fds_volid_t getVolId() const {
            return volId;
        }

        ~dmCatReq() {
            fdspUpdCatReqPtr = NULL;
        }

        void setBlobVersion(blob_version_t version) {
            blob_version = version;          
        }
    };

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DMIOREQ_H_
