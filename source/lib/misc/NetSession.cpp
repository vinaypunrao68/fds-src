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

FdsTProcessorEventHandler::FdsTProcessorEventHandler()
{
}

void FdsTProcessorEventHandler::handlerError(void* ctx, const char* fn_name)
{
    LOGCRITICAL << "TProcessor error at: " << fn_name;
    fds_assert(!"TProcessor error");
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
    synchronizedptr(sessionTblMutex) {
        auto iter = sessionTbl.find(key);
        if (iter != sessionTbl.end()) {
            return iter->second;
        }
    }
    LOGWARN << "unable to locate session for key:" << key;
    return nullptr;
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

bool netSessionTbl::endSession(const netSession* session) {
    synchronizedptr(sessionTblMutex) {
        for (auto iter = sessionTbl.begin() ; iter != sessionTbl.end() ; ++iter) {
            if (iter->second == session) {
                LOGNORMAL << "ending session:" << iter->first;
                iter->second->endSession();
                // TODO(prem) - should we delete ?
                // delete iter->second;
                sessionTbl.erase(iter);
                return true;
            }
        }
    }
    LOGWARN << "unable to locate session in the session tbl";
    return false;
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
        case FDSP_PLATFORM:
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
