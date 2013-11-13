/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_ICE_STORMGR_H_
#define SOURCE_STOR_MGR_ICE_STORMGR_H_

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
#include <fdsp/FDSP.h>
#include "stor_mgr_err.h"
#include <fds_volume.h>
#include <fds_types.h>
#include "ObjLoc.h"
#include <odb.h>
#include <unistd.h>
#include <assert.h>
#include <iostream>
#include <Ice/Ice.h>
#include <util/Log.h>
#include "StorMgrVolumes.h"
#include <persistent_layer/dm_service.h>
#include <persistent_layer/dm_io.h>

#include <fds_qos.h>
#include <qos_ctrl.h>
#include <fds_assert.h>
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

#undef FDS_TEST_SM_NOOP      /* if defined, IO completes as soon as it arrives to SM */

#define FDS_STOR_MGR_LISTEN_PORT FDS_CLUSTER_TCP_PORT_SM
#define FDS_STOR_MGR_DGRAM_PORT FDS_CLUSTER_UDP_PORT_SM
#define FDS_MAX_WAITING_CONNS  10

using namespace FDS_ProtocolInterface;
using namespace fds;
using namespace osm;
using namespace std;
using namespace Ice;
using namespace diskio;

namespace fds {

  /*
   * Forward declaration of Ice interface class
   * used for friend declaration
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


  class ObjectStorMgr : virtual public Ice::Application {
 private:
    typedef enum {
      NORMAL_MODE = 0,
      TEST_MODE   = 1,
      MAX
    } SmRunModes;

    /*
     * Command line settable members
     */
    fds_uint32_t port_num;     /* Data path port num */
    fds_uint32_t cp_port_num;  /* Control path port num */
    std::string  myIp;         /* This nodes local IP */
    std::string  stor_prefix;  /* Local storage prefix */
    SmRunModes   runMode;      /* Whether we're in a test mode or not */
    fds_uint32_t numTestVols;  /* Number of vols to use in test mode */

    /*
     * OM/boostrap related members
     */
    OMgrClient         *omClient;

    /*
     * Local storage members
     */
    fds_mutex *objStorMutex;
    ObjectDB  *objStorDB;
    ObjectDB  *objIndexDB;

    /*
     * Ice/network members
     * The map is used for sending back the response to the
     * appropriate SH/DM
     */
    FDS_ProtocolInterface::FDSP_DataPathReqPtr fdspDataPathServer;
    std::unordered_map<std::string,
        FDS_ProtocolInterface::FDSP_DataPathRespPrx> fdspDataPathClient;

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

        //dispatcher = new QoSMinPrioDispatcher(this, log, 500);
       dispatcher = new QoSWFQDispatcher(this, 500, 20, log);
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

    /*
     * Tiering related members
     */
    ObjectRankEngine *rankEngine;

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
    Error checkDuplicate(const ObjectID  &objId,
                         const ObjectBuf &objCompData);
    Error writeObjectLocation(const ObjectID &objId, 
                              meta_obj_map_t *obj_map,
                              fds_bool_t      append);
    Error readObjectLocations(const ObjectID &objId, 
                              meta_obj_map_t *objMaps);
    Error readObjectLocations(const ObjectID     &objId,
                              diskio::MetaObjMap &objMaps);
    Error writeObject(const ObjectID   &objId,
                      const ObjectBuf  &objCompData,
                      fds_volid_t       volId,
                      diskio::DataTier &tier);
    Error writeObject(const ObjectID  &objId, 
                      const ObjectBuf &objData,
                      diskio::DataTier tier);
    Error readObject(const ObjectID &objId,
                     ObjectBuf      &objCompData);
    Error readObject(const ObjectID   &objId,
                     ObjectBuf        &objCompData,
                     diskio::DataTier &tier);

 public:

    ObjectStorMgr();
    ~ObjectStorMgr();

    fds_log* GetLog();
    fds_log *sm_log;
    TierEngine     *tierEngine;
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
    Error getObjectInternal(SmIoReq* getReq);
    Error putObjectInternal(SmIoReq* putReq);
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
                                  int vol_action);

    int run(int, char*[]);
    void interruptCallback(int);
    void unitTest();

    const std::string& getStorPrefix() const {
      return stor_prefix;
    }

    /*
     * Declare the Ice interface class as a friend so it can access
     * the internal request tracking members.
     * TODO: Make this a nested class instead. No reason to make it
     * a separate class.
     */
    friend ObjectStorMgrI;
  };

  class ObjectStorMgrI : public FDS_ProtocolInterface::FDSP_DataPathReq {
  private:
    

 public:
    ObjectStorMgrI(const Ice::CommunicatorPtr& communicator);
    ~ObjectStorMgrI();
    Ice::CommunicatorPtr _communicator;

    // virtual void shutdown(const Ice::Current&);
    void PutObject(const FDSP_MsgHdrTypePtr &msg_hdr,
                   const FDSP_PutObjTypePtr &put_obj, const Ice::Current&);

    void GetObject(const FDSP_MsgHdrTypePtr &msg_hdr,
                   const FDSP_GetObjTypePtr& get_obj, const Ice::Current&);

    void UpdateCatalogObject(const FDSP_MsgHdrTypePtr &msg_hdr,
                             const FDSP_UpdateCatalogTypePtr& update_catalog,
                             const Ice::Current&);

    void QueryCatalogObject(const FDSP_MsgHdrTypePtr &msg_hdr,
                            const FDSP_QueryCatalogTypePtr& query_catalog,
                            const Ice::Current&);

    void OffsetWriteObject(const FDSP_MsgHdrTypePtr& msg_hdr,
                           const FDSP_OffsetWriteObjTypePtr& offset_write_obj,
                           const Ice::Current&);

    void RedirReadObject(const FDSP_MsgHdrTypePtr &msg_hdr,
                         const FDSP_RedirReadObjTypePtr& redir_read_obj,
                         const Ice::Current&);

    void AssociateRespCallback(const Ice::Identity&,
			       const std::string& src_node_name,
			       const Ice::Current&);
  };

}  // namespace fds

#endif  // SOURCE_STOR_MGR_ICE_STORMGR_H_
