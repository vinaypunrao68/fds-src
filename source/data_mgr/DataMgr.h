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
  FDS_ProtocolInterface::FDSP_DataPathRespPrx dataMgrCli;
  
public:
  virtual int run(int argc, char* argv[]);
  void swapMgrId(const FDSP_MsgHdrTypePtr& fdsp_msg);

  friend class DataMgrI;
};

class DataMgrI : public FDS_ProtocolInterface::FDSP_DataPathReq {
public:
  DataMgrI(const Ice::CommunicatorPtr& communicator);
  DataMgrI();
  ~DataMgrI();
  Ice::CommunicatorPtr _communicator;

  void PutObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_PutObjTypePtr &put_obj, const Ice::Current&);
  
  void GetObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_GetObjTypePtr& get_obj, const Ice::Current&);

  void UpdateCatalogObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_UpdateCatalogTypePtr& update_catalog , const Ice::Current&);

  void OffsetWriteObject(const FDSP_MsgHdrTypePtr& msg_hdr, const FDSP_OffsetWriteObjTypePtr& offset_write_obj, const Ice::Current&);

  void RedirReadObject(const FDSP_MsgHdrTypePtr &msg_hdr, const FDSP_RedirReadObjTypePtr& redir_read_obj, const Ice::Current&);
  void AssociateRespCallback(const Ice::Identity&, const Ice::Current&);
};

// }  // namespace fds

#endif
