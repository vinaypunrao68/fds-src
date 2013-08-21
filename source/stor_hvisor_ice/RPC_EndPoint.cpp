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
#include <StorHvisorNet.h>
#include <RPC_EndPoint.h>
#include <arpa/inet.h>


FDS_RPC_EndPoint::FDS_RPC_EndPoint(int ip_addr, int port, 
                                   FDS_ProtocolInterface::FDSP_MgrIdType remote_mgr_id, 
                                   Ice::CommunicatorPtr& ic)
{
Ice::Identity ident;
std::ostringstream tcpProxyStr;
            this->ip_addr = ip_addr;
            this->port_num = port;
    	    ip_addr_str = ipAddr2String(ip_addr);
            mgrId = remote_mgr_id;

            if (remote_mgr_id == FDSP_STOR_MGR) { 
               tcpProxyStr << "ObjectStorMgrSvr: tcp -h " << ip_addr_str << " -p  " << port;
	       fdspDPAPI = FDSP_DataPathReqPrx::checkedCast(ic->stringToProxy (tcpProxyStr.str()));
            } else { 
                tcpProxyStr << "DataMgrSvr: tcp -h " << ip_addr_str << " -p " << port;
	        fdspDPAPI = FDSP_DataPathReqPrx::checkedCast(ic->stringToProxy (tcpProxyStr.str()));
            }

            adapter = ic->createObjectAdapter("");
            if (!adapter)
                throw "Invalid adapter";
    
            ident.name = IceUtil::generateUUID();
            ident.category = "";
            fdspDataPathResp  = new FDSP_DataPathRespCbackI;
    
            if (!fdspDataPathResp)
                throw "Invalid fdspDataPathRespCback";
            adapter->add(fdspDataPathResp, ident);
      
            adapter->activate();
            fdspDPAPI->ice_getConnection()->setAdapter(adapter);
            fdspDPAPI->AssociateRespCallback(ident);
}

FDS_RPC_EndPoint::FDS_RPC_EndPoint(string ip_addr_str, int port, 
                                   FDS_ProtocolInterface::FDSP_MgrIdType remote_mgr_id, 
                                   Ice::CommunicatorPtr& ic) {
std::ostringstream tcpProxyStr;
Ice::Identity ident;
            this->port_num = port;
	    ip_addr = ipString2Addr(ip_addr_str);
    	    ip_addr_str = ip_addr_str;
            mgrId = remote_mgr_id;

            if (remote_mgr_id == FDSP_STOR_MGR) { 
               tcpProxyStr << "ObjectStorMgrSvr: tcp -h " << ip_addr_str << " -p  " << port;
	       fdspDPAPI = FDSP_DataPathReqPrx::checkedCast(ic->stringToProxy (tcpProxyStr.str()));
            } else { 
                tcpProxyStr << "DataMgrSvr: tcp -h " << ip_addr_str << " -p " << port;
	        fdspDPAPI = FDSP_DataPathReqPrx::checkedCast(ic->stringToProxy (tcpProxyStr.str()));
            }

            adapter = ic->createObjectAdapter("");
            if (!adapter)
                throw "Invalid adapter";
    
            ident.name = IceUtil::generateUUID();
            ident.category = "";
            fdspDataPathResp  = new FDSP_DataPathRespCbackI;
    
            if (!fdspDataPathResp)
                throw "Invalid fdspDataPathRespCback";
            adapter->add(fdspDataPathResp, ident);
      
            adapter->activate();
            fdspDPAPI->ice_getConnection()->setAdapter(adapter);
            fdspDPAPI->AssociateRespCallback(ident);
}

FDS_RPC_EndPoint::~FDS_RPC_EndPoint()
{
    this->Shutdown_RPC_EndPoint();
}

string FDS_RPC_EndPoint::ipAddr2String(int ipaddr) {
struct sockaddr_in sa;
char buf[32];
  sa.sin_addr.s_addr = htonl(ipaddr);
  inet_ntop(AF_INET, (void *)&(sa.sin_addr), buf, INET_ADDRSTRLEN);
string ipaddr_str(buf);
  return (ipaddr_str);
}

int FDS_RPC_EndPoint::ipString2Addr(string ipaddr_str) {
struct sockaddr_in sa;
  sa.sin_addr.s_addr = 0;
  inet_pton(AF_INET, (char *)ipaddr_str.data(), (void *)&(sa.sin_addr));
  return (ntohl(sa.sin_addr.s_addr));
}

void   FDS_RPC_EndPointTbl::Add_RPC_EndPoint(int  ipaddr, int& port, FDSP_MgrIdType remote_mgr_id) 
{
    FDS_RPC_EndPoint *endPoint = new FDS_RPC_EndPoint(ipaddr, port, remote_mgr_id, _communicator);
    rpcEndPointList.push_back(endPoint);
}

void   FDS_RPC_EndPointTbl::Add_RPC_EndPoint(string  ipaddr_str, int& port, FDSP_MgrIdType remote_mgr_id) 
{
    FDS_RPC_EndPoint *endPoint = new FDS_RPC_EndPoint(ipaddr_str, port, remote_mgr_id, _communicator);
    rpcEndPointList.push_back(endPoint);
}

int FDS_RPC_EndPointTbl::Get_RPC_EndPoint(int  ip_addr, FDSP_MgrIdType mgr_id, FDS_RPC_EndPoint* endPoint) 
{
    for (std::list<FDS_RPC_EndPoint *>::iterator it=rpcEndPointList.begin(); it != rpcEndPointList.end(); ++it) {
       if ((*it)->ip_addr == ip_addr && (*it)->mgrId  == mgr_id) { 
           endPoint = *it; 
           return 0;
       }
   }
   return -1;
}

int FDS_RPC_EndPointTbl::Get_RPC_EndPoint(string  ip_addr_str, FDSP_MgrIdType mgr_id, FDS_RPC_EndPoint* endPoint) 
{
    for (std::list<FDS_RPC_EndPoint *>::iterator it=rpcEndPointList.begin(); it != rpcEndPointList.end(); ++it) {
       if ((*it)->ip_addr_str == ip_addr_str && (*it)->mgrId  == mgr_id) { 
           endPoint = *it; 
           return 0;
       }
   }
   return -1;
}

void   FDS_RPC_EndPoint::Shutdown_RPC_EndPoint() 
{
    adapter->deactivate();

}

void   FDS_RPC_EndPointTbl::Delete_RPC_EndPoint(int  ip_addr, FDSP_MgrIdType mgr_id) 
{
    for (std::list<FDS_RPC_EndPoint *>::iterator it=rpcEndPointList.begin(); it != rpcEndPointList.end(); ++it) {

       if ((*it)->ip_addr == ip_addr && (*it)->mgrId  == mgr_id) { 
          (*it)->Shutdown_RPC_EndPoint();
           rpcEndPointList.erase(it); 
           return;
       }
    }
}

void   FDS_RPC_EndPointTbl::Delete_RPC_EndPoint(string  ip_addr, FDSP_MgrIdType mgr_id)  {
}
