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
// #include <Metrics.h>
// #include <IcePatch2/FileInfo.h>

#define FDS_STOR_MGR_LISTEN_PORT FDS_CLUSTER_TCP_PORT_SM
#define FDS_STOR_MGR_DGRAM_PORT FDS_CLUSTER_UDP_PORT_SM
#define FDS_MAX_WAITING_CONNS  10


using namespace FDS_ProtocolInterface;
// using namespace IceDelegate::FDS_ProtocolInterface;
using namespace fds;
using namespace osm;
using namespace std;
using namespace Ice;
// using namespace IceMX;
// using namespace IcePatch2;
// using namespace IceProxy::Ice;
// using namespace IceDelegateM::Ice;
// using namespace IceDelegateM::IceMX;

class ObjectStorMgr : virtual public Ice::Application {
public:
   fds_int32_t   sockfd;
   fds_uint32_t  num_threads;

   DiskMgr       *diskMgr;
   ObjectDB      *objStorDB;
   ObjectDB      *objIndexDB;
// IndexDb       volIndexDb;

//methods
   ObjectStorMgr();
   ~ObjectStorMgr();

   fds_int32_t  getSocket() { return sockfd; }   
   void PutObject(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr, const FDS_ProtocolInterface::FDSP_PutObjTypePtr& put_obj);
   void GetObject(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr, const FDS_ProtocolInterface::FDSP_GetObjTypePtr& get_obj);

   inline void swapMgrId(FDSP_MsgHdrType *fdsp_msg);

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

  ObjectStorMgrI();// { };
  ~ObjectStorMgrI();// { };

  // virtual void shutdown(const Ice::Current&);
  virtual void PutObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_PutObjTypePtr &put_obj, const Ice::Current&);
    // void PutObject(const ::FDS_ProtocolInterface::FDSP_MsgHdrTypePrx&, const ::FDS_ProtocolInterface::FDSP_PutObjTypePrx&, const ::Ice::Current& = ::Ice::Current()) { };
    // void PutObject(const FDSP_MsgHdrTypePrx &msg_hdr, const FDSP_PutObjTypePrx &put_obj, const ::Ice::Context* cont, ::IceInternal::InvocationObserver& Obs) { }

  virtual void GetObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_GetObjTypePtr& get_obj, const Ice::Current&);
    // void GetObject(const ::FDS_ProtocolInterface::FDSP_MsgHdrTypePrx&, const ::FDS_ProtocolInterface::FDSP_GetObjTypePrx&, const ::Ice::Current& = ::Ice::Current()) { };

  virtual void UpdateCatalogObject(const FDSP_MsgHdrTypePrx &msg_hdr, const FDSP_UpdateCatalogTypePrx& update_catalog , const Ice::Current&);
    // void UpdateCatalogObject(const ::FDS_ProtocolInterface::FDSP_MsgHdrTypePrx&, const ::FDS_ProtocolInterface::FDSP_UpdateCatalogTypePrx&, const ::Ice::Current& = ::Ice::Current()) { };

  virtual void OffsetWriteObject(const FDSP_MsgHdrTypePrx& msg_hdr, const FDSP_OffsetWriteObjTypePrx& offset_write_obj, const Ice::Current&);
    // void OffsetWriteObject(const ::FDS_ProtocolInterface::FDSP_MsgHdrTypePrx&, const ::FDS_ProtocolInterface::FDSP_OffsetWriteObjTypePrx&, const ::Ice::Current& = ::Ice::Current()) { };

  virtual void RedirReadObject(const FDSP_MsgHdrTypePrx &msg_hdr, const FDSP_RedirReadObjTypePrx& redir_read_obj, const Ice::Current&);
    // void RedirReadObject(const ::FDS_ProtocolInterface::FDSP_MsgHdrTypePrx&, const ::FDS_ProtocolInterface::FDSP_RedirReadObjTypePrx&, const ::Ice::Current& = ::Ice::Current()) { };

};
#endif
