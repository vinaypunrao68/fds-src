/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_STORMGR_H_
#define SOURCE_STOR_MGR_INCLUDE_STORMGR_H_

#include <atomic>
#include <string>
#include <queue>
#include <unordered_map>
#include <utility>

#include "fdsp/FDSP_types.h"
#include "fds_types.h"
#include "ObjectId.h"
#include "util/Log.h"
#include "StorMgrVolumes.h"
#include "SmObjDb.h"
#include "persistent-layer/dm_io.h"
#include "fds_migration.h"
#include "TransJournal.h"
#include "hash/md5.h"

#include "fds_qos.h"
#include "qos_ctrl.h"
#include "fds_assert.h"
#include "fds_config.hpp"
#include "util/timeutils.h"
#include "lib/StatsCollector.h"

/*
 * TODO: Move this header out of lib/
 * to include/ since it's linked by many.
 */
#include "lib/qos_htb.h"
#include "lib/qos_min_prio.h"
#include "lib/QoSWFQDispatcher.h"

/* TODO: avoid include across module, put API header file to include dir */
#include "lib/OMgrClient.h"
#include "concurrency/Mutex.h"

#include "fds_module.h"
#include "platform/platform-lib.h"

// #include "NetSession.h"
#include "kvstore/tokenstatedb.h"
#include "fdsp/SMSvc.h"

#include <SmDiskTypes.h>
#include <object-store/ObjectStore.h>


#define FDS_STOR_MGR_LISTEN_PORT FDS_CLUSTER_TCP_PORT_SM
#define FDS_STOR_MGR_DGRAM_PORT FDS_CLUSTER_UDP_PORT_SM
#define FDS_MAX_WAITING_CONNS  10

using namespace FDS_ProtocolInterface;  // NOLINT
namespace FDS_ProtocolInterface {
    class FDSP_DataPathReqProcessor;
    class FDSP_DataPathReqIf;
    class FDSP_DataPathRespClient;
}
typedef netServerSessionEx<FDSP_DataPathReqProcessor,
                FDSP_DataPathReqIf,
                FDSP_DataPathRespClient> netDataPathServerSession;

namespace fds {

extern ObjectStorMgr *objStorMgr;

using DPReqClientPtr = boost::shared_ptr<FDSP_DataPathReqClient>;
using DPRespClientPtr = boost::shared_ptr<FDSP_DataPathRespClient>;

/*
 * Forward declarations
 */
class ObjectStorMgrI;

/**
 * @brief Storage manager counters
 */
class SMCounters : public FdsCounters {
    public:
     SMCounters(const std::string &id, FdsCountersMgr *mgr)
         : FdsCounters(id, mgr),
         put_reqs("put_reqs", this),
         get_reqs("get_reqs", this),
         del_reqs("del_reqs", this),
         puts_latency("puts_latency", this),
         put_tok_objs("put_tok_objs", this),
         get_tok_objs("get_tok_objs", this),
         resolve_mrgd_cnt("resolve_mrgd_cnt", this),
         resolve_used_sync_cnt("resolve_used_sync_cnt", this),
         proxy_gets("proxy_gets", this) {
         }
     /* Exposed for counters */
     SMCounters() {}

     NumericCounter put_reqs;
     NumericCounter get_reqs;
     NumericCounter del_reqs;
     LatencyCounter puts_latency;
     NumericCounter put_tok_objs;
     NumericCounter get_tok_objs;
     /* During resolve number of merges that took place */
     NumericCounter resolve_mrgd_cnt;
     /* During resolve # of times we replaced existing entry with sync entry */
     NumericCounter resolve_used_sync_cnt;
     NumericCounter proxy_gets;
};


class ObjectStorMgr : public Module, public SmIoReqHandler {
    public:
     /*
      * OM/boostrap related members
      */
     OMgrClient         *omClient;

    protected:
     typedef enum {
         NORMAL_MODE = 0,
         TEST_MODE   = 1,
         MAX
     } SmRunModes;

     CommonModuleProviderIf *modProvider_;
     /*
      * glocal dedupe  stats  counter 
      */ 

     std::atomic<fds_uint64_t> dedupeByteCnt;

     /*
      * Local storage members
      */
     TransJournal<ObjectID, ObjectIdJrnlEntry> *omJrnl;
     fds_mutex *objStorMutex;

     /// Manager of persistent object storage
     ObjectStore::unique_ptr objectStore;

     /*
      * FDSP RPC members
      * The map is used for sending back the response to the
      * appropriate SH/DM
      */
     boost::shared_ptr<netSessionTbl> nst_;
     boost::shared_ptr<FDSP_DataPathReqIf> datapath_handler_;
     netDataPathServerSession *datapath_session_;

     /** Cluster communication manager */
     ClusterCommMgrPtr clust_comm_mgr_;

     /** Migrations related */
     FdsMigrationSvcPtr migrationSvc_;

     /** Token state db */
     kvstore::TokenStateDBPtr tokenStateDb_;

     /** Counters */
     std::unique_ptr<SMCounters> counters_;

     /* For caching dlt close response information */
     std::pair<std::string, FDSP_DltCloseTypePtr> cached_dlt_close_;

     /* To indicate whether tokens were migrated or not for the dlt. Based on this
      * flag we simulate sync/io close notification to OM.  We shouldn't need this
      * flag once we start doing per token copy/syncs
      */
     bool tok_migrated_for_dlt_;

     /** Helper for accessing datapth response client */
     DPRespClientPtr fdspDataPathClient(const std::string& session_uuid);

     /*
      * Service UUID to Session UUID mapping stuff. Used to support
      * proxying where SM doesn't know the session UUID because it
      * was proxyed from another node. We can use service UUID instead.
      */
     typedef std::string SessionUuid;
     typedef std::unordered_map<NodeUuid, SessionUuid, UuidHash> SvcToSessMap;
     /** Maps service UUIDs to established session id */
     SvcToSessMap svcSessMap;
     /** Protects the service to session map */
     fds_rwlock svcSessLock;
     /** Stores mapping from service uuid to session uuid */
     void addSvcMap(const NodeUuid    &svcUuid,
                    const SessionUuid &sessUuid);
     SessionUuid getSvcSess(const NodeUuid &svcUuid);

     NodeAgentDpClientPtr getProxyClient(ObjectID& oid, const FDSP_MsgHdrTypePtr& msg);

     /*
      * TODO: this one should be the singleton by itself.  Need to make it
      * a stand-alone module like resource manager for volume.
      * Volume specific members
      */
     StorMgrVolumeTable *volTbl;

     /*
      * Qos related members and classes
      */
     fds_uint32_t totalRate;
     fds_uint32_t qosThrds;
     fds_uint32_t qosOutNum;

     // Temporary to execute different stubs in processIO
     fds_bool_t execNewStubs;

     class SmQosCtrl : public FDS_QoSControl {
        private:
         ObjectStorMgr *parentSm;

        public:
         SmQosCtrl(ObjectStorMgr *_parent,
                   uint32_t _max_thrds,
                   dispatchAlgoType algo,
                   fds_log *log) :
             FDS_QoSControl(_max_thrds, algo, log, "SM") {
                 parentSm = _parent;
                 LOGNOTIFY << "Qos totalRate " << parentSm->totalRate
                     << ", num outstanding io " << parentSm->qosOutNum;

                 // dispatcher = new QoSMinPrioDispatcher(this, log, 3000);
                 dispatcher = new QoSWFQDispatcher(this,
                                                   parentSm->totalRate,
                                                   parentSm->qosOutNum,
                                                   log);
                 // dispatcher = new QoSHTBDispatcher(this, log, 150);
             }
         virtual ~SmQosCtrl() {
         }

         Error processIO(FDS_IOType* _io);

         Error markIODone(FDS_IOType& _io) {
             Error err(ERR_OK);
             dispatcher->markIODone(&_io);
             return err;
         }

         Error markIODone(FDS_IOType &_io,
                          diskio::DataTier  tier,
                          fds_bool_t iam_primary = false) {
             Error err(ERR_OK);
             dispatcher->markIODone(&_io);
             if (iam_primary &&
                 ((_io.io_type == FDS_SM_PUT_OBJECT) ||
                  (_io.io_type == FDS_SM_GET_OBJECT))) {
                 if (tier == diskio::diskTier) {
                     StatsCollector::singleton()->recordEvent(_io.io_vol_id,
                                                              _io.io_done_ts,
                                                              STAT_SM_OP_HDD,
                                                              _io.io_total_time);
                 } else if (tier == diskio::flashTier) {
                     StatsCollector::singleton()->recordEvent(_io.io_vol_id,
                                                              _io.io_done_ts,
                                                              STAT_SM_OP_SSD,
                                                              _io.io_total_time);
                 }
             }
             return err;
         }
     };


     SmVolQueue *sysTaskQueue;

     /*
      * Flash write-back members.
      * TODO: These should probably be in the persistent layer
      * but is easier here for now since needs the index.
      */
     typedef boost::lockfree::queue<ObjectID*> ObjQueue;  /* Dirty list type */
     static void writeBackFunc(ObjectStorMgr *parent);    /* Function for write-back */
     std::atomic_bool  shuttingDown;      /* SM shut down flag for write-back thread */
     fds_uint32_t      maxDirtyObjs;      /* Max dirty list size */
     ObjQueue         *dirtyFlashObjs;    /* Flash's dirty list */

     /*
      * Local perf stat collection
      */
     enum perfMigOp {
         flashToDisk,
         diskToFlash,
         invalidMig
     };

     SysParams *sysParams;

     /*
      * Private request processing members.
      */
     Error enqTransactionIo(FDSP_MsgHdrTypePtr msgHdr,
                            const ObjectID& obj_id,
                            SmIoReq *ioReq, TransJournalId &trans_id);
     void create_transaction_cb(FDSP_MsgHdrTypePtr msgHdr,
                                SmIoReq *ioReq, TransJournalId trans_id);
     Error writeObjectMetaData(const OpCtx &opCtx,
                               const ObjectID &objId,
                               fds_uint32_t  obj_size,
                               obj_phy_loc_t *obj_phy_lo,
                               fds_bool_t    relocate_flag,
                               diskio::DataTier      from_tier,
                               meta_vol_io_t  *vio);

     Error writeObject(const OpCtx &opCtx,
                       const ObjectID   &objId,
                       const ObjectBuf  &objCompData,
                       fds_volid_t       volId,
                       diskio::DataTier &tier);
     Error writeObjectToTier(const OpCtx &opCtx,
                             const ObjectID  &objId,
                             const ObjectBuf &objData,
                             fds_volid_t       volId,
                             diskio::DataTier tier);
     Error writeObjectDataToTier(const ObjectID  &objId,
                                 const ObjectBuf &objData,
                                 diskio::DataTier tier,
                                 obj_phy_loc_t& phys_loc);

     inline fds_uint32_t getSysTaskIopsMin() {
         return totalRate/10;  // 10% of total rate
     }

     inline fds_uint32_t getSysTaskIopsMax() {
         return totalRate/5;  // 20% of total rate
     }

     inline fds_uint32_t getSysTaskPri() {
         return FdsSysTaskPri;
     }


    protected:
     void setup_datapath_server(const std::string &ip);
     void setup_migration_svc(const std::string &obj_dir);

  public:
    SmQosCtrl  *qosCtrl;
    explicit ObjectStorMgr(CommonModuleProviderIf *modProvider);
     /* This constructor is exposed for mock testing */
     ObjectStorMgr()
             : Module("sm"), totalRate(20000) {
         smObjDb = nullptr;
         qosCtrl = nullptr;
         dirtyFlashObjs = nullptr;
         tierEngine = nullptr;
         volTbl = nullptr;
         objStorMutex = nullptr;
         omJrnl = nullptr;
     }
     /* this is for standalone testing */
     void setModProvider(CommonModuleProviderIf *modProvider);

     ~ObjectStorMgr();

     /* Overrides from Module */
     virtual int  mod_init(SysParams const *const param) override;
     virtual void mod_startup() override;
     virtual void mod_shutdown() override;
     virtual void mod_enable_service() override;

     int run();

     /// Enables uturn testing for all sm service ops
     fds_bool_t testUturnAll;
     /// Enables uturn testing for put object ops
     fds_bool_t testUturnPutObj;

     TierEngine     *tierEngine;
     SmObjDb        *smObjDb;  // Object Index DB <ObjId, Meta-data + data_loc>
     checksum_calc   *chksumPtr;
     // Extneral plugin object to handle policy requests.
     VolPolicyServ  *omc_srv_pol;

     fds_bool_t isShuttingDown() const {
         return shuttingDown;
     }
     fds_bool_t popDirtyFlash(ObjectID **objId) {
         return dirtyFlashObjs->pop(*objId);
     }
     Error writeBackObj(const ObjectID &objId);

     Error regVol(const VolumeDesc& vdb) {
         return volTbl->registerVolume(vdb);
     }

    Error deregVol(fds_volid_t volId) {
        return volTbl->deregisterVolume(volId);
    }

    void quieseceIOsQos(fds_volid_t volId) {
        qosCtrl->quieseceIOs(volId);
    }

    Error modVolQos(fds_volid_t vol_uuid,
                    fds_uint64_t iops_min,
                    fds_uint64_t iops_max,
                    fds_uint32_t prio) {
        return qosCtrl->modifyVolumeQosParams(vol_uuid,
                    iops_min, iops_max, prio);
    }

    StorMgrVolume * getVol(fds_volid_t volId)
    {
        return volTbl->getVolume(volId);
    }


    Error regVolQos(fds_volid_t volId, fds::FDS_VolumeQueue * q) {
        return qosCtrl->registerVolume(volId, q);
    }
    Error deregVolQos(fds_volid_t volId) {
        return qosCtrl->deregisterVolume(volId);
    }
    SmVolQueue * getQueue(fds_volid_t volId) const {
        return static_cast<SmVolQueue*>(qosCtrl->getQueue(volId));
    }

     // We need to get this info out of this big class to avoid making this
     // class even bigger than it should.  Not much point for making it
     // private and need a get method to get it out.
     //
     StorMgrVolumeTable *sm_getVolTables() {
         return volTbl;
     }

     /**
      * A callback from stats collector to sample SM specific stats
      * @param timestamp is timestamp to path to every recordEvent()
      * method to stats collector when recording SM stats
      */
     void sampleSMStats(fds_uint64_t timestamp);

     Error getObjectInternal(SmIoGetObjectReq *getReq);
     Error putObjectInternal(SmIoPutObjectReq* putReq);
     Error deleteObjectInternal(SmIoDeleteObjectReq* delReq);
     Error addObjectRefInternal(SmIoAddObjRefReq* addRefReq);

     void putTokenObjectsInternal(SmIoReq* ioReq);
     void getTokenObjectsInternal(SmIoReq* ioReq);
     void snapshotTokenInternal(SmIoReq* ioReq);
     void applySyncMetadataInternal(SmIoReq* ioReq);
     void resolveSyncEntryInternal(SmIoReq* ioReq);
     void applyObjectDataInternal(SmIoReq* ioReq);
     void readObjectDataInternal(SmIoReq* ioReq);
     void readObjectMetadataInternal(SmIoReq* ioReq);
     void compactObjectsInternal(SmIoReq* ioReq);

     Error relocateObject(const ObjectID &objId,
                          diskio::DataTier from_tier,
                          diskio::DataTier to_tier);
     void handleDltUpdate();

     static void nodeEventOmHandler(int node_id,
                                    unsigned int node_ip_addr,
                                    int node_state,
                                    fds_uint32_t node_port,
                                    FDS_ProtocolInterface::FDSP_MgrIdType node_type);
     static Error volEventOmHandler(fds::fds_volid_t volume_id,
                                    fds::VolumeDesc *vdb,
                                    int vol_action,
                                    FDSP_NotifyVolFlag vol_flag,
                                    FDSP_ResultType resut);
     static void migrationEventOmHandler(bool dlt_type);
     static void dltcloseEventHandler(FDSP_DltCloseTypePtr& dlt_close,
                                      const std::string& session_uuid);
     void migrationSvcResponseCb(const Error& err, const MigrationStatus& status);

     virtual Error enqueueMsg(fds_volid_t volId, SmIoReq* ioReq);

     /* Made virtual for google mock */
     TVIRTUAL const DLT* getDLT();
     TVIRTUAL fds_token_id getTokenId(const ObjectID& objId);
     TVIRTUAL kvstore::TokenStateDBPtr getTokenStateDb();
     TVIRTUAL bool isTokenInSyncMode(const fds_token_id &tokId);

     Error putTokenObjects(const fds_token_id &token,
                           FDSP_MigrateObjectList &obj_list);

     const std::string getStorPrefix() {
         return modProvider_->get_fds_config()->get<std::string>("fds.sm.prefix");
     }

     NodeUuid getUuid() const;
     fds_bool_t amIPrimary(const ObjectID& objId);

     const TokenList& getTokensForNode(const NodeUuid &uuid) const;
     void getTokensForNode(TokenList *tl,
                           const NodeUuid &uuid,
                           fds_uint32_t index);
     fds_uint32_t getTotalNumTokens() const;

     SMCounters* getCounters() {
         return counters_.get();
     }

     virtual std::string log_string() {
         std::stringstream ret;
         ret << " ObjectStorMgr";
         return ret.str();
     }
     /*
      * Declare the FDSP interface class as a friend so it can access
      * the internal request tracking members.
      * TODO: Make this a nested class instead. No reason to make it
      * a separate class.
      */
     friend ObjectStorMgrI;
     friend class SmObjDb;
     friend class SmLoadProc;
     friend class SMSvcHandler;
};

class ObjectStorMgrI : virtual public FDSP_DataPathReqIf {
    public:
     ObjectStorMgrI();
     ~ObjectStorMgrI();
     void GetObject(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,
                    boost::shared_ptr<FDSP_GetObjType>& get_obj_req);

     void PutObject(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,
                    boost::shared_ptr<FDSP_PutObjType>& put_obj_req);

     void DeleteObject(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,
                       boost::shared_ptr<FDSP_DeleteObjType>& del_obj_req);

     void OffsetWriteObject(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,
                            boost::shared_ptr<FDSP_OffsetWriteObjType>& offset_write_obj_req);

     void RedirReadObject(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,
                          boost::shared_ptr<FDSP_RedirReadObjType>& redir_write_obj_req);

     void GetObjectMetadata(boost::shared_ptr<FDSP_GetObjMetadataReq>& metadata_req);

     void GetObject(const FDSP_MsgHdrType& fdsp_msg, const FDSP_GetObjType& get_obj_req) {
         // Don't do anything here. This stub is just to keep cpp compiler happy
     }
     void PutObject(const FDSP_MsgHdrType& fdsp_msg, const FDSP_PutObjType& put_obj_req) {
         // Don't do anything here. This stub is just to keep cpp compiler happy
     }
     void DeleteObject(const FDSP_MsgHdrType& fdsp_msg, const FDSP_DeleteObjType& del_obj_req) {
         // Don't do anything here. This stub is just to keep cpp compiler happy
     }
     void OffsetWriteObject(const FDSP_MsgHdrType&, const FDSP_OffsetWriteObjType&) {
         // Don't do anything here. This stub is just to keep cpp compiler happy
     }
     void RedirReadObject(const FDSP_MsgHdrType&, const FDSP_RedirReadObjType&) {
         // Don't do anything here. This stub is just to keep cpp compiler happy
     }
     void GetObjectMetadata(const FDSP_GetObjMetadataReq& metadata_req) {
         // Don't do anything here. This stub is just to keep cpp compiler happy
     }

     /* user defined methods */
     void GetObjectMetadataCb(const Error &e, SmIoReadObjectMetadata *read_data);

     void GetTokenMigrationStats(FDSP_TokenMigrationStats& _return,
                                 const FDSP_MsgHdrType& fdsp_msg) {
         // Don't do anything here. This stub is just to keep cpp compiler happy
     }

     void GetTokenMigrationStats(FDSP_TokenMigrationStats& _return,
                                 boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg);
};

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_STORMGR_H_
