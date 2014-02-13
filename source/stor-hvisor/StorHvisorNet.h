#ifndef __StorHvisorNet_h__
#define __StorHvisorNet_h__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <arpa/inet.h>


#include <list>
#include "StorHvDataPlace.h"
#include <fds_err.h>
#include <fds_types.h>

/* TODO: Use API header in include directory. */
#include "ubd.h"
#include <lib/OMgrClient.h>
#include "StorHvVolumes.h"
#include "StorHvisorCPP.h" 
#include "qos_ctrl.h" 
#include "fds_qos.h" 
#include "StorHvQosCtrl.h" 
#include <hash/md5.h>

#include <fdsp/FDSP_DataPathReq.h>
#include <fdsp/FDSP_DataPathResp.h>
#include <fdsp/FDSP_MetaDataPathReq.h>
#include <fdsp/FDSP_MetaDataPathResp.h>
#include <fdsp/FDSP_ControlPathReq.h>
#include <fdsp/FDSP_ControlPathResp.h>
#include <fdsp/FDSP_ConfigPathReq.h>


#include "NetSession.h"

#include <map>
// #include "util/concurrency/Thread.h"
#include <concurrency/Synchronization.h>


#undef  FDS_TEST_SH_NOOP              /* IO returns (filled with 0s for read) as soon as SH receives it from ubd */
#undef FDS_TEST_SH_NOOP_DISPATCH     /* IO returns (filled with 0s for read) as soon as dispatcher takes it from the queue */



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
using namespace fds;

class FDSP_DataPathRespCbackI : public FDS_ProtocolInterface::FDSP_DataPathRespIf
{
public:
    FDSP_DataPathRespCbackI() {
    }
    ~FDSP_DataPathRespCbackI() {
    }

   virtual void GetObjectResp(const FDSP_MsgHdrType& fdsp_msg, const FDSP_GetObjType& get_obj_req) {
   }
   virtual void GetObjectResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg, boost::shared_ptr<FDSP_GetObjType>& get_obj_req);
   virtual void PutObjectResp(const FDSP_MsgHdrType& fdsp_msg, const FDSP_PutObjType& put_obj_req) {
   }
   virtual void PutObjectResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg, boost::shared_ptr<FDSP_PutObjType>& put_obj_req);
   virtual void DeleteObjectResp(const FDSP_MsgHdrType& fdsp_msg, const FDSP_DeleteObjType& del_obj_req) {
   }
   virtual void DeleteObjectResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg, boost::shared_ptr<FDSP_DeleteObjType>& del_obj_req);
   virtual void OffsetWriteObjectResp(const FDSP_MsgHdrType& fdsp_msg, const FDSP_OffsetWriteObjType& offset_write_obj_req) {
   }
   virtual void OffsetWriteObjectResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg, boost::shared_ptr<FDSP_OffsetWriteObjType>& offset_write_obj_req) {
   }
   virtual void RedirReadObjectResp(const FDSP_MsgHdrType& fdsp_msg, const FDSP_RedirReadObjType& redir_write_obj_req) {
   }
   virtual void RedirReadObjectResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg, boost::shared_ptr<FDSP_RedirReadObjType>& redir_write_obj_req){
   }

};


class FDSP_MetaDataPathRespCbackI : public FDS_ProtocolInterface::FDSP_MetaDataPathRespIf
{
public:
    FDSP_MetaDataPathRespCbackI() {
    }

    ~FDSP_MetaDataPathRespCbackI() {
    }


    virtual void UpdateCatalogObjectResp(const FDSP_MsgHdrType& fdsp_msg, const FDSP_UpdateCatalogType& cat_obj_req) {
    }
    virtual void UpdateCatalogObjectResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg, boost::shared_ptr<FDSP_UpdateCatalogType>& cat_obj_req);
    virtual void QueryCatalogObjectResp(const FDSP_MsgHdrType& fdsp_msg, const FDSP_QueryCatalogType& cat_obj_req) {
    }
    virtual void QueryCatalogObjectResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg, boost::shared_ptr<FDSP_QueryCatalogType>& cat_obj_req);
    virtual void DeleteCatalogObjectResp(const FDSP_MsgHdrType& fdsp_msg, const FDSP_DeleteCatalogType& cat_obj_req) {
    }
    virtual void DeleteCatalogObjectResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg, boost::shared_ptr<FDSP_DeleteCatalogType>& cat_obj_req);
    virtual void GetVolumeBlobListResp(const FDSP_MsgHdrType& fds_msg, const FDSP_GetVolumeBlobListRespType& blob_list_rsp) {
    }
    virtual void GetVolumeBlobListResp(boost::shared_ptr<FDSP_MsgHdrType>& fds_msg, boost::shared_ptr<FDSP_GetVolumeBlobListRespType>& blob_list_rsp);
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

class StorHvCtrl
 {


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
  
  StorHvCtrl(int argc, char *argv[], SysParams *params);
  StorHvCtrl(int argc, char *argv[], SysParams *params,
      sh_comm_modes _mode);
  StorHvCtrl(int argc,
             char *argv[],
             SysParams *params,
             sh_comm_modes _mode,
             fds_uint32_t sm_port_num,
             fds_uint32_t dm_port_num,
	     std::string config_path);
  ~StorHvCtrl();	
  hv_create_blkdev cr_blkdev;
  hv_delete_blkdev del_blkdev;

  //imcremental checksum  for header and payload 
  checksum_calc   *chksumPtr;
  
  // Data Members
  StorHvDataPlacement        *dataPlacementTbl;
  boost::shared_ptr<netSessionTbl> rpcSessionTbl; // RPC calls Switch Table
  StorHvVolumeTable          *vol_table;  
  fds::StorHvQosCtrl             *qos_ctrl; // Qos Controller object
  OMgrClient                 *om_client;
  FDS_ProtocolInterface::FDSP_AnnounceDiskCapabilityPtr dInfo;

  std::string                 myIp;
  std::string                 my_node_name;

  fds::Error pushBlobReq(FdsBlobReq *blobReq);
  fds::Error putBlob(AmQosReq *qosReq);
  fds::Error getBlob(AmQosReq *qosReq);
  fds::Error deleteBlob(AmQosReq *qosReq);
  fds::Error listBucket(AmQosReq *qosReq);
  fds::Error getBucketStats(AmQosReq *qosReq);
  fds::Error putObjResp(const FDSP_MsgHdrTypePtr& rxMsg,
                        const FDSP_PutObjTypePtr& putObjRsp);
  fds::Error upCatResp(const FDSP_MsgHdrTypePtr& rxMsg, 
                       const FDSP_UpdateCatalogTypePtr& catObjRsp);
  fds::Error deleteCatResp(const FDSP_MsgHdrTypePtr& rxMsg,
                           const FDSP_DeleteCatalogTypePtr& delCatRsp);
  fds::Error getObjResp(const FDSP_MsgHdrTypePtr& rxMsg,
                        const FDSP_GetObjTypePtr& getObjRsp);
  fds::Error getBucketResp(const FDSP_MsgHdrTypePtr& rxMsg,
			   const FDSP_GetVolumeBlobListRespTypePtr& blobListResp);

  static void bucketStatsRespHandler(const FDSP_MsgHdrTypePtr& rx_msg,
				     const FDSP_BucketStatsRespTypePtr& buck_stats);
  void getBucketStatsResp(const FDSP_MsgHdrTypePtr& rx_msg,
			  const FDSP_BucketStatsRespTypePtr& buck_stats);

  void InitDmMsgHdr(const FDSP_MsgHdrTypePtr &msg_hdr);
  void InitSmMsgHdr(const FDSP_MsgHdrTypePtr &msg_hdr);
  
  void fbd_process_req_timeout(unsigned long arg);

  int fds_move_wr_req_state_machine(const FDSP_MsgHdrTypePtr& rx_msg);  
  int fds_process_get_obj_resp(const FDSP_MsgHdrTypePtr& rd_msg, const FDSP_GetObjTypePtr& get_obj_rsp );
  int fds_process_put_obj_resp(const FDSP_MsgHdrTypePtr& rx_msg,const  FDSP_PutObjTypePtr& put_obj_rsp );
  int fds_process_update_catalog_resp(const FDSP_MsgHdrTypePtr& rx_msg,const  FDSP_UpdateCatalogTypePtr& cat_obj_rsp );
  fds_log* GetLog();
  SysParams* getSysParams();
  void StartOmClient();
  sh_comm_modes GetRunTimeMode() { return mode; }
  boost::shared_ptr<FDSP_DataPathRespCbackI> dPathRespCback;
  boost::shared_ptr<FDSP_MetaDataPathRespCbackI> mPathRespCback;

private:
  void handleDltMismatch(StorHvVolume *vol,
                         StorHvJournalEntry *journEntry);
  void procNewDlt(fds_uint64_t newDltVer);

  Error dispatchSmPutMsg(StorHvJournalEntry *journEntry);
  Error dispatchSmGetMsg(StorHvJournalEntry *journEntry);
  Error dispatchSmDelMsg(StorHvJournalEntry *journEntry);

  fds_log *sh_log;
  SysParams *sysParams;
  sh_comm_modes mode;
};

extern StorHvCtrl *storHvisor;

/*
 * Static function for process IO via a threadpool
 */
static void processBlobReq(AmQosReq *qosReq) {
  fds_verify(qosReq->io_module == FDS_IOType::STOR_HV_IO);
  fds_verify(qosReq->magicInUse() == true);

  fds::Error err(ERR_OK);
  switch (qosReq->io_type) { 
    case fds::FDS_IO_READ :
    case fds::FDS_GET_BLOB :
      err = storHvisor->getBlob(qosReq);
      break;

    case fds::FDS_IO_WRITE :
    case fds::FDS_PUT_BLOB:
      err = storHvisor->putBlob(qosReq);
      break;

    case fds::FDS_DELETE_BLOB: 
      err = storHvisor->deleteBlob(qosReq);
      break;

  case fds::FDS_LIST_BUCKET:
    err = storHvisor->listBucket(qosReq);
    break;

  case fds::FDS_BUCKET_STATS:
    err = storHvisor->getBucketStats(qosReq);
    break;

    default :
      break;
  }

  fds_verify((err == ERR_OK) || (err == ERR_NOT_IMPLEMENTED));
}

#endif
