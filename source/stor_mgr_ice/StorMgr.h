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
#include "odb.h"
#include <unistd.h>
#include <assert.h>
#include <iostream>
#include <Ice/Ice.h>
#include <util/Log.h>
#include "DiskMgr.h"
#include "StorMgrVolumes.h"
#include <disk-mgr/dm_service.h>
#include <disk-mgr/dm_io.h>

#include <include/fds_qos.h>
#include <include/qos_ctrl.h>
#include <include/fds_assert.h>
#include <utility>
#include <atomic>
#include <unordered_map>

/*
 * TODO: Move this header out of lib/
 * to include/ since it's linked by many.
 */
#include <lib/QoSWFQDispatcher.h>

/* TODO: avoid include across module, put API header file to include dir */
#include <lib/OMgrClient.h>
#include <concurrency/Mutex.h>

#define FDS_STOR_MGR_LISTEN_PORT FDS_CLUSTER_TCP_PORT_SM
#define FDS_STOR_MGR_DGRAM_PORT FDS_CLUSTER_UDP_PORT_SM
#define FDS_MAX_WAITING_CONNS  10

using namespace FDS_ProtocolInterface;
using namespace fds;
using namespace osm;
using namespace std;
using namespace Ice;

namespace fds {

  /*
   * Forward declaration of Ice interface class
   * used for friend declaration
   */
  class ObjectStorMgrI;

  class SMDiskReq : public diskio::DiskRequest
    {
    public:
      SMDiskReq(meta_vol_io_t       &vio,
                meta_obj_id_t       &oid,
                fds::ObjectBuf      *buf,
                bool                block)
	: diskio::DiskRequest(vio, oid, buf, block) {
      }
      ~SMDiskReq() { }

    void req_submit() {
      fdsio::Request::req_submit();
    }
    void req_complete() {
      fdsio::Request::req_complete();
    }
 
};


  class ObjectStorMgr : virtual public Ice::Application {
 private:
    typedef enum {
      NORMAL_MODE = 0,
      TEST_MODE   = 1,
      MAX
    } SmRunModes;

    fds_uint32_t port_num;     /* Data path port num */
    fds_uint32_t cp_port_num;  /* Control path port num */
    std::string  myIp;         /* This nodes local IP */
    std::string  stor_prefix;  /* Local storage prefix */
    SmRunModes   runMode;      /* Whether we're in a test mode or not */
    fds_uint32_t numTestVols;  /* Number of vols to use in test mode */

 public:
    fds_int32_t   sockfd;
    fds_uint32_t  num_threads;

   FDS_ProtocolInterface::FDSP_AnnounceDiskCapabilityPtr dInfo;
   
    fds_mutex     *objStorMutex;

    DiskMgr     *diskMgr;
    ObjectDB    *objStorDB;
    ObjectDB    *objIndexDB;

    StorMgrVolumeTable *volTbl;
    OMgrClient         *omClient;

    FDS_ProtocolInterface::FDSP_DataPathReqPtr fdspDataPathServer;
    FDS_ProtocolInterface::FDSP_DataPathRespPrx fdspDataPathClient; //For sending back the response to the SH/DM

    ObjectStorMgr();
    ~ObjectStorMgr();

    fds_log* GetLog();
    fds_log *sm_log;

    fds_int32_t  getSocket() { return sockfd; }   

    void PutObject(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                   const FDS_ProtocolInterface::FDSP_PutObjTypePtr& put_obj);
    void GetObject(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                   const FDS_ProtocolInterface::FDSP_GetObjTypePtr& get_obj);

    inline void swapMgrId(const FDSP_MsgHdrTypePtr& fdsp_msg);
    static void nodeEventOmHandler(int node_id,
                                   unsigned int node_ip_addr,
                                   int node_state,
                                   fds_uint32_t node_port,
                                   FDS_ProtocolInterface::FDSP_MgrIdType node_type);
    static void volEventOmHandler(fds::fds_volid_t volume_id,
                                  fds::VolumeDesc *vdb,
                                  int vol_action);

    virtual int run(int, char*[]);
    void interruptCallback(int);
    void          unitTest();

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

 private:
    fds_uint32_t totalRate;
    fds_uint32_t qosThrds;

    /*
     * Outstanding request tracking members.
     * TODO: We should have a better overall mechanism than
     * this. This is pretty slow and hackey. For example,
     * a map is only used for convienence.
     */
    typedef std::unordered_map<fds_uint64_t,
        FDS_ProtocolInterface::FDSP_MsgHdrTypePtr > WaitingReqMap;
    WaitingReqMap              waitingReqs;
    std::atomic<fds_uint64_t>  nextReqId;
    fds_mutex                 *waitingReqMutex;

    /*
     * Private request processing members.
     */
    Error getObjectInternal(FDSP_GetObjTypePtr getObjReq, 
                            fds_volid_t        volId, 
                            fds_uint32_t       transId, 
                            fds_uint32_t       numObjs);
    Error getObjectInternal(const SmIoReq& getReq);
    Error putObjectInternal(FDSP_PutObjTypePtr putObjReq, 
                            fds_volid_t        volId,
                            fds_uint32_t       transId,
                            fds_uint32_t       numObjs);
    Error putObjectInternal(const SmIoReq& putReq);
    Error checkDuplicate(const ObjectID& objId,
                         const ObjectBuf& objCompData);

    Error writeObjectLocation(const fds::ObjectID& objId, 
			      meta_obj_map_t* obj_map);

    Error readObjectLocation(const fds::ObjectID& objId, 
			      meta_obj_map_t* obj_map);


    Error writeObject(const ObjectID& objId,
		      const ObjectBuf& objCompData);

    Error readObject(const ObjectID& objId,
		     ObjectBuf& objCompData);


    /*
     * This inheritance is private because no one
     * else should need to care about the inheritance
     * knowledge.
     */
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
        dispatcher = new QoSWFQDispatcher(this, parentSm->totalRate, log);
        /* base class created stats, but they are disable by default */
        stats->enable();
      }
      ~SmQosCtrl() {
	if (stats)
	  stats->disable();
      }

      Error processIO(FDS_IOType* _io) {
        Error err(ERR_OK);
        SmIoReq *io = static_cast<SmIoReq*>(_io);

        if (io->io_type == FDS_IO_READ) {
          FDS_PLOG(FDS_QoSControl::qos_log) << "Processing a get request";
          err = parentSm->getObjectInternal(*io);
        } else if (io->io_type == FDS_IO_WRITE) {
          FDS_PLOG(FDS_QoSControl::qos_log) << "Processing a put request";
          err = parentSm->putObjectInternal(*io);
        } else {
          assert(0);
        }

        return err;
      }

      Error markIODone(const FDS_IOType& _io) {
	Error err(ERR_OK);
	stats->recordIO(_io.io_vol_id, 0);
	return err;
      }

    };

 private:
    /*
     * TODO: Just make a single queue for now for testing purposes.
     */
    SmQosCtrl  *qosCtrl;
  };

  class ObjectStorMgrI : public FDS_ProtocolInterface::FDSP_DataPathReq {
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

    void AssociateRespCallback(const Ice::Identity&, const Ice::Current&);
  };

}  // namespace fds

#endif  // SOURCE_STOR_MGR_ICE_STORMGR_H_
