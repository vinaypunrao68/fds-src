#ifndef __STOR_MGR_H__
#define __STOR_MGR_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "FDSP.h"
#include "stor_mgr_err.h"
#include "include/fds_volume.h"
#include "include/fds_types.h"
#include "fds_volume.h"
#include "ObjLoc.h"
#include "odb.h"
#include <unistd.h>
#include <assert.h>
#include "odb.h"
#include <iostream>
#include <Ice/Ice.h>
#include "util/Log.h"
#include <DiskMgr.h>
#include <StorMgrVolumes.h>
#include "lib/OMgrClient.h"
#include "util/concurrency/Mutex.h"
#include "StorMgrVolumes.h"

#define FDS_STOR_MGR_LISTEN_PORT FDS_CLUSTER_TCP_PORT_SM
#define FDS_STOR_MGR_DGRAM_PORT FDS_CLUSTER_UDP_PORT_SM
#define FDS_MAX_WAITING_CONNS  10


using namespace FDS_ProtocolInterface;
using namespace fds;
using namespace osm;
using namespace std;
using namespace Ice;

class ObjectStorMgr : virtual public Ice::Application {
public:
   fds_int32_t   sockfd;
   fds_uint32_t  num_threads;

   fds_mutex     *objStorMutex;

   DiskMgr       *diskMgr;
   ObjectDB      *objStorDB;
   ObjectDB      *objIndexDB;
   std::string stor_prefix;

   fds_uint32_t port_num; /* Data path port num */
   fds_uint32_t cp_port_num; /* Control path port num */

   StorMgrVolumeTable *volTbl;
   OMgrClient    *omClient;

  FDS_ProtocolInterface::FDSP_DataPathReqPtr fdspDataPathServer;
  FDS_ProtocolInterface::FDSP_DataPathRespPrx fdspDataPathClient; //For sending back the response to the SH/DM

  //methods
  ObjectStorMgr();
  ~ObjectStorMgr();

   fds_log* GetLog();
   fds_log *sm_log;

   fds_int32_t  getSocket() { return sockfd; }   
   void PutObject(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr, const FDS_ProtocolInterface::FDSP_PutObjTypePtr& put_obj);
   void GetObject(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr, const FDS_ProtocolInterface::FDSP_GetObjTypePtr& get_obj);

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

private :
   /*
    * Cmdline configurables
    */

   fds_sm_err_t getObjectInternal(FDSP_GetObjTypePtr get_obj_req, 
                       		  fds_uint32_t volid, 
                       		  fds_uint32_t transid, 
                       		  fds_uint32_t num_objs);
   fds_sm_err_t putObjectInternal(FDSP_PutObjTypePtr put_obj_req, 
                          	  fds_uint64_t volid, 
                                  fds_uint32_t num_objs);
   fds_sm_err_t checkDuplicate(FDS_ObjectIdType *object_id,
                               fds_uint32_t obj_len,
                               fds_char_t *data_object);

   fds_sm_err_t writeObject(FDS_ObjectIdType *object_id, 
                            fds_uint32_t obj_len, 
                            fds_char_t *data_object, 
                            FDS_DataLocEntry *data_loc);

   fds_sm_err_t writeObjLocation(FDS_ObjectIdType *object_id, 
                                 fds_uint32_t obj_len, 
                                 fds_uint32_t volid, 
                                 FDS_DataLocEntry  *data_loc);
};


class ObjectStorMgrI : public FDS_ProtocolInterface::FDSP_DataPathReq  
{
public:

  ObjectStorMgrI(const Ice::CommunicatorPtr& communicator);
  ~ObjectStorMgrI();
  Ice::CommunicatorPtr _communicator;

  // virtual void shutdown(const Ice::Current&);
  void PutObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_PutObjTypePtr &put_obj, const Ice::Current&);

  void GetObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_GetObjTypePtr& get_obj, const Ice::Current&);

  void UpdateCatalogObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_UpdateCatalogTypePtr& update_catalog , const Ice::Current&);

  void QueryCatalogObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_QueryCatalogTypePtr& query_catalog , const Ice::Current&);

  void OffsetWriteObject(const FDSP_MsgHdrTypePtr& msg_hdr, const FDSP_OffsetWriteObjTypePtr& offset_write_obj, const Ice::Current&);

  void RedirReadObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_RedirReadObjTypePtr& redir_read_obj, const Ice::Current&);

  void AssociateRespCallback(const Ice::Identity&, const Ice::Current&);
};


#endif
