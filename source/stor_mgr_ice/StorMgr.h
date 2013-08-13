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
#include "ObjLoc.h"
#include "odb.h"
#include <unistd.h>
#include <assert.h>
#include "odb.h"
#include <iostream>
#include <Ice/Ice.h>
#include <DiskMgr.h>

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

   DiskMgr       *diskMgr;
   ObjectDB      *objStorDB;
   ObjectDB      *objIndexDB;
// IndexDb       volIndexDb;

  FDS_ProtocolInterface::FDSP_DataPathReqPtr fdspDataPathServer;
  FDS_ProtocolInterface::FDSP_DataPathRespPrx fdspDataPathClient; //For sending back the response to the SH/DM

//methods
   ObjectStorMgr();
   ~ObjectStorMgr();

   fds_int32_t  getSocket() { return sockfd; }   
   void PutObject(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr, const FDS_ProtocolInterface::FDSP_PutObjTypePtr& put_obj);
   void GetObject(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr, const FDS_ProtocolInterface::FDSP_GetObjTypePtr& get_obj);

   inline void swapMgrId(const FDSP_MsgHdrTypePtr& fdsp_msg);

   virtual int run(int, char*[]);
   void interruptCallback(int);
   void          unitTest();
private :
   fds_sm_err_t getObjectInternal(FDSP_GetObjTypePtr get_obj_req, 
                       		  fds_uint32_t volid, 
                       		  fds_uint32_t num_objs);
   fds_sm_err_t putObjectInternal(FDSP_PutObjTypePtr put_obj_req, 
                          	  fds_uint32_t volid, 
                                  fds_uint32_t num_objs);
   fds_sm_err_t checkDuplicate(FDS_ObjectIdType *object_id,
                               fds_uint32_t obj_len,
                               fds_char_t *data_object);

   fds_sm_err_t writeObject(FDS_ObjectIdType *object_id, 
                            fds_uint32_t obj_len, 
                            fds_char_t *data_object, 
                            FDSDataLocEntryType *data_loc);

   fds_sm_err_t writeObjLocation(FDS_ObjectIdType *object_id, 
                                 fds_uint32_t obj_len, 
                                 fds_uint32_t volid, 
                                 FDSDataLocEntryType *data_loc);
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

  void OffsetWriteObject(const FDSP_MsgHdrTypePtr& msg_hdr, const FDSP_OffsetWriteObjTypePtr& offset_write_obj, const Ice::Current&);

  void RedirReadObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_RedirReadObjTypePtr& redir_read_obj, const Ice::Current&);

  void AssociateRespCallback(const Ice::Identity&, const Ice::Current&);
};


#endif
