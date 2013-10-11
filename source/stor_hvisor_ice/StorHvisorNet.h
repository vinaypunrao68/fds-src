#ifndef __StorHvisorNet_h__
#define __StorHvisorNet_h__
#include <Ice/ProxyF.h>
#include <Ice/ObjectF.h>
#include <Ice/Exception.h>
#include <Ice/LocalObject.h>
#include <Ice/StreamHelpers.h>
#include <Ice/Proxy.h>
#include <Ice/Object.h>
#include <Ice/Outgoing.h>
#include <Ice/OutgoingAsync.h>
#include <Ice/Incoming.h>
#include <Ice/IncomingAsync.h>
#include <Ice/Direct.h>
#include <Ice/FactoryTableInit.h>
#include <IceUtil/ScopedArray.h>
#include <IceUtil/Optional.h>
#include <Ice/StreamF.h>
#include <Ice/UndefSysMacros.h>
#include <IceUtil/IceUtil.h>
#include <Ice/Ice.h>
#include <fdsp/FDSP.h>
#include <list>
#include "RPC_EndPoint.h"
#include "StorHvDataPlace.h"
#include <fds_err.h>
#include <fds_types.h>

/* TODO: Use API header in include directory. */
#include <lib/OMgrClient.h>
#include "StorHvVolumes.h"
#include "StorHvisorCPP.h" 

#include <map>
// #include "util/concurrency/Thread.h"
#include <concurrency/Synchronization.h>

#ifndef ICE_IGNORE_VERSION
#   if ICE_INT_VERSION / 100 != 305
#       error Ice version mismatch!
#   endif
#   if ICE_INT_VERSION % 100 > 50
#       error Beta header file detected
#   endif
#   if ICE_INT_VERSION % 100 < 0
#       error Ice patch level mismatch!
#   endif
#endif


#define  FDS_NODE_OFFLINE               0
#define  FDS_NODE_ONLINE                1

#define  FDS_STORAGE_TYPE_SSD           1
#define  FDS_STORAGE_TYPE_FLASH         2
#define  FDS_STORAGE_TYPE_HARD          3
#define  FDS_STORAGE_TYPE_TAPE          4


#define  FDS_NODE_TYPE_PRIM             1
#define  FDS_NODE_TYPE_SEND             2
#define  FDS_NODE_TYPE_BCKP             3


#define  FDS_SUCCESS                    0
#define  FDS_FAILURE                    1

#define  FDS_TIMER_TIMEOUT              1

#define HVISOR_SECTOR_SIZE 		512

typedef unsigned int volid_t;

using namespace FDS_ProtocolInterface;
using namespace std;

class FDSP_DataPathRespCbackI : public FDSP_DataPathResp
{
public:
    void GetObjectResp(const FDSP_MsgHdrTypePtr&, const FDSP_GetObjTypePtr&, const Ice::Current&);

    void PutObjectResp(const FDSP_MsgHdrTypePtr&, const FDSP_PutObjTypePtr&, const Ice::Current&);

    void UpdateCatalogObjectResp(const FDSP_MsgHdrTypePtr& fdsp_msg, const FDSP_UpdateCatalogTypePtr& cat_obj_req, const Ice::Current &); 

    void QueryCatalogObjectResp(const FDSP_MsgHdrTypePtr& fdsp_msg, const FDSP_QueryCatalogTypePtr& cat_obj_req, const Ice::Current &);

    void OffsetWriteObjectResp(const FDSP_MsgHdrTypePtr& fdsp_msg, const FDSP_OffsetWriteObjTypePtr& offset_write_obj_req, const Ice::Current &) {

    }
    void RedirReadObjectResp(const FDSP_MsgHdrTypePtr& fdsp_msg, const FDSP_RedirReadObjTypePtr& redir_write_obj_req, const Ice::Current &)
    { 
    }
};


typedef struct {
  double   hash_high;
  double   hash_low;
  char    conflict_id;
} fds_object_id_t;


typedef union {

  fds_object_id_t obj_id; // object id fields
  unsigned char bytes[20]; // byte array

} fds_doid_t;

typedef unsigned char doid_t[20];

/*************************************************************************** */


class StorHvCtrl {


public:
  /*
   * Defines specific test modes used to
   * construct the object.
   */
  typedef enum {
    DATA_MGR_TEST, /* Only communicate with DMs */
    STOR_MGR_TEST, /* Only communicate with SMs */
    TEST_BOTH,     /* Communication with DMs and SMs */
    NORMAL,        /* Normal, non-test mode */
    MAX
  } sh_comm_modes;
  
  StorHvCtrl(int argc, char *argv[]);
  StorHvCtrl(int argc, char *argv[], sh_comm_modes _mode);
  StorHvCtrl(int argc,
             char *argv[],
             sh_comm_modes _mode,
             fds_uint32_t sm_port_num,
             fds_uint32_t dm_port_num);
  ~StorHvCtrl();	
  hv_create_blkdev cr_blkdev;
  hv_delete_blkdev del_blkdev;
  
  // Data Members
  Ice::CommunicatorPtr _communicator;
  StorHvDataPlacement        *dataPlacementTbl;
  FDS_RPC_EndPointTbl        *rpcSwitchTbl; // RPC calls Switch Table
  StorHvVolumeTable          *vol_table;  
  OMgrClient                 *om_client;
  FDS_ProtocolInterface::FDSP_AnnounceDiskCapabilityPtr dInfo;

  void  InitIceObjects();
  void InitDmMsgHdr(const FDSP_MsgHdrTypePtr &msg_hdr);
  void InitSmMsgHdr(const FDSP_MsgHdrTypePtr &msg_hdr);
  
  void fbd_process_req_timeout(unsigned long arg);

  int fds_move_wr_req_state_machine(const FDSP_MsgHdrTypePtr& rx_msg);  
  int fds_process_get_obj_resp(const FDSP_MsgHdrTypePtr& rd_msg, const FDSP_GetObjTypePtr& get_obj_rsp );
  int fds_process_put_obj_resp(const FDSP_MsgHdrTypePtr& rx_msg,const  FDSP_PutObjTypePtr& put_obj_rsp );
  int fds_process_update_catalog_resp(const FDSP_MsgHdrTypePtr& rx_msg,const  FDSP_UpdateCatalogTypePtr& cat_obj_rsp );
  fds_log* GetLog();
  void StartOmClient();
  sh_comm_modes GetRunTimeMode() { return mode; }

private:
  fds_log *sh_log;
  sh_comm_modes mode;
  IceUtil::CtrlCHandler *shCtrlHandler;
};
#endif
