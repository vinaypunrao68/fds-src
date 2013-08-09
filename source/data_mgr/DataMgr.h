#ifndef __DATA_MGR_H__
#define __DATA_MGR_H__

#include "FDSP.h"
#include <Ice/Ice.h>
#include "include/fds_types.h"

#include "dm.h"

using namespace FDS_ProtocolInterface;
// using namespace fds;
// using namespace osm;
using namespace std;
using namespace Ice;

// namespace fds {

#define FDS_DATA_MGR_DGRAM_PORT FDS_CLUSTER_UDP_PORT_SM

class DataMgr : virtual public Ice::Application {
private:
  fds_uint32_t sockfd;
  FDS_ProtocolInterface::FDSP_DataPathReqPtr dataMgrSrv;
  FDS_ProtocolInterface::FDSP_DataPathRespPtr dataMgrCli;
  
public:
  virtual int run(int argc, char* argv[]);
  void swapMgrId(const FDSP_MsgHdrTypePtr& fdsp_msg);

  friend class DataMgrI;
};

class DataMgrI : public FDS_ProtocolInterface::FDSP_DataPathReq {
public:
  DataMgrI();
  ~DataMgrI();

  virtual void PutObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_PutObjTypePtr &put_obj, const Ice::Current&);
  
  virtual void GetObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_GetObjTypePtr& get_obj, const Ice::Current&);

  virtual void UpdateCatalogObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_UpdateCatalogTypePtr& update_catalog , const Ice::Current&);

  virtual void OffsetWriteObject(const FDSP_MsgHdrTypePrx& msg_hdr, const FDSP_OffsetWriteObjTypePrx& offset_write_obj, const Ice::Current&);

  virtual void RedirReadObject(const FDSP_MsgHdrTypePrx &msg_hdr, const FDSP_RedirReadObjTypePrx& redir_read_obj, const Ice::Current&);
};

class DataMgrClientI : public FDS_ProtocolInterface::FDSP_DataPathResp {
public:
  DataMgrClientI();
  ~DataMgrClientI();

  virtual void PutObjectResp(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_PutObjTypePtr &put_obj, const Ice::Current&);

  virtual void GetObjectResp(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_GetObjTypePtr& get_obj, const Ice::Current&);

  virtual void UpdateCatalogObjectResp(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_UpdateCatalogTypePtr& update_catalog , const Ice::Current&);

  virtual void OffsetWriteObjectResp(const FDSP_MsgHdrTypePtr& msg_hdr, const FDSP_OffsetWriteObjTypePtr& offset_write_obj, const Ice::Current&);

  virtual void RedirReadObjectResp(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_RedirReadObjTypePtr& redir_read_obj, const Ice::Current&);
};

// }  // namespace fds

#endif
