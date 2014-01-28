#include <ifaddrs.h>
#include <list>
#include <NetSession.h>
#include <NetSessRespClient.h>
#include <NetSessRespSvr.h>
#include <arpa/inet.h>
#include <fds_assert.h>
#include <regex>
#include <sstream>

netSession::netSession()
    : node_index(0), proto_type(0), port_num(0) {
}

netSession::netSession(const std::string& _node_name, int port, 
                       FDS_ProtocolInterface::FDSP_MgrIdType local_mgr_id,
                       FDS_ProtocolInterface::FDSP_MgrIdType remote_mgr_id)
{
  this->ip_addr = ip_addr;
  this->port_num = port;
  ip_addr_str = ipAddr2String(ip_addr);
  remoteMgrId = remote_mgr_id;  
  localMgrId = local_mgr_id;
  /*
   * TODO: Set Node_index to something. Is 0 correct?
   */
  node_index = 0;
}

netSessionTbl::~netSessionTbl() {
    endAllSessions();
    delete sessionTblMutex;
}

netSession::~netSession()
{
}

/*
 * TODO: These should be static members. They
 * don't reference 'this'.
 */
string netSession::ipAddr2String(int ipaddr) {
    struct sockaddr_in sa;
    char buf[32];
    sa.sin_addr.s_addr = htonl(ipaddr);
    inet_ntop(AF_INET, (void *)&(sa.sin_addr), buf, INET_ADDRSTRLEN);
    string ipaddr_str(buf);
    return (ipaddr_str);
}

string netSessionTbl::ipAddr2String(int ipaddr) {
    struct sockaddr_in sa;
    char buf[32];
    sa.sin_addr.s_addr = htonl(ipaddr);
    inet_ntop(AF_INET, (void *)&(sa.sin_addr), buf, INET_ADDRSTRLEN);
    string ipaddr_str(buf);
    return (ipaddr_str);
}

fds_int32_t netSession::ipString2Addr(string ipaddr_str) {
    struct sockaddr_in sa;
    sa.sin_addr.s_addr = 0;
    inet_pton(AF_INET, (char *)ipaddr_str.data(), (void *)&(sa.sin_addr));
    return (ntohl(sa.sin_addr.s_addr));
}

fds_int32_t netSessionTbl::ipString2Addr(string ipaddr_str) {
    struct sockaddr_in sa;
    sa.sin_addr.s_addr = 0;
    inet_pton(AF_INET, (char *)ipaddr_str.data(), (void *)&(sa.sin_addr));
    return (ntohl(sa.sin_addr.s_addr));
}



/**
 * @return local ip
 */
std::string netSession::getLocalIp()
{
    struct ifaddrs *ifAddrStruct = NULL;
    struct ifaddrs *ifa          = NULL;
    void   *tmpAddrPtr           = NULL;
    std::string myIp;

    /*
     * Get the local IP of the host.
     * This is needed by the OM.
     */
    getifaddrs(&ifAddrStruct);
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr->sa_family == AF_INET) { // IPv4
            if (strncmp(ifa->ifa_name, "lo", 2) != 0) {
                tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
                char addrBuf[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, tmpAddrPtr, addrBuf, INET_ADDRSTRLEN);
                myIp = std::string(addrBuf);
                if (myIp.find("10.1") != std::string::npos)
                    break; /* TODO: more dynamic */
            }
        }
    }

    if (ifAddrStruct != NULL) {
        freeifaddrs(ifAddrStruct);
    }

    return myIp;
}

std::string netSessionTbl::getKey(std::string node_name, FDSP_MgrIdType remote_mgr_id) { 
    std::string node_name_key = node_name;
    switch ( localMgrId ) { 
        case FDSP_STOR_MGR :
            node_name_key  += "_SM";
            break;
        case FDSP_DATA_MGR :  
            node_name_key += "_DM";
            break;
        case FDSP_ORCH_MGR : 
            node_name_key += "_OM";
            break;
        case FDSP_STOR_HVISOR :
            node_name_key += "_AM";
            break;
        case FDSP_CLI_MGR : 
            node_name_key += "_CLI";
            break;
    }
    
    switch ( remote_mgr_id ) { 
        case FDSP_STOR_MGR :
            node_name_key  += "_SM";
            break;
        case FDSP_DATA_MGR :  
            node_name_key += "_DM";
            break;
        case FDSP_ORCH_MGR : 
            node_name_key += "_OM";
            break;
        case FDSP_STOR_HVISOR :
            node_name_key += "_AM";
            break;
        case FDSP_CLI_MGR : 
            node_name_key += "_CLI";
            break;
    }
    return node_name_key;
}

netSession *netSessionTbl::getSession(const std::string& node_name, FDSP_MgrIdType mgr_id) 
{
    netSession* session = NULL;
    std::string node_name_key = getKey(node_name, mgr_id);
    
    sessionTblMutex->lock();
    session = sessionTbl[node_name_key];
    sessionTblMutex->unlock();
    return session;
}

netSession *netSessionTbl::getSession(int ip_addr, FDSP_MgrIdType mgr_id) 
{
    std::string node_name = ipAddr2String(ip_addr);
    return getSession(node_name, mgr_id);
}

void netSessionTbl::endSession(int  dst_ip_addr, FDSP_MgrIdType mgr_id) 
{
    netSession* session = NULL;
    std::string node_name = ipAddr2String(dst_ip_addr);
    session = getSession(node_name, mgr_id);
    session->endSession();
}

void netSessionTbl::endAllSessions() {
    sessionTblMutex->lock();
    for (std::unordered_map<std::string, netSession*>::iterator it = sessionTbl.begin();
         it != sessionTbl.end();
         ++it) {
        netSession* session = it->second;
        session->endSession();
        delete session;
    }
    sessionTbl.clear();
    sessionTblMutex->unlock();
}

void netSessionTbl::listenServer(netSession* server_session) {
    switch(localMgrId) { 
        case FDSP_STOR_MGR: 
            if (server_session->getRemoteMgrId() == FDSP_ORCH_MGR) {
                netControlPathServerSession *servSession = 
                        reinterpret_cast<netControlPathServerSession *>(server_session);
                servSession->listenServer();
            } else {
                netDataPathServerSession *servSession = 
                        reinterpret_cast<netDataPathServerSession *>(server_session);
                servSession->listenServer();
            }
            break;
            
        case FDSP_DATA_MGR: 
            if (server_session->getRemoteMgrId() == FDSP_ORCH_MGR) {
                netControlPathServerSession *servSession = 
                        reinterpret_cast<netControlPathServerSession *>(server_session);
                servSession->listenServer();
            } else {
                netMetaDataPathServerSession *servSession = 
                        reinterpret_cast<netMetaDataPathServerSession *>(server_session);
                servSession->listenServer();
            }
            break;

        case FDSP_STOR_HVISOR:
            if (server_session->getRemoteMgrId() == FDSP_ORCH_MGR) {
                netControlPathServerSession *servSession = 
                        reinterpret_cast<netControlPathServerSession *>(server_session);
                servSession->listenServer();
            }            
            break;

        case FDSP_ORCH_MGR: 
            if (server_session->getRemoteMgrId() == FDSP_CLI_MGR) { 
                netConfigPathServerSession *servSession = 
                        reinterpret_cast<netConfigPathServerSession *>(server_session);
                servSession->listenServer();
            } else if (server_session->getRemoteMgrId() == FDSP_OMCLIENT_MGR) { 
                netOMControlPathServerSession *servSession = 
                        reinterpret_cast<netOMControlPathServerSession *>(server_session);
                servSession->listenServer();
            }
            break;
            
        default:
            break;
    }
}
