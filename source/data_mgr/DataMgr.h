/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 * Umbrella classes for the data manager component.
 */

#ifndef SOURCE_DATA_MGR_DATAMGR_H_
#define SOURCE_DATA_MGR_DATAMGR_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <stdlib.h>

#include <fdsp/FDSP.h>
#include <Ice/Ice.h>

#include <fds_err.h>
#include <fds_types.h>
#include <fds_volume.h>

/* TODO: avoid cross module include, move API header file to include dir. */
#include <lib/OMgrClient.h>

#include <util/Log.h>
#include <VolumeMeta.h>
#include <concurrency/ThreadPool.h>
#include <concurrency/Mutex.h>

#include <include/fds_qos.h>
#include <include/qos_ctrl.h>
#include <unordered_map>
#include <string>
#include <persistent_layer/dm_service.h>

#include <lib/QoSWFQDispatcher.h>
#include <lib/qos_min_prio.h>

#undef FDS_TEST_DM_NOOP     /* if defined, puts complete as soon as they arrive to DM (not for gets right now) */

namespace fds {

#define DM_TP_THREADS 20
int scheduleUpdateCatalog(void * _io);
int scheduleQueryCatalog(void * _io);

  class DataMgr : virtual public Ice::Application {
public:
  void InitMsgHdr(const FDSP_MsgHdrTypePtr& msg_hdr);
  class dmCatReq : public FDS_IOType {
 public:
    fds_volid_t  volId;
    std::string blob_name;
    fds_uint32_t transId;
    fds_uint32_t transOp;
    long 	 srcIp;
    long 	 dstIp;
    fds_uint32_t srcPort;
    fds_uint32_t dstPort;
    std::string src_node_name;
    fds_uint32_t reqCookie;
    FDS_ProtocolInterface::FDSP_UpdateCatalogTypePtr fdspUpdCatReqPtr;


    dmCatReq(fds_volid_t        _volId,
	     std::string        _blob_name,
	     fds_uint32_t 	_transId,
	     fds_uint32_t 	_transOp,
	     long 	 	_srcIp,
	     long 	 	_dstIp,
	     fds_uint32_t 	_srcPort,
	     fds_uint32_t 	_dstPort,
	     std::string        _src_node_name,
	     fds_uint32_t 	_reqCookie,
	     fds_io_op_t        _ioType,
	     FDS_ProtocolInterface::FDSP_UpdateCatalogTypePtr _updCatReq)
    {
         volId             = _volId;
      	 blob_name         = _blob_name;
         transId           = _transId;
         transOp           = _transOp;
         srcIp 	           = _srcIp;
         dstIp 	           = _dstIp;
         srcPort           = _srcPort;
         dstPort           = _dstPort;
	 src_node_name     = _src_node_name;
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
    };

 private:
    typedef enum {
      NORMAL_MODE = 0,
      TEST_MODE   = 1,
      MAX
    } dmRunModes;
 
    dmRunModes    runMode;
    fds_uint32_t numTestVols;  /* Number of vols to use in test mode */

    class dmQosCtrl : public FDS_QoSControl {
   public:
      DataMgr *parentDm;

      dmQosCtrl(DataMgr *_parent,
                uint32_t _max_thrds,
                dispatchAlgoType algo,
                fds_log *log) :
      FDS_QoSControl(_max_thrds, algo, log, "DM") {
        parentDm = _parent;
	dispatcher = new QoSWFQDispatcher(this, parentDm->scheduleRate, _max_thrds, log);
	//dispatcher = new QoSMinPrioDispatcher(this, log, parentDm->scheduleRate);
      }


      Error processIO(FDS_IOType* _io) {
        Error err(ERR_OK);
        dmCatReq *io = static_cast<dmCatReq*>(_io);

        if (io->io_type == FDS_CAT_UPD) {
          FDS_PLOG(FDS_QoSControl::qos_log) << "Processing  the Catalog update  request";
	  threadPool->schedule(scheduleUpdateCatalog, io);
        } else if (io->io_type == FDS_CAT_QRY) {
          FDS_PLOG(FDS_QoSControl::qos_log) << "Processing  the Catalog Query  request";
	  threadPool->schedule(scheduleQueryCatalog, io);
        } else {
          assert(0);
        }

        return err;
      }

      Error markIODone(const FDS_IOType& _io) {
	Error err(ERR_OK);
	dispatcher->markIODone((FDS_IOType *)&_io);
	return err;
      }

      ~dmQosCtrl() {
      }

    };


    typedef FDS_ProtocolInterface::FDSP_DataPathReqPtr  ReqHandlerPtr;
    typedef FDS_ProtocolInterface::FDSP_DataPathRespPrx RespHandlerPrx;

    /*
     * Ice handlers and comm endpoints.
     */
    ReqHandlerPtr  reqHandleSrv;
    std::unordered_map<std::string, RespHandlerPrx> respHandleCli;
    fds_rwlock respMapMtx;
    OMgrClient     *omClient;

    fds_log *dm_log;
    SysParams *sysParams;
    dmQosCtrl   *qosCtrl;

    /*
     * Cmdline configurables
     */
    fds_uint32_t port_num;      /* Data path port num */
    fds_uint32_t cp_port_num;   /* Control path port num */
    std::string  stor_prefix;   /* String prefix to make file unique */
    fds_uint32_t  scheduleRate;
    fds_bool_t   use_om;        /* Whether to bootstrap from OM */
    std::string  omIpStr;       /* IP addr of the OM used to bootstrap */
    fds_uint32_t omConfigPort;  /* Port of OM used to bootstrap */

    std::string myIp;

    /*
     * Internal threadpool.
     */
    fds_uint32_t num_threads;
    fds_threadpool *_tp;

    /*
     * TODO: Move to STD shared or unique pointers. That's
     * safer.
     */
    std::unordered_map<fds_uint64_t, VolumeMeta*> vol_meta_map;
    /*
     * Used to protect access to vol_meta_map.
     */
    fds_mutex *vol_map_mtx;

    Error _add_if_no_vol(const std::string& vol_name,
                         fds_volid_t vol_uuid,VolumeDesc* desc);
    Error _add_vol_locked(const std::string& vol_name,
                          fds_volid_t vol_uuid,VolumeDesc* desc);
    Error _process_add_vol(const std::string& vol_name,
                           fds_volid_t vol_uuid,VolumeDesc* desc);
    Error _process_rm_vol(fds_volid_t vol_uuid);
    Error _process_mod_vol(fds_volid_t vol_uuid,
			   const VolumeDesc& voldesc);

    Error _process_open(fds_volid_t vol_uuid,
                        std::string blob_name,
                        fds_uint32_t trans_id,
                        const BlobNode*& bnode);
    Error _process_commit(fds_volid_t vol_uuid,
                          std::string blob_name,
                          fds_uint32_t trans_id,
                          const BlobNode*& bnode);
    Error _process_abort();

    Error _process_query(fds_volid_t vol_uuid,
                         std::string blob_name,
                         BlobNode*& bnode);

    fds_bool_t volExistsLocked(fds_volid_t vol_uuid) const;

    static void vol_handler(fds_volid_t vol_uuid,
                            VolumeDesc* desc,
                            fds_vol_notify_t vol_action,
			    FDS_ProtocolInterface::FDSP_ResultType result);

    static void node_handler(fds_int32_t  node_id,
                             fds_uint32_t node_ip,
                             fds_int32_t  node_st,
                             fds_uint32_t node_port,
                             FDS_ProtocolInterface::FDSP_MgrIdType node_type);

 public:
    DataMgr();
    ~DataMgr();


    virtual int run(int argc, char* argv[]);
    void interruptCallback(int arg);
    void swapMgrId(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg);
    fds_log* GetLog();

    void setSysParams(SysParams *params);
    SysParams* getSysParams();

    std::string getPrefix() const;
    fds_bool_t volExists(fds_volid_t vol_uuid) const;
    FDS_ProtocolInterface::FDSP_AnnounceDiskCapabilityPtr dInfo;

    void updateCatalogBackend(dmCatReq  *updCatReq);
    void queryCatalogBackend(dmCatReq  *qryCatReq);

    /* 
     * FDS protocol processing proto types 
     */
	Error updateCatalogInternal(FDSP_UpdateCatalogTypePtr updCatReq, 
                                 fds_volid_t volId,long srcIp,long dstIp,fds_uint32_t srcPort,
				    fds_uint32_t dstPort, std::string src_node_name, fds_uint32_t reqCookie);
	Error queryCatalogInternal(FDSP_QueryCatalogTypePtr qryCatReq, 
                                 fds_volid_t volId,long srcIp,long dstIp,fds_uint32_t srcPort,
				   fds_uint32_t dstPort, std::string src_node_name, fds_uint32_t reqCookie);


    /*
     * Nested class that manages the server interface.
     */
    class ReqHandler : public FDS_ProtocolInterface::FDSP_DataPathReq {
   private:
      Ice::CommunicatorPtr _communicator;

   public:
      explicit ReqHandler(const Ice::CommunicatorPtr& communicator);
      ~ReqHandler();

      void PutObject(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &msg_hdr,
                     const FDS_ProtocolInterface::FDSP_PutObjTypePtr &put_obj,
                     const Ice::Current&);

      void GetObject(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &msg_hdr,
                     const FDS_ProtocolInterface::FDSP_GetObjTypePtr& get_obj,
                     const Ice::Current&);

      void DeleteObject(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &msg_hdr,
                     const FDS_ProtocolInterface::FDSP_DeleteObjTypePtr& get_obj,
                     const Ice::Current&);

      void UpdateCatalogObject(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr
                               &msg_hdr,
                               const FDS_ProtocolInterface::
                               FDSP_UpdateCatalogTypePtr& update_catalog,
                               const Ice::Current&);

      void QueryCatalogObject(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr
                              &msg_hdr,
                              const FDS_ProtocolInterface::
                              FDSP_QueryCatalogTypePtr& query_catalog,
                              const Ice::Current&);

      void DeleteCatalogObject(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr
                              &msg_hdr,
                              const FDS_ProtocolInterface::
                              FDSP_DeleteCatalogTypePtr& query_catalog,
                              const Ice::Current&);

      void OffsetWriteObject(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr&
                             msg_hdr,
                             const FDS_ProtocolInterface::
                             FDSP_OffsetWriteObjTypePtr& offset_write_obj,
                             const Ice::Current&);

      void RedirReadObject(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr
                           &msg_hdr,
                           const FDS_ProtocolInterface::
                           FDSP_RedirReadObjTypePtr& redir_read_obj,
                           const Ice::Current&);

      void AssociateRespCallback(const Ice::Identity& ident,
				 const std::string& src_node_name,
                                 const Ice::Current& current);

      void GetVolumeBlobList(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fds_msg, 
			     const FDS_ProtocolInterface::FDSP_GetVolumeBlobListReqTypePtr& blob_list_req,
			     const Ice::Current& current) {

      }
    };

    /*
     * Nested class that manages the client interface.
     */
    class RespHandler : public FDS_ProtocolInterface::FDSP_DataPathResp {
   public:
      RespHandler();
      ~RespHandler();

      void PutObjectResp(const FDS_ProtocolInterface::
                         FDSP_MsgHdrTypePtr &msg_hdr,
                         const FDS_ProtocolInterface::
                         FDSP_PutObjTypePtr &put_obj,
                         const Ice::Current&);

      void GetObjectResp(const FDS_ProtocolInterface::
                         FDSP_MsgHdrTypePtr &msg_hdr,
                         const FDS_ProtocolInterface::
                         FDSP_GetObjTypePtr& get_obj,
                         const Ice::Current&);
      void UpdateCatalogObjectResp(const FDS_ProtocolInterface::
                                   FDSP_MsgHdrTypePtr &msg_hdr,
                                   const FDS_ProtocolInterface::
                                   FDSP_UpdateCatalogTypePtr& update_catalog,
                                   const Ice::Current&);
      void QueryCatalogObjectResp(const FDS_ProtocolInterface::
                                  FDSP_MsgHdrTypePtr &msg_hdr,
                                  const FDS_ProtocolInterface::
                                  FDSP_QueryCatalogTypePtr& query_catalog,
                                  const Ice::Current&);
      void DeleteCatalogObjectResp(const FDS_ProtocolInterface::
                                  FDSP_MsgHdrTypePtr &msg_hdr,
                                  const FDS_ProtocolInterface::
                                  FDSP_DeleteCatalogTypePtr& query_catalog,
                                  const Ice::Current&);
      void OffsetWriteObjectResp(const FDS_ProtocolInterface::
                                 FDSP_MsgHdrTypePtr& msg_hdr,
                                 const FDS_ProtocolInterface::
                                 FDSP_OffsetWriteObjTypePtr& offset_write_obj,
                                 const Ice::Current&);
      void RedirReadObjectResp(const FDS_ProtocolInterface::
                               FDSP_MsgHdrTypePtr &msg_hdr,
                               const FDS_ProtocolInterface::
                               FDSP_RedirReadObjTypePtr& redir_read_obj,
                               const Ice::Current&);
      void GetVolumeBlobListResp(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fds_msg, 
				 const FDS_ProtocolInterface::FDSP_GetVolumeBlobListRespTypePtr& blob_list_rsp, 
				 const Ice::Current &){
    }
    };
  };

}  // namespace fds

#endif  // SOURCE_DATA_MGR_DATAMGR_H_
