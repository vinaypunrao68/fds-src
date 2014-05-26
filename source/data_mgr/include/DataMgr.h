/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 * Umbrella classes for the data manager component.
 */

#ifndef SOURCE_DATA_MGR_DATAMGR_H_
#define SOURCE_DATA_MGR_DATAMGR_H_

#include <fds_error.h>
#include <fds_types.h>
#include <fds_volume.h>
#include <dm-platform.h>

/* TODO: avoid cross module include, move API header file to include dir. */
#include <lib/OMgrClient.h>

#include <util/Log.h>
#include <VolumeMeta.h>
#include <concurrency/ThreadPool.h>

#include <include/fds_qos.h>
#include <include/qos_ctrl.h>
#include <unordered_map>
#include <string>
#include <persistent_layer/dm_service.h>

#include <lib/QoSWFQDispatcher.h>
#include <lib/qos_min_prio.h>
#include <NetSession.h>
#include <DmIoReq.h>
#include <CatalogSync.h>

#include <CommitLog.h>

#include <blob/BlobTypes.h>
#include <fdsp/DMSvc.h>
#include <functional>

#undef FDS_TEST_DM_NOOP     /* if defined, puts complete as soon as they arrive to DM (not for gets right now) */

namespace fds {

#define DM_TP_THREADS 20
int scheduleUpdateCatalog(void * _io);
int scheduleQueryCatalog(void * _io);
int scheduleStatBlob(void * _io);
int scheduleStartBlobTx(void * _io);
int scheduleDeleteCatObj(void * _io);
int scheduleBlobList(void * _io);
int scheduleGetBlobMetaData(void* io);
int scheduleSetBlobMetaData(void* io);
int scheduleGetVolumeMetaData(void* io);
int scheduleSnapVolCat(void* _io);

class DataMgr;
class DMSvcHandler;
extern DataMgr *dataMgr;

class DataMgr : public PlatformProcess, public DmIoReqHandler {
public:
    static void InitMsgHdr(const FDSP_MsgHdrTypePtr& msg_hdr);

    class ReqHandler;
    typedef boost::shared_ptr<ReqHandler> ReqHandlerPtr;
    typedef boost::shared_ptr<FDS_ProtocolInterface::FDSP_MetaDataPathRespClient> RespHandlerPrx; //NOLINT
    OMgrClient     *omClient;
    /*
     * TODO: Move to STD shared or unique pointers. That's
     * safer.
     */
    std::unordered_map<fds_uint64_t, VolumeMeta*> vol_meta_map;
    /**
     * Catalog sync manager
     */
    CatalogSyncMgrPtr catSyncMgr;

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
        GLOGDEBUG << "processing : " << io->io_type;
	switch (io->io_type){
            case FDS_CAT_UPD:
                threadPool->schedule(scheduleUpdateCatalog, io);
                break;
            case FDS_CAT_QRY:
                threadPool->schedule(scheduleQueryCatalog, io);
                break;
            case FDS_CAT_QRY2:
                threadPool->schedule(&DataMgr::queryCatalogBackend2, dataMgr, io);
                break;
            case FDS_STAT_BLOB:
                threadPool->schedule(scheduleStatBlob, io);
                break;
            case FDS_START_BLOB_TX:
                threadPool->schedule(scheduleStartBlobTx, io);
                break;
            case FDS_DELETE_BLOB:
                threadPool->schedule(scheduleDeleteCatObj, io);
                break;
            case FDS_LIST_BLOB:
                threadPool->schedule(scheduleBlobList, io);
                break;
            case FDS_GET_BLOB_METADATA:
                threadPool->schedule(scheduleGetBlobMetaData, io);
                break;
            case FDS_SET_BLOB_METADATA:
                threadPool->schedule(scheduleSetBlobMetaData, io);
                break;
            case FDS_DM_SNAP_VOLCAT:
                threadPool->schedule(scheduleSnapVolCat, io);
                break;
             case FDS_GET_VOLUME_METADATA:
                threadPool->schedule(scheduleGetVolumeMetaData, io);
                break;
            default:
                FDS_PLOG(FDS_QoSControl::qos_log) << "Unknown IO Type received";
                assert(0);
                break;
	}

        return err;
      }

      Error markIODone(const FDS_IOType& _io) {
	Error err(ERR_OK);
	dispatcher->markIODone((FDS_IOType *)&_io);
	return err;
      }

      virtual ~dmQosCtrl() {
      }

    };

    /*
     * RPC handlers and comm endpoints.
     */
    ReqHandlerPtr  metadatapath_handler;
    boost::shared_ptr<netSessionTbl> nstable;
    netMetaDataPathServerSession *metadatapath_session;
    // std::unordered_map<std::string, RespHandlerPrx> respHandleCli;
    
    fds_rwlock respMapMtx;

    dmQosCtrl   *qosCtrl;


    /*
     * Cmdline configurables
     */
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
     * Used to protect access to vol_meta_map.
     */
    fds_mutex *vol_map_mtx;

    /**
     * Manages pending 2-phase commit updates
     */
    DmCommitLog::ptr commitLog;

    /**
     * Giant, slow, big hammer lock.
     * TODO(Andrew): Remove this! It's needed to provide
     * serialization of update catalog requests to the same
     * blob. There's a much better solution than this that
     * needs to be added. It's just more code...
     */
    fds_mutex *big_fat_lock;

    Error getVolObjSize(fds_volid_t volId,
                        fds_uint32_t *maxObjSize);

    Error _add_if_no_vol(const std::string& vol_name,
                         fds_volid_t vol_uuid,VolumeDesc* desc);
    Error _add_vol_locked(const std::string& vol_name,
                          fds_volid_t vol_uuid, VolumeDesc* desc,
                          fds_bool_t crt_catalogs);
    Error _process_add_vol(const std::string& vol_name,
                           fds_volid_t vol_uuid,VolumeDesc* desc,
                           fds_bool_t crt_catalogs);
    Error _process_rm_vol(fds_volid_t vol_uuid, fds_bool_t check_only);
    Error _process_mod_vol(fds_volid_t vol_uuid,
			   const VolumeDesc& voldesc);

    Error _process_open(fds_volid_t vol_uuid,
                        std::string blob_name,
                        fds_uint32_t trans_id,
                        const BlobNode* bnode);
    Error _process_commit(fds_volid_t vol_uuid,
                          std::string blob_name,
                          fds_uint32_t trans_id,
                          const BlobNode* bnode);
    Error _process_abort();

    Error _process_query(fds_volid_t vol_uuid,
                         std::string blob_name,
                         BlobNode*& bnode);
    Error _process_delete(fds_volid_t vol_uuid,
                         std::string blob_name);
    fds_bool_t _process_isEmpty(fds_volid_t volId);
    Error _process_list(fds_volid_t volId,
                        std::list<BlobNode>& bNodeList);

    void initSmMsgHdr(FDSP_MsgHdrTypePtr msgHdr);

    fds_bool_t amIPrimary(fds_volid_t volUuid);
    Error applyBlobUpdate(fds_volid_t volUuid,
                          const BlobObjectList &offsetList,
                          BlobNode *bnode);
    Error expungeBlob(const BlobNode *bnode);
    Error expungeObject(fds_volid_t volId, const ObjectID &objId);

    fds_bool_t volExistsLocked(fds_volid_t vol_uuid) const;

    /**
     * DmIoReqHandler method implementation
     */
    virtual Error enqueueMsg(fds_volid_t volId, dmCatReq* ioReq);

    static Error vol_handler(fds_volid_t vol_uuid,
                             VolumeDesc* desc,
                             fds_vol_notify_t vol_action,
                             fpi::FDSP_NotifyVolFlag vol_flag,
                             fpi::FDSP_ResultType result);

    static void node_handler(fds_int32_t  node_id,
                             fds_uint32_t node_ip,
                             fds_int32_t  node_st,
                             fds_uint32_t node_port,
                             FDS_ProtocolInterface::FDSP_MgrIdType node_type);

    static Error volcat_evt_handler(fds_catalog_action_t,
                                    const fpi::FDSP_PushMetaPtr& push_meta,
                                    const std::string& session_uuid);

  protected:
    void setup_metadatapath_server(const std::string &ip);
    void setup_metasync_service();

  public:
    DataMgr(int argc, char *argv[], Platform *platform, Module **mod_vec);
    ~DataMgr();

  /* From FdsProcess */
    virtual void proc_pre_startup() override;
    virtual int  run() override;
    virtual void interrupt_cb(int signum) override;

    void swapMgrId(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg);
    void setResponseError(fpi::FDSP_MsgHdrTypePtr& msg_hdr, const Error& err);

    std::string getPrefix() const;
    fds_bool_t volExists(fds_volid_t vol_uuid) const;

    inline RespHandlerPrx respHandleCli(const string& session_uuid) {
        return metadatapath_session->getRespClient(session_uuid);
    }

    void updateCatalogBackend(dmCatReq  *updCatReq);
    Error updateCatalogProcess(const dmCatReq  *updCatReq, BlobNode **bnode);
    void queryCatalogBackend(dmCatReq  *qryCatReq);
    void queryCatalogBackend2(void * _io);
    Error queryCatalogProcess(const dmCatReq  *qryCatReq, BlobNode **bnode);
    void deleteCatObjBackend(dmCatReq  *delCatReq);
    Error deleteBlobProcess(const dmCatReq  *delCatReq, BlobNode **bnode);
    void blobListBackend(dmCatReq *listBlobReq);
    void statBlobBackend(const dmCatReq *statBlobReq);
    void startBlobTxBackend(const dmCatReq *startBlobTxReq);
    void getBlobMetaDataBackend(const dmCatReq *request);
    void setBlobMetaDataBackend(const dmCatReq *request);
    void getVolumeMetaDataBackend(const dmCatReq *request);
    void snapVolCat(DmIoSnapVolCat* snapReq);
    /* 
     * FDS protocol processing proto types 
     */
    Error updateCatalogInternal(FDSP_UpdateCatalogTypePtr updCatReq, 
                                fds_volid_t volId,long srcIp,long dstIp,fds_uint32_t srcPort,
                                fds_uint32_t dstPort, std::string session_uuid, fds_uint32_t reqCookie);
    Error queryCatalogInternal(FDSP_QueryCatalogTypePtr qryCatReq, 
                               fds_volid_t volId,long srcIp,long dstIp,fds_uint32_t srcPort,
                               fds_uint32_t dstPort, std::string session_uuid, fds_uint32_t reqCookie);
    Error deleteCatObjInternal(FDSP_DeleteCatalogTypePtr delCatReq, 
                               fds_volid_t volId,long srcIp,long dstIp,fds_uint32_t srcPort,
                               fds_uint32_t dstPort, std::string session_uuid, fds_uint32_t reqCookie);
    Error blobListInternal(const FDSP_GetVolumeBlobListReqTypePtr& blob_list_req,
                           fds_volid_t volId,long srcIp,long dstIp,fds_uint32_t srcPort,
                           fds_uint32_t dstPort, std::string session_uuid, fds_uint32_t reqCookie);
    Error statBlobInternal(const std::string volumeName, const std::string &blobName,
                           fds_volid_t volId, long srcIp, long dstIp, fds_uint32_t srcPort,
                           fds_uint32_t dstPort, std::string session_uuid, fds_uint32_t reqCookie);
    void startBlobTxInternal(const std::string volumeName, const std::string &blobName,
                             BlobTxId::const_ptr blobTxId,
                             fds_volid_t volId, long srcIp, long dstIp, fds_uint32_t srcPort,
                             fds_uint32_t dstPort, std::string session_uuid, fds_uint32_t reqCookie);
    void commitBlobTxInternal(BlobTxId::const_ptr blobTxId,
                             fds_volid_t volId, long srcIp, long dstIp, fds_uint32_t srcPort,
                             fds_uint32_t dstPort, std::string session_uuid, fds_uint32_t reqCookie);
    void abortBlobTxInternal(BlobTxId::const_ptr blobTxId,
                             fds_volid_t volId, long srcIp, long dstIp, fds_uint32_t srcPort,
                             fds_uint32_t dstPort, std::string session_uuid, fds_uint32_t reqCookie);

    /*
     * Nested class that manages the server interface.
     */
    class ReqHandler : public FDS_ProtocolInterface::FDSP_MetaDataPathReqIf {
   private:

   public:
      explicit ReqHandler();
      ~ReqHandler();

      void StartBlobTx(const FDSP_MsgHdrType& msg_hdr,
                       const std::string &volumeName,
                       const std::string &blobName,
                       const TxDescriptor &txDesc) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
      }

      void UpdateCatalogObject(const FDSP_MsgHdrType& fdsp_msg, 
			       const FDSP_UpdateCatalogType& cat_obj_req) {
	// Don't do anything here. This stub is just to keep cpp compiler happy
      }

      void QueryCatalogObject(const FDSP_MsgHdrType& fdsp_msg, 
			      const FDSP_QueryCatalogType& cat_obj_req) {
	// Don't do anything here. This stub is just to keep cpp compiler happy
      }

      void DeleteCatalogObject(const FDSP_MsgHdrType& fdsp_msg, 
			       const FDSP_DeleteCatalogType& cat_obj_req) {
	// Don't do anything here. This stub is just to keep cpp compiler happy
      }

      void GetVolumeBlobList(const FDSP_MsgHdrType& fds_msg, 
			     const FDSP_GetVolumeBlobListReqType& blob_list_req) {
	// Don't do anything here. This stub is just to keep cpp compiler happy
      }

      void StatBlob(const FDSP_MsgHdrType& msg_hdr,
                    const std::string &volumeName,
                    const std::string &blobName) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
      }

      void SetBlobMetaData(const FDSP_MsgHdrType& header, 
                           const std::string& volumeName, 
                           const std::string& blobName, const 
                           FDSP_MetaDataList& metaDataList) {
      }
      
      void GetBlobMetaData(const FDSP_MsgHdrType& header, 
                           const std::string& volumeName, 
                           const std::string& blobName) {
      }

      void GetVolumeMetaData(const FDSP_MsgHdrType& header, 
                             const std::string& volumeName) {
      }

      // ================================================================================================
      // The actual stuff
      // ================================================================================================

      void StartBlobTx(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                       boost::shared_ptr<std::string> &volumeName,
                       boost::shared_ptr<std::string> &blobName,
                       FDS_ProtocolInterface::TxDescriptorPtr& txDesc);

      void UpdateCatalogObject(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr
                               &msg_hdr,
                               FDS_ProtocolInterface::
                               FDSP_UpdateCatalogTypePtr& update_catalog);


      void QueryCatalogObject(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr
                              &msg_hdr,
                              FDS_ProtocolInterface::
                              FDSP_QueryCatalogTypePtr& query_catalog);


      void DeleteCatalogObject(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr
			       &msg_hdr,
			       FDS_ProtocolInterface::
			       FDSP_DeleteCatalogTypePtr& query_catalog);

      void GetVolumeBlobList(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr, 
			     FDS_ProtocolInterface::FDSP_GetVolumeBlobListReqTypePtr& blobListReq);

      void StatBlob(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                    boost::shared_ptr<std::string> &volumeName,
                    boost::shared_ptr<std::string> &blobName);

      void SetBlobMetaData(boost::shared_ptr<FDSP_MsgHdrType>& header, 
                           boost::shared_ptr<std::string>& volumeName, 
                           boost::shared_ptr<std::string>& blobName, 
                           boost::shared_ptr<FDSP_MetaDataList>& metaDataList);

      void GetBlobMetaData(boost::shared_ptr<FDSP_MsgHdrType>& header,
                           boost::shared_ptr<std::string>& volumeName,
                           boost::shared_ptr<std::string>& blobName);

      void GetVolumeMetaData(boost::shared_ptr<FDSP_MsgHdrType>& header,
                             boost::shared_ptr<std::string>& volumeName);
    };

    friend class DMSvcHandler;
};

}  // namespace fds

#endif  // SOURCE_DATA_MGR_DATAMGR_H_
