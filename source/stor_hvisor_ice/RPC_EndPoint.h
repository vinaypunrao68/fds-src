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
#include <FDSP.h>
#include "list.h"
#include <list>
#include "ubd.h"


using namespace FDS_ProtocolInterface;
using namespace std;

class FDS_RPC_EndPoint {
public:
        FDS_RPC_EndPoint();
	FDS_RPC_EndPoint(std::string ip_addr, int port, 
                         FDS_ProtocolInterface::FDSP_MgrIdType mgr_id, 
                         Ice::CommunicatorPtr &ic);
	FDS_RPC_EndPoint(int  ip_addr_int, int port, 
                         FDS_ProtocolInterface::FDSP_MgrIdType mgr_id, 
                         Ice::CommunicatorPtr &ic);
        string 	ipAddr2String(int ipaddr);
        int ipString2Addr(string ipaddr_str);

        ~FDS_RPC_EndPoint();
    	void     Shutdown_RPC_EndPoint();

	int 		node_index;
	short   	proto_type;
	std::string 	ip_addr_str;
        int 		ip_addr;
        Ice::ObjectAdapterPtr adapter;
	int 		port_num;
        FDS_ProtocolInterface::FDSP_MgrIdType mgrId;
	FDSP_DataPathReqPrx  fdspDPAPI;
        FDSP_DataPathRespPtr fdspDataPathResp;
};

class FDS_RPC_EndPointTbl {
public :
    FDS_RPC_EndPointTbl();
    FDS_RPC_EndPointTbl(Ice::CommunicatorPtr& ic):_communicator(ic) { };
    ~FDS_RPC_EndPointTbl();

    Ice::CommunicatorPtr& _communicator;
    list<FDS_RPC_EndPoint *>    rpcEndPointList;

    void 	      Add_RPC_EndPoint(int  ipaddr, int& port, FDSP_MgrIdType mgr_id);
    void 	      Add_RPC_EndPoint(string  ipaddr, int& port, FDSP_MgrIdType mgr_id);
    void 	      Delete_RPC_EndPoint(int  ip_addr, FDSP_MgrIdType);
    void 	      Delete_RPC_EndPoint(string  ip_addr, FDSP_MgrIdType);
    int               Get_RPC_EndPoint(int ip_addr, FDSP_MgrIdType mgrId, FDS_RPC_EndPoint* endpoint);
    int               Get_RPC_EndPoint(string ip_addr, FDSP_MgrIdType mgrId, FDS_RPC_EndPoint* endpoint);
};

#endif
