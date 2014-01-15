#include <list>
#include <NetSession.h>
#include <NetSessRespClient.h>
#include <NetSessRespSvr.h>
#include <arpa/inet.h>

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

netSession* netSessionTbl::setupClientSession(const std::string& dest_node_name,
                                              int port,
                                              FDS_ProtocolInterface::FDSP_MgrIdType local_mgr_id, 
                                              FDS_ProtocolInterface::FDSP_MgrIdType remote_mgr_id,
                                              void *respSvrObj) {
    netSession *session;
    switch(remote_mgr_id) { 
        case FDSP_STOR_MGR :  
            if (local_mgr_id == FDSP_STOR_HVISOR) { 
                session = dynamic_cast<netSession *>(
                    new netDataPathClientSession(dest_node_name,
                                                 port,
                                                 local_mgr_id,
                                                 remote_mgr_id,
                                                 1, /* number of threads (TODO) */
                                                 respSvrObj));
 
            } else if (local_mgr_id == FDSP_ORCH_MGR) { 
                session = dynamic_cast<netSession *>(
                    new netControlPathClientSession(dest_node_name,
                                                    port,
                                                    local_mgr_id,
                                                    remote_mgr_id,
                                                    1, /* number of threads (TODO) */
                                                    respSvrObj)); 
            }
            break;
            
        case FDSP_DATA_MGR : 
            if (local_mgr_id == FDSP_STOR_HVISOR) { 
                session = dynamic_cast<netSession *>(
                    new netMetaDataPathClientSession(dest_node_name,
                                                     port,
                                                     local_mgr_id,
                                                     remote_mgr_id,
                                                     1, /* number of threasd (TODO) */
                                                     respSvrObj)); 
            } else if (local_mgr_id == FDSP_ORCH_MGR) { 
                session = dynamic_cast<netSession *>(
                    new netControlPathClientSession(dest_node_name,
                                                    port,
                                                    local_mgr_id,
                                                    remote_mgr_id,
                                                    1, /* number of threads (TODO) */
                                                    respSvrObj)); 
            }
            break;
            
        case FDSP_ORCH_MGR: 
            if (local_mgr_id == FDSP_CLI_MGR) { 
                session = dynamic_cast<netSession *>(
                    new netConfigPathClientSession(dest_node_name,
                                                   port,
                                                   local_mgr_id,
                                                   remote_mgr_id,
                                                   1, /* number of threads (TODO) */
                                                   respSvrObj)); 
            }
            break;
    } 
    return session;
}


netSession* netSessionTbl::setupServerSession(const std::string& dest_node_name,
                                              int port,
                                              FDS_ProtocolInterface::FDSP_MgrIdType local_mgr_id,
                                              FDS_ProtocolInterface::FDSP_MgrIdType remote_mgr_id,
					      void *SvrObj) {
    netSession *session = NULL;
    
    switch(local_mgr_id) { 
        case FDSP_STOR_MGR :
            if (remote_mgr_id == FDSP_ORCH_MGR) {
                session = dynamic_cast<netSession *>(
                    new netControlPathServerSession(dest_node_name,
                                                    port,
                                                    local_mgr_id,
                                                    remote_mgr_id,
                                                    1, /* number of threads (TODO) */
                                                    SvrObj)); 
            } else {
                session = dynamic_cast<netSession *>(
                    new netDataPathServerSession(dest_node_name,
                                                 port,
                                                 local_mgr_id,
                                                 remote_mgr_id,
                                                 1, /* number of threads (TODO) */
                                                 SvrObj)); 
            }
            break;
            
        case FDSP_DATA_MGR :
            if (remote_mgr_id == FDSP_ORCH_MGR) {
	      session = dynamic_cast<netSession *>(
                  new netControlPathServerSession(dest_node_name,
                                                  port,
                                                  local_mgr_id,
                                                  remote_mgr_id,
                                                  1, /* number of threads (TODO) */
                                                  SvrObj));             
            } else {
	      session = dynamic_cast<netSession *>(
                  new netMetaDataPathServerSession(dest_node_name,
                                                   port,
                                                   local_mgr_id,
                                                   remote_mgr_id,
                                                   1, /* number of threads (TODO) */
                                                   SvrObj)); 
            }
            break;

        case FDSP_ORCH_MGR: 
            if (remote_mgr_id == FDSP_CLI_MGR) { 
                session = dynamic_cast<netSession *>(
                    new netConfigPathServerSession(dest_node_name,
                                                   port,
                                                   local_mgr_id,
                                                   remote_mgr_id,
                                                   1, /* number of threads (TODO) */
                                                   SvrObj)); 
            } else {
                session = dynamic_cast<netSession *>(
                    new netControlPathServerSession(dest_node_name,
                                                    port,
                                                    local_mgr_id,
                                                    remote_mgr_id,
                                                    1, /* number of threads (TODO) */
                                                    SvrObj)); 
            }
            break;
    } 
    return session;
}

netSession::~netSession()
{
    this->endSession();
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

netSession* netSessionTbl::startSession(int ipaddr,
					int port,
					FDSP_MgrIdType remote_mgr_id,
					int num_channels,
					void *respSvr)
{
    std::string node_name = ipAddr2String(ipaddr);
    return startSession(node_name, port, remote_mgr_id, num_channels, respSvr);
}

std::string netSessionTbl::getKey(std::string node_name, FDSP_MgrIdType remote_mgr_id) { 
    std::string node_name_key = node_name;
    switch ( localMgrId ) { 
        case FDSP_STOR_MGR :
            node_name_key  += "_SM";
            break;
        case FDSP_DATA_MGR :  
            node_name += "_DM";
            break;
        case FDSP_ORCH_MGR : 
            node_name += "_OM";
            break;
        case FDSP_STOR_HVISOR :
            node_name += "_AM";
            break;
        case FDSP_CLI_MGR : 
            node_name += "_CLI";
            break;
    }
    
    switch ( remote_mgr_id ) { 
        case FDSP_STOR_MGR :
            node_name_key  += "_SM";
            break;
        case FDSP_DATA_MGR :  
            node_name += "_DM";
            break;
        case FDSP_ORCH_MGR : 
            node_name += "_OM";
            break;
        case FDSP_STOR_HVISOR :
            node_name += "_AM";
            break;
        case FDSP_CLI_MGR : 
            node_name += "_CLI";
            break;
    }
    return node_name_key;
}

netSession* netSessionTbl::startSession(const std::string& node_name,
                                        int port,
                                        FDS_ProtocolInterface::FDSP_MgrIdType remote_mgr_id,
                                        int num_channels,
                                        void *respSvr) 
{
    netSession *session = static_cast<netSession *>
            (setupClientSession(node_name, port, localMgrId, remote_mgr_id, respSvr));
    if (session == NULL) { 
        return NULL;
    }
    std::string node_name_key = getKey(node_name, remote_mgr_id);
    
    sessionTblMutex->lock();
    sessionTbl[node_name_key] = session;
    sessionTblMutex->unlock();
    
    return session;
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

void netSession::endSession() 
{
    // TODO -- calling delete on netSession should close everything I think 
    //   transport->close();
}

void netSessionTbl::endSession(netSession *session)  {
    session->endSession();
}

netSession* netSessionTbl::createServerSession(int local_ipaddr, 
					       int _port, 
					       std::string local_node_name, 
					       FDSP_MgrIdType remote_mgr_id, 
					       void *respHandlerObj) {
    netSession* session = NULL;
    src_ipaddr = local_ipaddr;
    port = _port;
    src_node_name = local_node_name;
    remoteMgrId = remote_mgr_id;

    session = setupServerSession(local_node_name, port, localMgrId, remote_mgr_id, respHandlerObj);
    if ( session == NULL ) {
       return NULL;
    }
    std::string node_name_key = getKey(local_node_name, remote_mgr_id);
    
    sessionTblMutex->lock();
    sessionTbl[node_name_key] = session;
    sessionTblMutex->unlock();
    threadManager = ThreadManager::newSimpleThreadManager(num_threads);
    threadFactory = boost::shared_ptr<PosixThreadFactory>(new PosixThreadFactory());
    threadManager->threadFactory(threadFactory);
    threadManager->start();

    return session;
}

void netSessionTbl::listenServer(netSession* server_session) {
    switch(localMgrId) { 
        case FDSP_STOR_MGR: 
            {
                netDataPathServerSession *servSession = 
                        reinterpret_cast<netDataPathServerSession *>(server_session);
                servSession->listenServer();
            }
            break;
            
        case FDSP_DATA_MGR: 
            {
                netMetaDataPathServerSession *servSession = 
                        reinterpret_cast<netMetaDataPathServerSession *>(server_session);
                servSession->listenServer();
            }
            break;
            
        case FDSP_ORCH_MGR: 
            if ( remoteMgrId == FDSP_CLI_MGR) { 
                netConfigPathServerSession *servSession = 
                        reinterpret_cast<netConfigPathServerSession *>(server_session);
                servSession->listenServer();
            } else { 
                //FDSP_ControlPathServerSession *servSession = 
                //dynamic_cast<FDSP_ConfigPathServerSession *>(server_session);
                //servSession->listenServer();
            }
            break;
            
        default:
            break;
    }
}
