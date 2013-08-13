#ifndef __StorHvisorClient_h__
#define __StorHvisorClient_h__
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
#include <FDSP.h>
#include "list.h"

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

#if 0
class FDSP_DataPathRespCbackI : public FDSP_DataPathResp
{
public:
    void GetObjectResp(const FDSP_MsgHdrTypePtr&, const FDSP_GetObjTypePtr&, const Ice::Current&) {
    }

    void PutObjectResp(const FDSP_MsgHdrTypePtr&, const FDSP_PutObjTypePtr&, const Ice::Current&) {
    }

    void UpdateCatalogObjectResp(const FDSP_MsgHdrTypePtr& fdsp_msg, const FDSP_UpdateCatalogTypePtr& cat_obj_req, const Ice::Current &) {
    }

    void OffsetWriteObjectResp(const FDSP_MsgHdrTypePtr& fdsp_msg, const FDSP_OffsetWriteObjTypePtr& offset_write_obj_req, const Ice::Current &) {

    }
    void RedirReadObjectResp(const FDSP_MsgHdrTypePtr& fdsp_msg, const FDSP_RedirReadObjTypePtr& redir_write_obj_req, const Ice::Current &)
    { 
    }
};
#endif

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
//class StorHvisorClient : public Ice::Application
class StorHvisorClient
{
public:
    StorHvisorClient();
    int run(int, char*[]);

private:
    void exception(const Ice::Exception&);
};


class SHvisorClientCallback : public IceUtil::Shared
{
public:

    void response()
    {
    }

    void exception(const Ice::Exception& ex)
    {
        std::cerr << " AMI call to ObjStorMgr failed:\n" << ex << std::endl;
    }
};

typedef IceUtil::Handle<SHvisorClientCallback> shClientCallbackPtr;
class FDSP_NetworkRec {
public:
	int 	node_index;
	short   proto_type;
	long 	ip_addr;
	long 	port_num;
};

class FDSP_DmNode {
public:
        int   node_ipaddr;   /* ip address of the node */
        short  node_state;    /* state of the node online/offline */
        short  num_nodes;     /* number of DM nodes in the cluster */
        short  node_type;     /* primary, secondary .... */
        short  node_id;       /* node identifier , this might be overloaded with IP address */
        struct   list_head list;

};

/* storage manager  structure */

class FDSP_SmNode {
public:
        int    node_ipaddr;   /* ip address of the node */
        short  stor_type;     /* type  of the storage in the node */
        short  node_state;    /* state of the node online/offline */
        short  num_nodes;     /* number of DM nodes in the cluster */
        short  node_type;     /* primary, secondary .... */
        short  node_id;       /* node identifier , this might be overloaded with IP address */
        struct   list_head list;
};



class FDSP_IpNode {
public:
        long  ipAddr;
        short  ack_status;
        short  commit_status;
};

class   FDSP_fdsJourn {
public:
        short  replc_cnt;
        short  sm_ack_cnt;
        short  dm_ack_cnt;
        short  dm_commit_cnt;
        short  trans_state;
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

class FDSP_procJourn {
public:
	int fds_init_trans_log(void);
	int get_trans_id(void);


};


class FDSP_procTbl {
public:
	int  fds_init_dmt();
	int fds_init_dlt();
	int  add_dlt_entry(FDSP_SmNode *newdlt, int doid_key);
	int add_dmt_entry(FDSP_DmNode  *newdmt, volid_t  vol_id);
	int del_dmt_entry(int ipaddr, volid_t  vol_id);
	int del_dlt_entry(int ipaddr, int doid_key);
	FDSP_SmNode *get_sm_nodes_for_doid_key(int doid_key);
	FDSP_DmNode *get_dm_nodes_for_volume(volid_t vol_id);
	int populate_dmt_dlt_tbl();

};

class FDSP_Proc_Io:public FDS_ProtocolInterface::FDSP_MsgHdrType {
public:
	
	void InitDmMsgHdr(const FDSP_MsgHdrTypePtr &msg_hdr);
	void InitSmMsgHdr(const FDSP_MsgHdrTypePtr &msg_hdr);
	FDSP_fdsJourn	rwlog_tbl[FDS_READ_WRITE_LOG_ENTRIES];
	FDSP_DmNode     dmt_tbl[FDS_MAC_DM_ENTRIES];
	FDSP_SmNode     dlt_tbl[FDS_MAC_SM_ENTRIES];
	FDSP_procJourn  procJ; 
	FDSP_procTbl    procT;

};

class FDSP_ProcRx:public FDS_ProtocolInterface::FDSP_MsgHdrType {
public:
	int fds_set_dmack_status( int ipAddr, int  trans_id);
	int fds_set_dm_commit_status( int ipAddr, int  trans_id);
	int fds_set_smack_status( int ipAddr, int  trans_id);
	void fbd_process_req_timeout(unsigned long arg);
	void fbd_complete_req(int trans_id, fbd_request_t *req, int status);
	int fds_process_get_obj_resp(FDSP_MsgHdrType *rd_msg, FDSP_GetObjType *get_obj_rsp );
	int fds_process_put_obj_resp(FDSP_MsgHdrTypePtr rx_msg, FDSP_PutObjTypePtr put_obj_rsp );
	int fds_process_update_catalog_resp(FDSP_MsgHdrTypePtr rx_msg, FDSP_UpdateCatalogTypePtr cat_obj_rsp );
};


class FDSP_NetworkCon {
public:
	Ice::InitializationData  initData;
	FDSP_DataPathReqPrx  fdspDPAPI;
	FDSP_Proc_Io	     procIo;
	void  CreateNetworkEndPoint( FDSP_NetworkRec *netRec );
	void  InitIceObejcts();
	
};

#endif
