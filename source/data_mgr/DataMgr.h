/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 * Umbrella classes for the data manager component.
 */

#ifndef SOURCE_DATA_MGR_DATA_MGR_H_
#define SOURCE_DATA_MGR_DATA_MGR_H_

#include <unordered_map>

#include "fdsp/FDSP.h"
#include <Ice/Ice.h>
#include "include/fds_types.h"
#include "include/fds_err.h"

#include "util/Log.h"
#include "data_mgr/VolumeMeta.h"

using namespace FDS_ProtocolInterface;
using namespace Ice;

namespace fds {

class DataMgr : virtual public Ice::Application {
private:
  typedef FDS_ProtocolInterface::FDSP_DataPathReqPtr  ReqHandlerPtr;
  typedef FDS_ProtocolInterface::FDSP_DataPathRespPrx RespHandlerPrx;

  ReqHandlerPtr  reqHandleSrv;
  RespHandlerPrx respHandleCli;

  fds_log *dm_log;

  std::unordered_map<fds_uint64_t, VolumeMeta> vol_meta_map;

  Error _process_open(fds_uint32_t vol_offset,
                      fds_uint32_t trans_id,
                      const ObjectID& oid);
  Error _process_close();
  
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
    
    void PutObject(const FDSP_MsgHdrTypePtr &msg_hdr,
                   const FDSP_PutObjTypePtr &put_obj,
                   const Ice::Current&);

    void GetObject(const FDSP_MsgHdrTypePtr &msg_hdr,
                   const FDSP_GetObjTypePtr& get_obj,
                   const Ice::Current&);

    void UpdateCatalogObject(const FDSP_MsgHdrTypePtr &msg_hdr,
                             const FDSP_UpdateCatalogTypePtr& update_catalog,
                             const Ice::Current&);

    void OffsetWriteObject(const FDSP_MsgHdrTypePtr& msg_hdr,
                           const FDSP_OffsetWriteObjTypePtr& offset_write_obj,
                           const Ice::Current&);

    void RedirReadObject(const FDSP_MsgHdrTypePtr &msg_hdr,
                         const FDSP_RedirReadObjTypePtr& redir_read_obj,
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
    
    virtual void PutObjectResp(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_PutObjTypePtr &put_obj, const Ice::Current&);
    
    virtual void GetObjectResp(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_GetObjTypePtr& get_obj, const Ice::Current&);
    
    virtual void UpdateCatalogObjectResp(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_UpdateCatalogTypePtr& update_catalog , const Ice::Current&);
    
    virtual void OffsetWriteObjectResp(const FDSP_MsgHdrTypePtr& msg_hdr, const FDSP_OffsetWriteObjTypePtr& offset_write_obj, const Ice::Current&);
    
    virtual void RedirReadObjectResp(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_RedirReadObjTypePtr& redir_read_obj, const Ice::Current&);
  };
};

}  // namespace fds

#endif // SOURCE_DATA_MGR_DATA_MGR_H
