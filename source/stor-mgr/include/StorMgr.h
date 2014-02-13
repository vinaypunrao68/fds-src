/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_STORMGR_H_
#define SOURCE_STOR_MGR_STORMGR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fdsp/FDSP_types.h>
#include <fdsp/FDSP_DataPathReq.h>
#include <fdsp/FDSP_DataPathResp.h>
#include "stor_mgr_err.h"
#include <fds_volume.h>
#include <fds_types.h>
#include "ObjLoc.h"
#include <odb.h>
#include <unistd.h>
#include <assert.h>
#include <iostream>
#include <util/Log.h>
#include "StorMgrVolumes.h"
#include "SmObjDb.h"
#include <persistent_layer/dm_service.h>
#include <persistent_layer/dm_io.h>
#include <fds_migration.h>

#include <fds_qos.h>
#include <qos_ctrl.h>
#include <fds_obj_cache.h>
#include <fds_assert.h>
#include <fds_config.hpp>
#include <fds_stopwatch.h>

#include <utility>
#include <atomic>
#include <unordered_map>
#include <ObjStats.h>

/*
 * TODO: Move this header out of lib/
 * to include/ since it's linked by many.
 */
#include <lib/qos_htb.h>
#include <lib/qos_min_prio.h>
#include <lib/QoSWFQDispatcher.h>

/* TODO: avoid include across module, put API header file to include dir */
#include <lib/OMgrClient.h>
#include <concurrency/Mutex.h>

#include <TierEngine.h>
#include <ObjRank.h>

#include <fds_module.h>
#include <fds_process.h>

#include <NetSession.h>

#undef FDS_TEST_SM_NOOP      /* if defined, IO completes as soon as it arrives to SM */

#define FDS_STOR_MGR_LISTEN_PORT FDS_CLUSTER_TCP_PORT_SM
#define FDS_STOR_MGR_DGRAM_PORT FDS_CLUSTER_UDP_PORT_SM
#define FDS_MAX_WAITING_CONNS  10

using namespace FDS_ProtocolInterface;
using namespace fds;
using namespace osm;
using namespace std;
using namespace diskio;

namespace fds {

void log_ocache_stats();

/*
 * Forward declarations
 */
class ObjectStorMgrI;
class TierEngine;
class ObjectRankEngine;


class SmPlReq : public diskio::DiskRequest {
 public:
    /*
     * TODO: This defaults to disk at the moment...
     * need to specify any tier, specifically for
     * read
     */
    SmPlReq(meta_vol_io_t   &vio,
            meta_obj_id_t   &oid,
            ObjectBuf        *buf,
            fds_bool_t        block)
    : diskio::DiskRequest(vio, oid, buf, block) {
    }
    SmPlReq(meta_vol_io_t   &vio,
            meta_obj_id_t   &oid,
            ObjectBuf        *buf,
            fds_bool_t        block,
            diskio::DataTier  tier)
    : diskio::DiskRequest(vio, oid, buf, block, tier) {
    }
    ~SmPlReq() { }

    void req_submit() {
        fdsio::Request::req_submit();
    }
    void req_complete() {
        fdsio::Request::req_complete();
    }
    void setTierFromMap() {
        datTier = static_cast<diskio::DataTier>(idx_vmap.obj_tier);
    }
};

/**
 * @brief Storage manager counters
 */
class SMCounters : public FdsCounters
{
 public:
  SMCounters(const std::string &id, FdsCountersMgr *mgr)
      : FdsCounters(id, mgr),
        put_reqs("put_reqs", this),
        get_reqs("get_reqs", this),
        puts_latency("puts_latency", this)
  {
  }

  NumericCounter put_reqs;
  NumericCounter get_reqs;
  LatencyCounter puts_latency;
};


class ObjectStorMgr :
        public FdsProcess,
        public SmIoReqHandler,
        public Module // todo: We shouldn't be deriving module here.  ObjectStorMgr is
                      // an FDSProcess, it contains Modules
        {
 private:
    typedef enum {
        NORMAL_MODE = 0,
        TEST_MODE   = 1,
        MAX
    } SmRunModes;

    /*
     * OM/boostrap related members
     */
    OMgrClient         *omClient;

    /*
     * Local storage members
     */
    // TransJournal<ObjectID, ObjectIdJrnlEntry> *omJrnl;
    fds_mutex *objStorMutex;
    ObjectDB  *objStorDB;
    ObjectDB  *objIndexDB;

    /*
     * FDSP RPC members
     * The map is used for sending back the response to the
     * appropriate SH/DM
     */
    boost::shared_ptr<netSessionTbl> nst_;
    boost::shared_ptr<FDSP_DataPathReqIf> datapath_handler_;
    netDataPathServerSession *datapath_session_;

    /* Cluster communication manager */
    ClusterCommMgrPtr clust_comm_mgr_;

    /* Migrations related */
    FdsMigrationSvcPtr migrationSvc_;

    /* Counters */
    SMCounters counters_;

    /* Helper for accessing datapth response client */
    inline boost::shared_ptr<FDS_ProtocolInterface::FDSP_DataPathRespClient> 
    fdspDataPathClient(const std::string& session_uuid)
    {
        return datapath_session_->getRespClient(session_uuid);
    }
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

            //dispatcher = new QoSMinPrioDispatcher(this, log, 3000);
            dispatcher = new QoSWFQDispatcher(this, parentSm->totalRate, 10, log);
            //dispatcher = new QoSHTBDispatcher(this, log, 150);

            /* base class created stats, but they are disable by default */
            stats->enable();
        }
        ~SmQosCtrl() {
            if (stats)
                stats->disable();
        }

        Error processIO(FDS_IOType* _io);

        Error markIODone(const FDS_IOType& _io) {
            Error err(ERR_OK);
            dispatcher->markIODone((FDS_IOType *)&_io);
            stats->recordIO(_io.io_vol_id, _io.io_total_time);
            return err;
        }

        Error markIODone(const FDS_IOType &_io,
                diskio::DataTier  tier) {
            Error err(ERR_OK);
            dispatcher->markIODone((FDS_IOType *)&_io);
            stats->recordIO(_io.io_vol_id, 0, tier, _io.io_type);
            return err;
        }
    };

    SmQosCtrl  *qosCtrl;

    SmVolQueue *sysTaskQueue;

    /*
     * Tiering related members
     */
    ObjectRankEngine *rankEngine;

    FdsObjectCache *objCache;

    /*
     * Flash write-back members.
     * TODO: These should probably be in the persistent layer
     * but is easier here for now since needs the index.
     */
    typedef boost::lockfree::queue<ObjectID*> ObjQueue;  /* Dirty list type */
    static void writeBackFunc(ObjectStorMgr *parent);    /* Function for write-back */
    std::atomic_bool  shuttingDown;      /* SM shut down flag for write-back thread */
    fds_uint32_t      numWBThreads;      /* Number of write-back threads */
    fds_threadpool   *writeBackThreads;  /* Threads performing write-back */
    fds_uint32_t      maxDirtyObjs;      /* Max dirty list size */
    ObjQueue         *dirtyFlashObjs;    /* Flash's dirty list */

    /*
     * Outstanding request tracking members.
     * TODO: We should have a better overall mechanism than
     * this. This is pretty slow and hackey. The networking
     * layer should eventually handle this, not SM.
     */
    typedef std::unordered_map<fds_uint64_t,
            FDS_ProtocolInterface::FDSP_MsgHdrTypePtr> WaitingReqMap;
    WaitingReqMap              waitingReqs;
    std::atomic<fds_uint64_t>  nextReqId;
    fds_mutex                 *waitingReqMutex;

    /*
     * Local perf stat collection
     */
    enum perfMigOp {
        flashToDisk,
        diskToFlash,
        invalidMig
    };
    PerfStats *perfStats;

    SysParams *sysParams;

    /*
     * Private request processing members.
     */
    Error getObjectInternal(FDSP_GetObjTypePtr getObjReq, 
            fds_volid_t        volId,
            fds_uint32_t       transId,
            fds_uint32_t       numObjs);
    Error putObjectInternal(FDSP_PutObjTypePtr putObjReq, 
            fds_volid_t        volId,
            fds_uint32_t       transId,
            fds_uint32_t       numObjs);
    Error deleteObjectInternal(FDSP_DeleteObjTypePtr delObjReq, 
            fds_volid_t        volId,
            fds_uint32_t       transId);
    Error checkDuplicate(const ObjectID  &objId,
            const ObjectBuf &objCompData);
    Error writeObjectLocation(const ObjectID &objId, 
            meta_obj_map_t *obj_map,
            fds_bool_t      append);
    Error readObjectLocations(const ObjectID &objId, 
            meta_obj_map_t *objMaps);
    Error readObjectLocations(const ObjectID     &objId,
            diskio::MetaObjMap &objMaps);
    Error deleteObjectLocation(const ObjectID     &objId);
    Error writeObject(const ObjectID   &objId,
            const ObjectBuf  &objCompData,
            fds_volid_t       volId,
            diskio::DataTier &tier);
    Error writeObject(const ObjectID  &objId, 
            const ObjectBuf &objData,
            diskio::DataTier tier);
    Error readObject(const ObjectID &objId,
            ObjectBuf      &objCompData);

    inline fds_uint32_t getSysTaskIopsMin() {
        return totalRate/10; // 10% of total rate
    }
    
    inline fds_uint32_t getSysTaskIopsMax() {
        return totalRate/5; // 20% of total rate
    }

    inline fds_uint32_t getSysTaskPri() {
        return FdsSysTaskPri;
    }


 protected:
    void setup_datapath_server(const std::string &ip);
    void setup_migration_svc();

 public:

    ObjectStorMgr(int argc, char *argv[],
                  const std::string &default_config_path,
                  const std::string &base_path);
    ~ObjectStorMgr();

    /* From FdsProcess */
    virtual void setup(int argc, char *argv[], fds::Module **mod_vec) override;
    virtual void run() override;
    virtual void interrupt_cb(int signum) override;

    /* From Module */
    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();

    fds_log* GetLog() {return sm_log;}
    fds_log *sm_log;
    TierEngine     *tierEngine;
    SmObjDb        *smObjDb;
    /*
     * stats  class 
     */
    ObjStatsTracker   *objStats;


    fds_bool_t isShuttingDown() const {
        return shuttingDown;
    }
    fds_bool_t popDirtyFlash(ObjectID **objId) {
        return dirtyFlashObjs->pop(*objId);
    }
    Error writeBackObj(const ObjectID &objId);

    /*
     * Public volume reg handlers
     */
    void regVolHandler(volume_event_handler_t volHndlr) {
        omClient->registerEventHandlerForVolEvents(volHndlr);
    }

    Error regVol(const VolumeDesc& vdb) {
        return volTbl->registerVolume(vdb);
    }

    Error deregVol(fds_volid_t volId) {
        return volTbl->deregisterVolume(volId);
    }
    // We need to get this info out of this big class to avoid making this
    // class even bigger than it should.  Not much point for making it
    // private and need a get method to get it out.
    //
    StorMgrVolumeTable *sm_getVolTables() {
        return volTbl;
    }

    void PutObject(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
            const FDS_ProtocolInterface::FDSP_PutObjTypePtr& put_obj);
    void GetObject(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
            const FDS_ProtocolInterface::FDSP_GetObjTypePtr& get_obj);
    void DeleteObject(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
            const FDS_ProtocolInterface::FDSP_DeleteObjTypePtr& del_obj);
    Error getObjectInternal(SmIoReq* getReq);
    Error putObjectInternal(SmIoReq* putReq);
    Error deleteObjectInternal(SmIoReq* delReq);
    Error putTokenObjectsInternal(SmIoReq* ioReq);
    Error getTokenObjectsInternal(SmIoReq* ioReq);
    Error relocateObject(const ObjectID &objId,
            diskio::DataTier from_tier,
            diskio::DataTier to_tier);

    inline void swapMgrId(const FDSP_MsgHdrTypePtr& fdsp_msg);
    static void nodeEventOmHandler(int node_id,
            unsigned int node_ip_addr,
            int node_state,
            fds_uint32_t node_port,
            FDS_ProtocolInterface::FDSP_MgrIdType node_type);
    static void volEventOmHandler(fds::fds_volid_t volume_id,
            fds::VolumeDesc *vdb,
            int vol_action,
            FDSP_ResultType resut);
    static void migrationEventOmHandler(bool dlt_type);
    void migrationSvcResponseCb(const Error& err);

    virtual Error enqueueMsg(fds_volid_t volId, SmIoReq* ioReq);

    Error retrieveTokenObjects(const fds_token_id &token, 
                             const size_t &max_size, 
                             FDSP_MigrateObjectList &obj_list, 
                             SMTokenItr &itr);

    Error putTokenObjects(const fds_token_id &token, 
                          FDSP_MigrateObjectList &obj_list);
    void unitTest();
    Error readObject(const ObjectID   &objId,
            ObjectBuf        &objCompData,
            diskio::DataTier &tier);

    const std::string getStorPrefix() {
        return conf_helper_.get<std::string>("prefix");
    }

    FdsObjectCache *getObjCache() {
        return objCache;
    }

    virtual std::string log_string()
    {
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

    void GetObject(const FDSP_MsgHdrType& fdsp_msg, const FDSP_GetObjType& get_obj_req) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }
    void PutObject(const FDSP_MsgHdrType& fdsp_msg, const FDSP_PutObjType& put_obj_req) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }
    void DeleteObject(const FDSP_MsgHdrType& fdsp_msg, const FDSP_DeleteObjType& del_obj_req) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }
    void OffsetWriteObject(const FDSP_MsgHdrType& fdsp_msg, const FDSP_OffsetWriteObjType& offset_write_obj_req) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }
    void RedirReadObject(const FDSP_MsgHdrType& fdsp_msg, const FDSP_RedirReadObjType& redir_write_obj_req) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }
};

}  // namespace fds

#endif  // SOURCE_STOR_MGR_STORMGR_H_
