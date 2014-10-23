/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 * Umbrella classes for the data manager component.
 */

#ifndef SOURCE_DATA_MGR_INCLUDE_DATAMGR_H_
#define SOURCE_DATA_MGR_INCLUDE_DATAMGR_H_

#include <list>
#include <vector>
#include <map>
#include <fds_error.h>
#include <fds_types.h>
#include <fds_volume.h>
#include <dm-platform.h>
#include <fds_timer.h>

/* TODO: avoid cross module include, move API header file to include dir. */
#include <lib/OMgrClient.h>

#include <util/Log.h>
#include <VolumeMeta.h>
#include <concurrency/ThreadPool.h>

#include <include/fds_qos.h>
#include <include/qos_ctrl.h>
#include <unordered_map>
#include <string>
#include <persistent-layer/dm_service.h>

#include <lib/QoSWFQDispatcher.h>
#include <lib/qos_min_prio.h>
#include <NetSession.h>
#include <DmIoReq.h>
#include <dmhandler.h>
#include <CatalogSync.h>
#include <CatalogSyncRecv.h>
#include <dm-tvc/CommitLog.h>

#include <blob/BlobTypes.h>
#include <fdsp/DMSvc.h>
#include <functional>

#include <dm-tvc/TimeVolumeCatalog.h>
#include <StatStreamAggregator.h>

/* if defined, puts complete as soon as they
 * arrive to DM (not for gets right now)
 */
#undef FDS_TEST_DM_NOOP

namespace fds {

struct DataMgr;
class DMSvcHandler;
extern DataMgr *dataMgr;

struct DataMgr : Module, DmIoReqHandler {
    static void InitMsgHdr(const FDSP_MsgHdrTypePtr& msg_hdr);

    class ReqHandler;

    typedef boost::shared_ptr<ReqHandler> ReqHandlerPtr;
    typedef boost::shared_ptr<FDS_ProtocolInterface::FDSP_MetaDataPathRespClient> RespHandlerPrx;
    OMgrClient     *omClient;

    /* Common module provider */
    CommonModuleProviderIf *modProvider_;
    /*
     * TODO: Move to STD shared or unique pointers. That's
     * safer.
     */
    std::unordered_map<fds_uint64_t, VolumeMeta*> vol_meta_map;
    /**
     * Catalog sync manager
     */
    CatalogSyncMgrPtr catSyncMgr;  // sending vol meta
    CatSyncReceiverPtr catSyncRecv;  // receiving vol meta
    void initHandlers();

    /**
     * DmIoReqHandler method implementation
     */
    virtual Error enqueueMsg(fds_volid_t volId, dmCatReq* ioReq) override;
    fds_bool_t amIPrimary(fds_volid_t volUuid);

    inline StatStreamAggregator::ptr statStreamAggregator() {
        return statStreamAggr_;
    }

    inline const std::string & volumeName(fds_volid_t volId) {
        FDSGUARD(*vol_map_mtx);
        return vol_meta_map[volId]->vol_desc->name;
    }

    inline const VolumeDesc * getVolumeDesc(fds_volid_t volId) const {
        FDSGUARD(*vol_map_mtx);
        std::unordered_map<fds_uint64_t, VolumeMeta*>::const_iterator iter =
                vol_meta_map.find(volId);
        return (vol_meta_map.end() != iter && iter->second ?
                iter->second->vol_desc : 0);
    }

    Error process_rm_vol(fds_volid_t vol_uuid, fds_bool_t check_only);

    typedef enum {
      NORMAL_MODE = 0,
      TEST_MODE   = 1,
      MAX
    } dmRunModes;
    dmRunModes    runMode;

    struct Features {
        bool fQosEnabled = true;
        bool fCatSyncEnabled = true;
        bool fTestMode = false;
        bool isTestMode() {
            return fTestMode;
        }
        bool isQosEnabled() {
            return fQosEnabled;
        }
        bool isCatSyncEnabled() {
            return fCatSyncEnabled;
        }
    } feature;

    fds_uint32_t numTestVols;  /* Number of vols to use in test mode */

    /**
     * For timing out request forwarding in DM (to send DMT close ack)
     */
    FdsTimerPtr closedmt_timer;
    FdsTimerTaskPtr closedmt_timer_task;

    /**
     * Time Volume Catalog that provides update and query access
     * to volume catalog
     */
    DmTimeVolCatalog::ptr timeVolCat_;

    /**
     * Aggregator of volume stats streams
     */
    StatStreamAggregator::ptr statStreamAggr_;


    struct dmQosCtrl : FDS_QoSControl {
        DataMgr *parentDm;

        dmQosCtrl(DataMgr *_parent,
                  uint32_t _max_thrds,
                  dispatchAlgoType algo,
                  fds_log *log) :
                FDS_QoSControl(_max_thrds, algo, log, "DM") {
            parentDm = _parent;
            dispatcher = new QoSWFQDispatcher(this, parentDm->scheduleRate,
                    parentDm->qosOutstandingTasks, log);
            // dispatcher = new QoSMinPrioDispatcher(this, log, parentDm->scheduleRate);
        }

        Error processIO(FDS_IOType* _io) {
            Error err(ERR_OK);

            dmCatReq *io = static_cast<dmCatReq*>(_io);
            GLOGDEBUG << "processing : " << io->io_type;

            PerfTracer::tracePointEnd(io->opQoSWaitCtx);
            PerfTracer::tracePointBegin(io->opLatencyCtx);

            switch (io->io_type){
                /* TODO(Rao): Add the new refactored DM messages types here */
                case FDS_START_BLOB_TX:
                    threadPool->schedule(&DataMgr::startBlobTx, dataMgr, io);
                    break;
                case FDS_CAT_UPD_ONCE:
                    threadPool->schedule(&DataMgr::updateCatalogOnce,
                                         dataMgr,
                                         io);
                    break;
                case FDS_COMMIT_BLOB_TX:
                    threadPool->schedule(&DataMgr::commitBlobTx, dataMgr, io);
                    break;
                case FDS_CAT_QRY:
                    threadPool->schedule(&DataMgr::queryCatalogBackendSvc, dataMgr, io);
                    break;
                case FDS_DM_SNAP_VOLCAT:
                case FDS_DM_SNAPDELTA_VOLCAT:
                    threadPool->schedule(&DataMgr::snapVolCat, dataMgr, io);
                    break;
                case FDS_DM_FWD_CAT_UPD:
                    threadPool->schedule(&DataMgr::fwdUpdateCatalog, dataMgr, io);
                    break;
                case FDS_GET_VOLUME_METADATA:
                    threadPool->schedule(&DataMgr::getVolumeMetaData, dataMgr, io);
                    break;
                case FDS_DM_PUSH_META_DONE:
                    threadPool->schedule(&DataMgr::handleDMTClose, dataMgr, io);
                    break;
                case FDS_DM_PURGE_COMMIT_LOG:
                    threadPool->schedule(io->proc, io);
                    break;
                case FDS_DM_META_RECVD:
                    threadPool->schedule(&DataMgr::handleForwardComplete, dataMgr, io);
                    break;
                case FDS_DM_STAT_STREAM:
                    threadPool->schedule(&DataMgr::handleStatStream, dataMgr, io);
                    break;

                /* End of new refactored DM message types */

                case FDS_GET_BLOB_METADATA:
                    threadPool->schedule(&DataMgr::scheduleGetBlobMetaDataSvc, dataMgr, io);
                    break;
                case FDS_SET_BLOB_METADATA:
                    threadPool->schedule(&DataMgr::setBlobMetaDataSvc, dataMgr, io);
                    break;
                case FDS_ABORT_BLOB_TX:
                    threadPool->schedule(&DataMgr::scheduleAbortBlobTxSvc, dataMgr, io);
                    break;

                    // new handlers
                case FDS_DM_SYS_STATS:
                case FDS_DELETE_BLOB:
                case FDS_LIST_BLOB:
                case FDS_CAT_UPD:
                    threadPool->schedule(&dm::Handler::handleQueueItem,
                                         dataMgr->handlers.at(io->io_type), io);
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
            dispatcher->markIODone(const_cast<FDS_IOType *>(&_io));
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

    FDS_VolumeQueue*  sysTaskQueue;

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
     * Used to protect access to vol_meta_map.
     */
    fds_mutex *vol_map_mtx;

    Error getVolObjSize(fds_volid_t volId,
                        fds_uint32_t *maxObjSize);

    Error _add_if_no_vol(const std::string& vol_name,
                         fds_volid_t vol_uuid, VolumeDesc* desc);
    Error _add_vol_locked(const std::string& vol_name,
                          fds_volid_t vol_uuid, VolumeDesc* desc,
                          fds_bool_t vol_will_sync);
    Error _process_add_vol(const std::string& vol_name,
                           fds_volid_t vol_uuid, VolumeDesc* desc,
                           fds_bool_t vol_will_sync);
    Error _process_mod_vol(fds_volid_t vol_uuid,
                           const VolumeDesc& voldesc);

    void initSmMsgHdr(FDSP_MsgHdrTypePtr msgHdr);

    Error expungeObject(fds_volid_t volId, const ObjectID &objId);
    void  expungeObjectCb(QuorumSvcRequest* svcReq,
                         const Error& error,
                         boost::shared_ptr<std::string> payload);
    Error expungeObjectsIfPrimary(fds_volid_t volid,
                                  const std::vector<ObjectID>& oids);

    fds_bool_t volExistsLocked(fds_volid_t vol_uuid) const;

    /**
     * Will move volumes that are in forwarding state to
     * finish forwarding state -- forwarding will actually end when
     * all updates that are currently queued are processed.
     */
    Error notifyDMTClose();
    void finishForwarding(fds_volid_t volid);

    static Error volcat_evt_handler(fds_catalog_action_t,
                                    const fpi::FDSP_PushMetaPtr& push_meta,
                                    const std::string& session_uuid);


    /**
     * A callback from stats collector to sample DM-specific stats
     */
    void sampleDMStats(fds_uint64_t timestamp);

    /**
     * A callback from stats collector with stats for a given volume
     * to add to the aggregator
     */
    void handleLocalStatStream(fds_uint64_t start_timestamp,
                               fds_volid_t volume_id,
                               const std::vector<StatSlot>& slots);

    void setup_metadatapath_server(const std::string &ip);
    void setup_metasync_service();

    explicit DataMgr(CommonModuleProviderIf *modProvider);
    ~DataMgr();
    std::map<fds_io_op_t, dm::Handler*> handlers;
    dmQosCtrl   *qosCtrl;

    fds_uint32_t qosThreadCount = 10;
    fds_uint32_t qosOutstandingTasks = 20;

    // Test related members
    fds_bool_t testUturnAll;
    fds_bool_t testUturnUpdateCat;
    fds_bool_t testUturnStartTx;
    fds_bool_t testUturnSetMeta;

    /* Overrides from Module */
    virtual int  mod_init(SysParams const *const param) override;
    virtual void mod_startup() override;
    virtual void mod_shutdown() override;
    virtual void mod_enable_service() override;

    int run();

    void swapMgrId(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg);
    void setResponseError(fpi::FDSP_MsgHdrTypePtr& msg_hdr, const Error& err);

    std::string getPrefix() const;
    fds_bool_t volExists(fds_volid_t vol_uuid) const;

    inline RespHandlerPrx respHandleCli(const string& session_uuid) {
        return metadatapath_session->getRespClient(session_uuid);
    }

    /* TODO(Rao): Add the new refactored DM messages handlers here */
    void startBlobTx(dmCatReq *io);
    void updateCatalogOnce(dmCatReq *io);
    void commitBlobTx(dmCatReq *io);
    /**
     * Callback from volume catalog when transaction is commited
     * @param[in] version of blob that was committed
     * @param[in] blob_obj_list list of offset to object mapping that
     * was committed or NULL
     * @param[in] meta_list list of metadata k-v pairs that was
     * committed or NULL
     */
    void commitBlobTxCb(const Error &err,
                        blob_version_t blob_version,
                        const BlobObjList::const_ptr& blob_obj_list,
                        const MetaDataList::const_ptr& meta_list,
                        DmIoCommitBlobTx *commitBlobReq);
    void fwdUpdateCatalog(dmCatReq *io);
    /**
     * Callback from volume catalog when forwarded blob update is
     * committed to volume catalog
     */
    void updateFwdBlobCb(const Error &err, DmIoFwdCat *fwdCatReq);
    /* End of new refactored DM message handlers */

    void setBlobMetaDataSvc(void *io);
    void queryCatalogBackendSvc(void * _io);
    void scheduleDeleteCatObjSvc(void * _io);
    void scheduleAbortBlobTxSvc(void * _io);
    void setBlobMetaDataBackend(const dmCatReq *request);
    void getVolumeMetaData(dmCatReq *io);
    void snapVolCat(dmCatReq *io);
    void handleDMTClose(dmCatReq *io);
    void handleForwardComplete(dmCatReq *io);
    void handleStatStream(dmCatReq *io);

    void scheduleGetBlobMetaDataSvc(void *io);
    Error processVolSyncState(fds_volid_t volume_id, fds_bool_t fwd_complete);

    /**
     * Timeout to send DMT close ack if not sent yet and stop forwarding
     * cat updates for volumes that are still in 'finishing forwarding' state
     */
    void finishCloseDMT();

    /**
     * Create snapshot of a specified volume
     */
    Error createSnapshot(const fpi::Snapshot& snapDetails);

    /**
     * Delete snapshot
     */
    Error deleteSnapshot(const fds_uint64_t snapshotId);

    Error deleteVolumeContents(fds_volid_t volId);

    /*
     * Nested class that manages the server interface.
     */
    class ReqHandler : public FDS_ProtocolInterface::FDSP_MetaDataPathReqIf {
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

        /* =========
         * The actual interfaces that get called
         * =========
         */
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
                               FDS_ProtocolInterface::FDSP_GetVolumeBlobListReqTypePtr&
                               blobListReq);
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
    friend class dm::GetBucketHandler;
    friend class dm::DmSysStatsHandler;
    friend class dm::DeleteBlobHandler;
};

class CloseDMTTimerTask : public FdsTimerTask {
  public:
    typedef std::function<void ()> cbType;

    CloseDMTTimerTask(FdsTimer& timer, cbType cb)  //NOLINT
            : FdsTimerTask(timer), timeout_cb(cb) {
    }
    ~CloseDMTTimerTask() {}

    void runTimerTask();

  private:
    cbType timeout_cb;
};

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DATAMGR_H_
