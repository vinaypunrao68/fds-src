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
#include <list>
#include <StorHvisorNet.h>
#include <RPC_EndPoint.h>
#include <arpa/inet.h>

FDS_RPC_EndPoint::FDS_RPC_EndPoint()
    : node_index(0), proto_type(0), port_num(0) {
}

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
  /*
   * TODO: Set Node_index to something. Is 0 correct?
   */
  node_index = 0;
  
  if (remote_mgr_id == FDSP_STOR_MGR) { 
    tcpProxyStr << "ObjectStorMgrSvr: tcp -h " << ip_addr_str << " -p  " << port;
    fdspDPAPI = FDSP_DataPathReqPrx::checkedCast(ic->stringToProxy (tcpProxyStr.str()));
  } else { 
    //tcpProxyStr << "DataMgrSvr: tcp -h " << ip_addr_str << " -p " << port;
    tcpProxyStr << "DataMgr: tcp -h " << ip_addr_str << " -p " << port;
    fdspDPAPI = FDSP_DataPathReqPrx::checkedCast(ic->stringToProxy (tcpProxyStr.str()));
  }
  
  _adapter = ic->createObjectAdapter("");
  if (!_adapter)
    throw "Invalid adapter";
  
  ident.name = IceUtil::generateUUID();
  ident.category = "";
  fdspDataPathResp  = new FDSP_DataPathRespCbackI;
  
  if (!fdspDataPathResp)
    throw "Invalid fdspDataPathRespCback";
  _adapter->add(fdspDataPathResp, ident);
  
  _adapter->activate();
  fdspDPAPI->ice_getConnection()->setAdapter(_adapter);
  fdspDPAPI->AssociateRespCallback(ident);
}

/*
 * TODO: Clean up all of these constructors to use a single base
 * constructor.
 */
FDS_RPC_EndPoint::FDS_RPC_EndPoint(const std::string& ip_addr_str_arg,
                                   int port, 
                                   FDS_ProtocolInterface::FDSP_MgrIdType remote_mgr_id, 
                                   Ice::CommunicatorPtr& ic) {
  std::ostringstream tcpProxyStr;
  Ice::Identity ident;
  this->port_num = port;
  ip_addr_str = ip_addr_str_arg;
  ip_addr = ipString2Addr(ip_addr_str_arg);
  /*
   * TODO: This is a hack since we can't
   * convert the string 'localhost' to
   * an IP int in the function above.
   */
  if (ip_addr_str == "localhost") {
    ip_addr = 0x7f000001;
  }
  mgrId = remote_mgr_id;
  /*
   * TODO: Set Node_index to something. Is 0 correct?
   */
  node_index = 0;
  
  if (remote_mgr_id == FDSP_STOR_MGR) { 
    tcpProxyStr << "ObjectStorMgrSvr: tcp -h " << ip_addr_str << " -p  " << port;
    fdspDPAPI = FDSP_DataPathReqPrx::checkedCast(ic->stringToProxy (tcpProxyStr.str()));
  } else { 
    tcpProxyStr << "DataMgr: tcp -h " << ip_addr_str << " -p " << port;
    fdspDPAPI = FDSP_DataPathReqPrx::checkedCast(ic->stringToProxy (tcpProxyStr.str()));
  }
  
  _adapter = ic->createObjectAdapter("");
  if (!_adapter)
    throw "Invalid adapter";
  
  ident.name = IceUtil::generateUUID();
  ident.category = "";
  fdspDataPathResp  = new FDSP_DataPathRespCbackI;
  
  if (!fdspDataPathResp)
    throw "Invalid fdspDataPathRespCback";
  _adapter->add(fdspDataPathResp, ident);
  
  _adapter->activate();
  fdspDPAPI->ice_getConnection()->setAdapter(_adapter);
  fdspDPAPI->AssociateRespCallback(ident);
}

FDS_RPC_EndPoint::~FDS_RPC_EndPoint()
{
    this->Shutdown_RPC_EndPoint();
}

/*
 * TODO: These should be static members. They
 * don't reference 'this'.
 */
string FDS_RPC_EndPoint::ipAddr2String(int ipaddr) {
struct sockaddr_in sa;
char buf[32];
  sa.sin_addr.s_addr = htonl(ipaddr);
  inet_ntop(AF_INET, (void *)&(sa.sin_addr), buf, INET_ADDRSTRLEN);
string ipaddr_str(buf);
  return (ipaddr_str);
}

fds_int32_t FDS_RPC_EndPoint::ipString2Addr(string ipaddr_str) {
  struct sockaddr_in sa;
  sa.sin_addr.s_addr = 0;
  inet_pton(AF_INET, (char *)ipaddr_str.data(), (void *)&(sa.sin_addr));
  return (ntohl(sa.sin_addr.s_addr));
}

void   FDS_RPC_EndPointTbl::Add_RPC_EndPoint(int  ipaddr, int port, FDSP_MgrIdType remote_mgr_id) 
{
    FDS_RPC_EndPoint *endPoint = new FDS_RPC_EndPoint(ipaddr, port, remote_mgr_id, _communicator);
    rpcTblMutex->lock();
    rpcEndPointList.push_back(endPoint);
    rpcTblMutex->unlock();
}

void   FDS_RPC_EndPointTbl::Add_RPC_EndPoint(std::string ipaddr_str, int port, FDSP_MgrIdType remote_mgr_id) 
{
    FDS_RPC_EndPoint *endPoint = new FDS_RPC_EndPoint(ipaddr_str, port, remote_mgr_id, _communicator);
    rpcTblMutex->lock();
    rpcEndPointList.push_back(endPoint);
    rpcTblMutex->unlock();
}

int FDS_RPC_EndPointTbl::Get_RPC_EndPoint(int  ip_addr, FDSP_MgrIdType mgr_id, FDS_RPC_EndPoint *endPoint) 
{
  rpcTblMutex->lock();
  for (std::list<FDS_RPC_EndPoint *>::iterator it=rpcEndPointList.begin(); it != rpcEndPointList.end(); ++it) {
    if ((*it)->ip_addr == ip_addr && (*it)->mgrId  == mgr_id) { 
      *endPoint = *(*it); 
      rpcTblMutex->unlock();
      return 0;
    }
  }
  rpcTblMutex->unlock();
  return -1;
}

int FDS_RPC_EndPointTbl::Get_RPC_EndPoint(int ip_addr,
                                          FDSP_MgrIdType mgr_id,
                                          FDS_RPC_EndPoint **endPoint) 
{
  rpcTblMutex->lock();
  for (std::list<FDS_RPC_EndPoint *>::iterator it=rpcEndPointList.begin(); it != rpcEndPointList.end(); ++it) {
    if ((*it)->ip_addr == ip_addr && (*it)->mgrId  == mgr_id) { 
      *endPoint = (*it); 
      rpcTblMutex->unlock();
      return 0;
    }
  }
  rpcTblMutex->unlock();
  return -1;
}

int FDS_RPC_EndPointTbl::Get_RPC_EndPoint(int ip_addr,
                                          fds_uint32_t port_num,
                                          FDSP_MgrIdType mgr_id,
                                          FDS_RPC_EndPoint **endPoint) 
{
  rpcTblMutex->lock();
  for (std::list<FDS_RPC_EndPoint *>::iterator it=rpcEndPointList.begin(); it != rpcEndPointList.end(); ++it) {
    if (((*it)->ip_addr == ip_addr) &&
        ((*it)->mgrId  == mgr_id) &&
        ((*it)->port_num == port_num)) { 
      *endPoint = (*it); 
      rpcTblMutex->unlock();
      return 0;
    }
  }
  rpcTblMutex->unlock();
  return -1;
}

int FDS_RPC_EndPointTbl::Get_RPC_EndPoint(std::string ip_addr_str,
                                          FDSP_MgrIdType mgr_id,
                                          FDS_RPC_EndPoint **endPoint) 
{
  rpcTblMutex->lock();
  for (std::list<FDS_RPC_EndPoint *>::iterator it=rpcEndPointList.begin(); it != rpcEndPointList.end(); ++it) {
    if ((*it)->ip_addr_str == ip_addr_str && (*it)->mgrId  == mgr_id) { 
      *endPoint = (*it); 
       rpcTblMutex->unlock();
      return 0;
    }
  }
  rpcTblMutex->unlock();
  return -1;
}

int FDS_RPC_EndPointTbl::Get_RPC_EndPoint(string  ip_addr_str, FDSP_MgrIdType mgr_id, FDS_RPC_EndPoint* endPoint) 
{
    rpcTblMutex->lock();
    int retval = 0;
    for (std::list<FDS_RPC_EndPoint *>::iterator it=rpcEndPointList.begin(); it != rpcEndPointList.end(); ++it) {
       if ((*it)->ip_addr_str == ip_addr_str && (*it)->mgrId  == mgr_id) { 
           endPoint = *it;
   	   rpcTblMutex->unlock();
           return retval;;
       }
   }
   rpcTblMutex->unlock();
   retval = -1;
   return retval;
}

void   FDS_RPC_EndPoint::Shutdown_RPC_EndPoint() 
{
    if (_adapter) 
        _adapter->deactivate();

}

void   FDS_RPC_EndPointTbl::Delete_RPC_EndPoint(int  ip_addr, FDSP_MgrIdType mgr_id) 
{
    rpcTblMutex->lock();
    for (std::list<FDS_RPC_EndPoint *>::iterator it=rpcEndPointList.begin(); it != rpcEndPointList.end(); ++it) {

       if ((*it)->ip_addr == ip_addr && (*it)->mgrId  == mgr_id) { 
          (*it)->Shutdown_RPC_EndPoint();
           rpcEndPointList.erase(it); 
           break;
       }
    }
    rpcTblMutex->unlock();
}

void   FDS_RPC_EndPointTbl::Delete_RPC_EndPoint(string  ip_addr, FDSP_MgrIdType mgr_id)  {
}
