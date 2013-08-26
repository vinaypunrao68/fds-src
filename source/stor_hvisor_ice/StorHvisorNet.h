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
#include "list.h"
#include <list>
#include "RPC_EndPoint.h"
#include "StorHvDataPlace.h"
#include "include/fds_err.h"
#include "include/fds_types.h"

#include "VolumeCatalogCache.h"

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

#define  FDS_MAX_DM_NODES_PER_CLST      16
#define  FDS_MAX_SM_NODES_PER_CLST      16
#define  FDS_MAC_DM_ENTRIES             256
#define  FDS_MAC_SM_ENTRIES             256
#define  FDS_READ_WRITE_LOG_ENTRIES 	256
#define  FDS_TRANS_EMPTY                0x00
#define  FDS_TRANS_OPEN                 0x1
#define  FDS_TRANS_OPENED               0x2
#define  FDS_TRANS_COMMITTED            0x3
#define  FDS_TRANS_SYNCED               0x4
#define  FDS_TRANS_DONE                 0x5
#define  FDS_TRANS_VCAT_QUERY_PENDING   0x6
#define  FDS_TRANS_GET_OBJ	        0x7


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

#define  FDS_MIN_ACK                    1
#define  FDS_CLS_ACK                    0
#define  FDS_SET_ACK                    1

#define  FDS_COMMIT_MSG_SENT            1
#define  FDS_COMMIT_MSG_ACKED           2

#define HVISOR_SECTOR_SIZE 		512

typedef unsigned int volid_t;
//typedef void (*complete_req_cb_t)(void *arg1, void *arg2, void *treq, int res);

using namespace FDS_ProtocolInterface;
using namespace std;;

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

#if 0
typedef double                     td_sector_t;
typedef struct fbd_request            fbd_request_t;
typedef struct td_image_handle       td_image_t;


/*  these need to be removed once andrew  cleans up the DM */

struct fbd_request {
        int                          op;
        char                        *buf;
        td_sector_t                  sec;
        int                          secs;

        short                      blocked; /* blocked on a dependency */

        td_image_t                  *image;

        void * /*td_callback_t*/     cb;
        void                        *cb_data;

        double                     id;
        int                          sidx;
        void                        *privateI;

#ifdef MEMSHR
        share_tuple_t                memshr_hnd;
#endif
};
#endif

typedef void *vvc_vhdl_t;
struct fbd_device {

  	int dev_id;
    pthread_mutex_t tx_lock;
    int blocksize;
    double bytesize;
    int xmit_timeout;
    int sock;
    long int   src_ip_addr;
    long int   tcp_destAddr;
    long int   udp_destAddr;
        int stor_mgr_port;
        int data_mgr_port;
    int        proto_type;
    /*  vvc  defs */
    volid_t     vol_id;
    vvc_vhdl_t  vhdl;
  pthread_t rx_thread;

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

typedef void (*complete_req_cb_t)(void *arg1, void *arg2, fbd_request_t *treq, int res);
typedef unsigned char doid_t[20];

/*************************************************************************** */
class FDSP_IpNode {
public:
        long  ipAddr;
        short  ack_status;
        short  commit_status;
};

enum FDS_IO_Type {
   FDS_IO_READ,
   FDS_IO_WRITE,
   FDS_IO_REDIR_READ,
   FDS_IO_OFFSET_WRITE
};

class   StorHvJournalEntry {
public:
        short  replc_cnt;
        short  sm_ack_cnt;
        short  dm_ack_cnt;
        short  dm_commit_cnt;
        short  trans_state;
        FDS_IO_Type   op;
        FDS_ObjectIdType data_obj_id;
        int      data_obj_len;
        void     *fbd_ptr;
        void     *read_ctx;
        void     *write_ctx;
	complete_req_cb_t comp_req;
	void 	*comp_arg1;
	void 	*comp_arg2;
        FDSP_MsgHdrTypePtr     sm_msg;
        FDSP_MsgHdrTypePtr     dm_msg;
        struct   timer_list *p_ti;
        int      lt_flag;
        int      st_flag;
        short    num_dm_nodes;
        FDSP_IpNode    dm_ack[FDS_MAX_DM_NODES_PER_CLST];
        short    num_sm_nodes;
        FDSP_IpNode    sm_ack[FDS_MAX_SM_NODES_PER_CLST];
};

class StorHvJournal {
public:
 	StorHvJournal();
 	~StorHvJournal();
	StorHvJournalEntry  rwlog_tbl[FDS_READ_WRITE_LOG_ENTRIES];
        int next_trans_id;

	int get_trans_id(void);
        StorHvJournalEntry *get_journal_entry(int trans_id) {
             if (trans_id < FDS_READ_WRITE_LOG_ENTRIES) {
                 return &rwlog_tbl[trans_id];
             }
        return NULL;
        }
};


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
  ~StorHvCtrl();	
  
  // Data Members
  Ice::CommunicatorPtr _communicator;
  StorHvJournal   	*journalTbl; 
  StorHvDataPlacement     *dataPlacementTbl;
  FDS_RPC_EndPointTbl        *rpcSwitchTbl; // RPC calls Switch Table
  VolumeCatalogCache         *volCatalogCache;
  
  void  InitIceObjects();
  void InitDmMsgHdr(const FDSP_MsgHdrTypePtr &msg_hdr);
  void InitSmMsgHdr(const FDSP_MsgHdrTypePtr &msg_hdr);
  
  int fds_set_dmack_status( int ipAddr, int  trans_id);
  int fds_set_dm_commit_status( int ipAddr, int  trans_id);
  int fds_set_smack_status( int ipAddr, int  trans_id);
  void fbd_process_req_timeout(unsigned long arg);
  void fbd_complete_req(int trans_id, fbd_request_t *req, int status);
  
  int fds_process_get_obj_resp(const FDSP_MsgHdrTypePtr& rd_msg, const FDSP_GetObjTypePtr& get_obj_rsp );
  int fds_process_put_obj_resp(const FDSP_MsgHdrTypePtr& rx_msg,const  FDSP_PutObjTypePtr& put_obj_rsp );
  int fds_process_update_catalog_resp(const FDSP_MsgHdrTypePtr& rx_msg,const  FDSP_UpdateCatalogTypePtr& cat_obj_rsp );

private:
  sh_comm_modes mode;
};
#endif
