/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 * Umbrella classes for the data manager component.
 */

#ifndef SOURCE_DATA_MGR_DATAMGR_H_
#define SOURCE_DATA_MGR_DATAMGR_H_

#include <Ice/Ice.h>

#include <unordered_map>

#include "fdsp/FDSP.h"
#include "include/fds_types.h"
#include "include/fds_err.h"

#include "util/Log.h"
#include "data_mgr/VolumeMeta.h"
#include "util/concurrency/ThreadPool.h"
#include "util/concurrency/Mutex.h"


namespace fds {

#define DM_TP_THREADS 3

class DataMgr : virtual public Ice::Application {
private:
  typedef FDS_ProtocolInterface::FDSP_DataPathReqPtr  ReqHandlerPtr;
  typedef FDS_ProtocolInterface::FDSP_DataPathRespPrx RespHandlerPrx;

  ReqHandlerPtr  reqHandleSrv;
  RespHandlerPrx respHandleCli;

  fds_log *dm_log;

  /*
   * Cmdline configurables
   */
  fds_uint32_t port_num;
  std::string stor_prefix;

  /*
   * Internal threadpool.
   */
  fds_uint32_t num_threads;
  fds_threadpool *_tp;

  /*
   * TODO: Move to STD shared or unique pointers. That's
   * safer.
   */
  std::unordered_map<fds_uint64_t, VolumeMeta*> vol_meta_map;
  fds_mutex *vol_map_mtx; // To protect access to vol_meta_map.

  // static void _open_entry();

  Error _process_open(fds_uint32_t vol_uuid,
                      fds_uint32_t vol_offset,
                      fds_uint32_t trans_id,
                      const ObjectID& oid);
  Error _process_commit(fds_uint32_t vol_uuid,
                        fds_uint32_t vol_offset,
                        fds_uint32_t trans_id,
                        const ObjectID& oid);
  Error _process_abort();

  Error _process_query(fds_uint32_t vol_uuid,
                       fds_uint32_t vol_offset,
                       ObjectID *oid);
  
public:
  DataMgr();
  ~DataMgr();

  virtual int run(int argc, char* argv[]);
  void interruptCallback(int);
  void swapMgrId(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg);
  fds_log* GetLog();
  
  /*
   * Nested class that manages the server interface.
   */
  class ReqHandler : public FDS_ProtocolInterface::FDSP_DataPathReq {
 private:
    Ice::CommunicatorPtr _communicator;

 public:
    ReqHandler(const Ice::CommunicatorPtr& communicator);
    ~ReqHandler();
    
    void PutObject(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &msg_hdr,
                   const FDS_ProtocolInterface::FDSP_PutObjTypePtr &put_obj,
                   const Ice::Current&);

    void GetObject(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &msg_hdr,
                   const FDS_ProtocolInterface::FDSP_GetObjTypePtr& get_obj,
                   const Ice::Current&);

    void UpdateCatalogObject(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &msg_hdr,
                             const FDS_ProtocolInterface::FDSP_UpdateCatalogTypePtr& update_catalog,
                             const Ice::Current&);

    void QueryCatalogObject(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &msg_hdr,
                            const FDS_ProtocolInterface::FDSP_QueryCatalogTypePtr& query_catalog,
                            const Ice::Current&);

    void OffsetWriteObject(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                           const FDS_ProtocolInterface::FDSP_OffsetWriteObjTypePtr& offset_write_obj,
                           const Ice::Current&);

    void RedirReadObject(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &msg_hdr,
                         const FDS_ProtocolInterface::FDSP_RedirReadObjTypePtr& redir_read_obj,
                         const Ice::Current&);

    void AssociateRespCallback(const Ice::Identity& ident,
                               const Ice::Current& current);
  };
  
  /*
   * Nested class that manages the client interface.
   */
  class RespHandler : public FDS_ProtocolInterface::FDSP_DataPathResp {
 public:
    RespHandler();
    ~RespHandler();
    
    void PutObjectResp(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &msg_hdr,
                       const FDS_ProtocolInterface::FDSP_PutObjTypePtr &put_obj,
                       const Ice::Current&);
    
    void GetObjectResp(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &msg_hdr,
                       const FDS_ProtocolInterface::FDSP_GetObjTypePtr& get_obj,
                       const Ice::Current&);
    
    void UpdateCatalogObjectResp(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &msg_hdr,
                                 const FDS_ProtocolInterface::FDSP_UpdateCatalogTypePtr& update_catalog,
                                 const Ice::Current&);

    void QueryCatalogObjectResp(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &msg_hdr,
                                const FDS_ProtocolInterface::FDSP_QueryCatalogTypePtr& query_catalog,
                                const Ice::Current&);
    
    void OffsetWriteObjectResp(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                               const FDS_ProtocolInterface::FDSP_OffsetWriteObjTypePtr& offset_write_obj,
                               const Ice::Current&);
    
    void RedirReadObjectResp(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &msg_hdr,
                             const FDS_ProtocolInterface::FDSP_RedirReadObjTypePtr& redir_read_obj,
                             const Ice::Current&);
  };
};

}  // namespace fds

#endif // SOURCE_DATA_MGR_DATAMGR_H_
