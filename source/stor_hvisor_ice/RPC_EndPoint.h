#ifndef __RPC_EndPoint_h__
#define __RPC_EndPoint_h__
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
#include "fds_client/include/ubd.h"
#include <Mutex.h>
#include "include/fds_types.h"


using namespace FDS_ProtocolInterface;
using namespace std;
using namespace fds;

class FDS_RPC_EndPoint {
public:
        FDS_RPC_EndPoint();
	FDS_RPC_EndPoint(const std::string& ip_addr_str_arg, int port, 
                         FDS_ProtocolInterface::FDSP_MgrIdType mgr_id,
                         Ice::CommunicatorPtr &ic);
	FDS_RPC_EndPoint(int  ip_addr_int, int port,
                         FDS_ProtocolInterface::FDSP_MgrIdType mgr_id, 
                         Ice::CommunicatorPtr &ic);
        static string ipAddr2String(int ipaddr);
        static int ipString2Addr(string ipaddr_str);

        ~FDS_RPC_EndPoint();
    	void     Shutdown_RPC_EndPoint();

	int 		node_index;
	short   	proto_type;
	std::string 	ip_addr_str;
        int 		ip_addr;
        Ice::ObjectAdapterPtr _adapter;
	fds_uint32_t port_num;
        FDS_ProtocolInterface::FDSP_MgrIdType mgrId;
	FDSP_DataPathReqPrx  fdspDPAPI;
        FDSP_DataPathRespPtr fdspDataPathResp;

        FDS_RPC_EndPoint& operator=(const FDS_RPC_EndPoint& rhs) {
          if (this != &rhs) {
            node_index = rhs.node_index;
            proto_type = rhs.proto_type;
            ip_addr_str = rhs.ip_addr_str;
            ip_addr    = rhs.ip_addr;
            mgrId      = rhs.mgrId;
            fdspDPAPI  = rhs.fdspDPAPI;
            fdspDataPathResp = rhs.fdspDataPathResp;
          }
          return *this;
        }
};

inline std::ostream& operator<<(std::ostream& out, const FDS_RPC_EndPoint& ep) {
  out << "Network endpoint ";
  if (ep.mgrId == FDSP_DATA_MGR) {
    out << "DM";
  } else if (ep.mgrId == FDSP_STOR_MGR) {
    out << "SM";
  } else if (ep.mgrId == FDSP_ORCH_MGR) {
    out << "OM";
  } else {
    assert(ep.mgrId == FDSP_STOR_HVISOR);
    out << "SH";
  }
  out << " with IP " << ep.ip_addr
      << " and port " << ep.port_num;
  return out;
}

class FDS_RPC_EndPointTbl {
public :
    FDS_RPC_EndPointTbl();
    FDS_RPC_EndPointTbl(Ice::CommunicatorPtr& ic):_communicator(ic) {
        rpcTblMutex = new fds_mutex("RPC Tbl mutex"); 
     };
    ~FDS_RPC_EndPointTbl();

    Ice::CommunicatorPtr& _communicator;
    list<FDS_RPC_EndPoint *>    rpcEndPointList;
    fds_mutex   *rpcTblMutex;

    void 	      Add_RPC_EndPoint(int  ipaddr, int port, FDSP_MgrIdType mgr_id);
    void 	      Add_RPC_EndPoint(string  ipaddr, int port, FDSP_MgrIdType mgr_id);
    void 	      Delete_RPC_EndPoint(int  ip_addr, FDSP_MgrIdType);
    void 	      Delete_RPC_EndPoint(string  ip_addr, FDSP_MgrIdType);
    int               Get_RPC_EndPoint(int ip_addr, FDSP_MgrIdType mgrId, FDS_RPC_EndPoint* endpoint);
    int               Get_RPC_EndPoint(string ip_addr, FDSP_MgrIdType mgrId, FDS_RPC_EndPoint* endpoint);
    int 	      Get_RPC_EndPoint(string ip_addr_str,
                         	      FDSP_MgrIdType mgr_id,
                         	      FDS_RPC_EndPoint **endPoint);
    int 	      Get_RPC_EndPoint(int ip_addr,
                         	      FDSP_MgrIdType mgrId,
                                       FDS_RPC_EndPoint** endpoint);
    int Get_RPC_EndPoint(int ip_addr,
                         fds_uint32_t port_num,
                         FDSP_MgrIdType mgr_id,
                         FDS_RPC_EndPoint **endPoint) ;
};

#endif
