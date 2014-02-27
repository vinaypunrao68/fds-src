/* Copyright 2013 Formation Data Systems, Inc.
 */
#include <ifaddrs.h>
#include <list>
#include <NetSession.h>
#include <NetSessRespClient.h>
#include <NetSessRespSvr.h>
#include <arpa/inet.h>
#include <fds_assert.h>
#include <regex>
#include <sstream>
#include <string>

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
    inet_ntop(AF_INET, reinterpret_cast<void *>(&(sa.sin_addr)),
              buf, INET_ADDRSTRLEN);
    string ipaddr_str(buf);
    return (ipaddr_str);
}

string netSessionTbl::ipAddr2String(int ipaddr) {
    struct sockaddr_in sa;
    char buf[32];
    sa.sin_addr.s_addr = htonl(ipaddr);
    inet_ntop(AF_INET, reinterpret_cast<void *>(&(sa.sin_addr)),
              buf, INET_ADDRSTRLEN);
    string ipaddr_str(buf);
    return (ipaddr_str);
}

fds_int32_t netSession::ipString2Addr(string ipaddr_str) {
    struct sockaddr_in sa;
    sa.sin_addr.s_addr = 0;
    inet_pton(AF_INET, reinterpret_cast<const char *>(ipaddr_str.data()),
              reinterpret_cast<void *>(&(sa.sin_addr)));
    return (ntohl(sa.sin_addr.s_addr));
}

fds_int32_t netSessionTbl::ipString2Addr(string ipaddr_str) {
    struct sockaddr_in sa;
    sa.sin_addr.s_addr = 0;
    inet_pton(AF_INET, reinterpret_cast<const char *>(ipaddr_str.data()),
              reinterpret_cast<void *>(&(sa.sin_addr)));
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
        if (ifa->ifa_addr->sa_family == AF_INET) {  // IPv4
            if (strncmp(ifa->ifa_name, "lo", 2) != 0) {
                tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;  // NOLINT
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

std::string
netSessionTbl::getServerSessionKey(const std::string &ip, const int &port)
{
    std::stringstream ss;
    ss << "srvr_" << ip << ":" << port;
    return ss.str();
}
std::string
netSessionTbl::getClientSessionKey(const std::string &ip, const int &port)
{
    std::stringstream ss;
    ss << "client_" << ip << ":" << port;
    return ss.str();
}

bool netSessionTbl::clientSessionExists(const int &ip, const int &port)
{
    std::string ip_str = ipAddr2String(ip);
    std::string key = getClientSessionKey(ip_str, port);

    sessionTblMutex->lock();
    bool exists = (sessionTbl.find(key) != sessionTbl.end());
    sessionTblMutex->unlock();

    return exists;
}

netSession* netSessionTbl::getSession(const std::string& key)
{
    sessionTblMutex->lock();
    auto itr = sessionTbl.find(key);
    if (itr == sessionTbl.end()) {
        sessionTblMutex->unlock();
        /* NOTE: Remove if not needed */
        fds_assert(!"Null session");
        return nullptr;
    }
    netSession* session = itr->second;
    sessionTblMutex->unlock();
    return session;
}

void netSessionTbl::endClientSession(const int  &ip, const int &port)
{
    netSession* session = NULL;
    std::string ip_str = ipAddr2String(ip);
    std::string key = getClientSessionKey(ip_str, port);
    endSession(key);
}

void netSessionTbl::endSession(const std::string &key) {
    sessionTblMutex->lock();
    auto itr = sessionTbl.find(key);
    if (itr != sessionTbl.end()) {
        itr->second->endSession();
        sessionTbl.erase(itr);
    } else {
        LOGWARN << "netSession " << key << " doesn't exist";
        fds_assert(!"netSession disappered");
    }
    sessionTblMutex->unlock();
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
    switch (localMgrId) {
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
