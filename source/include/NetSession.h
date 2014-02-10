#ifndef __Net_Session_h__
#define __Net_Session_h__
#include <concurrency/Mutex.h>
#include <stdio.h>
#include <thread>
#include <iostream>
#include <fds_types.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TTransportUtils.h>
#include <fdsp/FDSP_DataPathReq.h>
#include <fdsp/FDSP_DataPathResp.h>
#include <fdsp/FDSP_MetaDataPathReq.h>
#include <fdsp/FDSP_MetaDataPathResp.h>
#include <fdsp/FDSP_ConfigPathReq.h>
#include <fdsp/FDSP_ConfigPathResp.h>
#include <fdsp/FDSP_MigrationPathReq.h>
#include <fdsp/FDSP_MigrationPathResp.h>
#include <fdsp/FDSP_Service.h>
#include <fds_globals.h>
#include <NetSessRespClient.h>
#include <NetSessRespSvr.h>
#include <util/Log.h>
#include <fds_assert.h>
#include <fds_err.h>

#include <fds_uuid.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::concurrency;

using namespace FDS_ProtocolInterface;
using namespace std;
using namespace fds;

#define NETSESS_SERVER 1
#define NETSESS_CLIENT 0

typedef void  (*sessionErrorCallback)(string ip_addr, 
        FDSP_MgrIdType mgrId,
        int channel, int errno,
        std::string errMsg);

typedef boost::shared_ptr<TTransport> TTransportPtr;

class netServerSession;

class netSession {
public:
    netSession();
    netSession(const std::string& node_name, int port,
            FDS_ProtocolInterface::FDSP_MgrIdType local_mgr_id,
            FDS_ProtocolInterface::FDSP_MgrIdType remote_mgr_id);
    netSession(int  ip_addr_int, int port,
            FDS_ProtocolInterface::FDSP_MgrIdType local_mgr_id,
            FDS_ProtocolInterface::FDSP_MgrIdType remote_mgr_id);
    static string ipAddr2String(int ipaddr);
    static int ipString2Addr(string ipaddr_str);
    static std::string getLocalIp();

    inline static std::string getIPV4FromMappedAddress(const std::string& peer_addr)
    {
        std::string peer_address;
        char *paddr = (char *)peer_addr.data();

        if (strncmp(peer_addr.c_str(), "::ffff:", (sizeof("::ffff:") -1)) == 0 ) {
            paddr = paddr + sizeof("::ffff:") -1;
            peer_address.append(paddr);
        } else {
            peer_address =  peer_addr;
        }
        return peer_address;
    }

    void setSessionErrHandler(sessionErrorCallback cback);

    virtual ~netSession();
    virtual void endSession() = 0;

    int 		node_index;
    int 		channel_number;
    short   	proto_type;
    std::string 	ip_addr_str;
    int 		ip_addr;
    fds_uint32_t    port_num;
    sessionErrorCallback cback;

    netSession& operator=(const netSession& rhs) {
        if (this != &rhs) {
            node_index = rhs.node_index;
            proto_type = rhs.proto_type;
            ip_addr_str = rhs.ip_addr_str;
            ip_addr    = rhs.ip_addr;
            localMgrId      = rhs.localMgrId;
            remoteMgrId      = rhs.remoteMgrId;
        }
        return *this;
    }

    /* accessor methods */
    FDS_ProtocolInterface::FDSP_MgrIdType getRemoteMgrId() const {
        return remoteMgrId;
    }
    FDS_ProtocolInterface::FDSP_MgrIdType getLocalMgrId() const {
        return localMgrId;
    }
    void setSessionRole(int role_){
        role = role_;
    }

private:
    int role; // Server or Client side binding
    FDS_ProtocolInterface::FDSP_MgrIdType localMgrId;
    FDS_ProtocolInterface::FDSP_MgrIdType remoteMgrId;
};

/**
 * @brief netClientSession encapsualtes client reqeust/response
 * handling interfaces.  It's responsible for 
 * -establising a connection with the server (This involes a handshake).
 * -Running a a thread to handle respsonses from the server.
 * NOTE:  Communication between client and the server is done over
 * one connection for now.  That means writes to the socket need to
 * be syncronized (as multiple threads do the write).  Reads don't need
 * to be synchronized (currenly only one thread does the reads).
 * @tparam ReqClientT
 * @tparam RespProcessorT
 * @tparam RespHandlerT
 */
template <class ReqClientT, class RespProcessorT, class RespHandlerT>
class netClientSessionEx : public netSession { 
public:
    netClientSessionEx(string node_name, int port, FDSP_MgrIdType local_mgr,
            FDSP_MgrIdType remote_mgr,
            boost::shared_ptr<RespHandlerT> resp_handler)
: netSession(node_name, port, local_mgr, remote_mgr),
  socket_(new apache::thrift::transport::TSocket(node_name, port)),
  transport_(new apache::thrift::transport::TBufferedTransport(
          boost::dynamic_pointer_cast<apache::thrift::transport::TTransport>(socket_))),
          protocol_(new TBinaryProtocol(transport_)),
          session_id_(""),
          resp_handler_(resp_handler)
{
}

    virtual bool start()
    {
        try {
            /* First do a connection request to get a session id */
            transport_->open();
            while (!transport_->isOpen()) {
                usleep(500);
            }
            establishSession();

            /* Create the interface for issuing requests */
            req_client_.reset(new ReqClientT(protocol_));

            if (resp_handler_) {
                /* Create the interface for receiving responses */
                resp_processor_.reset(new RespProcessorT(resp_handler_));
                recv_thread_.reset(new std::thread(
                        &netClientSessionEx<ReqClientT,
                        RespProcessorT, RespHandlerT>::run, this));
            }
        } catch (const std::exception &e) {
            FDS_PLOG_WARN(g_fdslog) << e.what();
            return false;
        } catch (...) {
            FDS_PLOG_WARN(g_fdslog) << "Exception";
            return false;
        }
        return true;
    }

    void run()
    {
        /* wait for connection to be established */
        /* for now just busy wait */
        while (!protocol_->getTransport()->isOpen()) {
            printf("fdspDataPathRespReceiver: waiting for transport...\n");
            usleep(500);
        }

        try {
            for (;;) {
                if (!resp_processor_->process(protocol_, protocol_, NULL) ||
                        !protocol_->getTransport()->peek()) {
                    break;
                }
            }
        }
        catch (TException& tx) {
            FDS_PLOG_WARN(g_fdslog) << "Receiver exception" << tx.what();
        } catch (...) {
            FDS_PLOG_WARN(g_fdslog) << "Uncaught exception ";
        }
    }

    virtual ~netClientSessionEx() {
        if (transport_ && transport_->isOpen())
            transport_->close();

        if (recv_thread_) {
            recv_thread_->join();
        }
    }

    virtual void endSession() {
        if (transport_ && transport_->isOpen())
            transport_->close();
    }

    boost::shared_ptr<ReqClientT> getClient() {
        return req_client_;
    }

    boost::shared_ptr<RespHandlerT> getRespHandler()
          {
        return resp_handler_;
          }

    std::string getSessionId() {
        return session_id_;
    }

    virtual std::string log_string() {
        std::stringstream ret;
        ret << ip_addr_str << port_num;
        return ret.str();
    }

protected:
    /**
     * @brief Once client connects the first thing it should do is invoked
     * this method to get a session id.  All futher communication should
     * use this session id
     *
     * @param client_if
     */
    virtual void establishSession()
    {
        FDSP_SessionReqResp session_info;
        FDSP_MsgHdrType fdsp_msg;
        boost::shared_ptr<FDSP_ServiceClient> client_if(new FDSP_ServiceClient(protocol_));

        session_info.status = 0;
        /* We will overload src_node_name to contain the ip address */
        fdsp_msg.src_node_name = socket_->getPeerAddress();
        fdsp_msg.src_port = socket_->getPeerPort();

        client_if->EstablishSession(session_info, fdsp_msg);
        // TODO: based on return code the do the appropriate
        fds_verify(session_info.status == 0 && !session_info.sid.empty());
        session_id_ = session_info.sid;

        FDS_PLOG(g_fdslog) << __FUNCTION__ << " sid: " << session_id_;
    }
protected:
    boost::shared_ptr<apache::thrift::transport::TSocket> socket_;
    boost::shared_ptr<TTransport> transport_;
    boost::shared_ptr<TProtocol> protocol_;
    boost::shared_ptr<TThreadPoolServer> server;
    std::string session_id_;

    /* Thrift generated client interface */
    boost::shared_ptr<ReqClientT> req_client_;

    /* Thrift generated processor interface */
    boost::shared_ptr<RespProcessorT> resp_processor_;

    /* Response handler */
    boost::shared_ptr<RespHandlerT> resp_handler_;

    /* Responses to client requests are serviced on this thread */
    boost::shared_ptr<std::thread> recv_thread_;
};

typedef netClientSessionEx<FDSP_DataPathReqClient,
        FDSP_DataPathRespProcessor,FDSP_DataPathRespIf> netDataPathClientSession;
typedef netClientSessionEx<FDSP_MetaDataPathReqClient,
        FDSP_MetaDataPathRespProcessor,FDSP_MetaDataPathRespIf> netMetaDataPathClientSession;
typedef netClientSessionEx<FDSP_ControlPathReqClient,
        FDSP_ControlPathRespProcessor,FDSP_ControlPathRespIf> netControlPathClientSession;
typedef netClientSessionEx<FDSP_OMControlPathReqClient,
        FDSP_OMControlPathRespProcessor,FDSP_OMControlPathRespIf> netOMControlPathClientSession;
typedef netClientSessionEx<FDSP_ConfigPathReqClient,
        FDSP_ConfigPathRespProcessor, FDSP_ConfigPathRespIf> netConfigPathClientSession;
typedef netClientSessionEx<FDSP_MigrationPathReqClient,
        FDSP_MigrationPathRespProcessor, FDSP_MigrationPathRespIf> netMigrationPathClientSession;

/**
 * @brief Encapsulates functionality for fds server sessions.  Responsibilities
 * include
 * -Running a server for handling incoming requests.  Request Handler needs to
 *  be provided as input.
 * -Managing lifecycle of incoming connections (not implemented yet).
 * -Managing response clients.  For each incoming connection request a response 
 *  client is created.  Response client handles sending responses.  The idea is 
 *  to share the same socket for both request and response.
 *
 * @tparam ReqProcessorT
 * @tparam ReqHandlerT
 * @tparam RespClientT
 */
template <class ReqProcessorT, class ReqHandlerT, class RespClientT>
class netServerSessionEx: public netSession { 
public :
    netServerSessionEx(string node_name,
            int port,
            FDSP_MgrIdType local_mgr_id,
            FDSP_MgrIdType remote_mgr_id,
            int num_threads,
            boost::shared_ptr<ReqHandlerT> handler)
: netSession(node_name, port, local_mgr_id, remote_mgr_id),
  lock_("netServerSession lock"),
  handler_(handler)
{
        /* Keeping these local here.  Expose them if needed */
        boost::shared_ptr<TServerTransport> serverTransport;
        boost::shared_ptr<TTransportFactory> transportFactory;
        boost::shared_ptr<TProtocolFactory> protocolFactory;
        boost::shared_ptr<PosixThreadFactory> threadFactory;
        boost::shared_ptr<ThreadManager> threadManager;
        boost::shared_ptr<TProcessor> processor;

        serverTransport.reset(new apache::thrift::transport::TServerSocket(port));
        transportFactory.reset(getTransportFactory());
        protocolFactory.reset( new TBinaryProtocolFactory());

        processor.reset(new ReqProcessorT(handler_));

        event_handler_.reset(new ServerEventHandler(*this));

        threadManager = ThreadManager::newSimpleThreadManager(num_threads);
        threadFactory = boost::shared_ptr<PosixThreadFactory>(new PosixThreadFactory());
        threadManager->threadFactory(threadFactory);
        threadManager->start();

        server_.reset(new TThreadPoolServer (processor,
                serverTransport,
                transportFactory,
                protocolFactory,
                threadManager));
        server_->setServerEventHandler(event_handler_);
}

    virtual ~netServerSessionEx()
    {
        server_->stop();
    }

    virtual void endSession()
    {
        server_->stop();
    }

    void listenServer()
    {
        std::cerr << "Starting the server..." << std::endl;
        server_->serve();
    }

    virtual Error addRespClientSession(const std::string &session_id, TTransportPtr t)
    {
        boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(t));
        boost::shared_ptr<RespClientT> resp_cli( new RespClientT(protocol));
        respClients_[session_id] = resp_cli;
        return ERR_OK;
    }

    boost::shared_ptr<RespClientT> getRespClient(const std::string& sid)
          {
        fds_mutex::scoped_lock l(lock_);
        auto itr = respClients_.find(sid);
        if (itr == respClients_.end()) {
            return nullptr;
        }
        return itr->second;
          }

    virtual std::string log_str()
    {
        std::stringstream ret;
        ret << ip_addr_str << port_num;
        return ret.str();
    }

private:
    class FDSP_ConnectHandler : virtual public FDSP_ServiceIf {
    public:
        FDSP_ConnectHandler(
                netServerSessionEx<ReqProcessorT, ReqHandlerT, RespClientT> &parent,
                TTransportPtr t)
    : parent_(parent),
      transport_(t)
    {
    }

        /**
         * @brief We get this request after socket connect.  As part of this call
         * we will create session and associate the connection with the generated
         * session id.  This connection is also used for response client
         *
         * @param _return
         * @param fdsp_msg
         */
        virtual void EstablishSession(FDSP_SessionReqResp& _return,
                const FDSP_MsgHdrType& fdsp_msg) override
                        {
            /* Generate a uuid and add a session.  Any further authentication can be
             * done here
             */
            /*srvr_session_->addRespClientSession(fdsp_msg.src_node_name,
          fdsp_msg.src_port, fds::get_uuid());*/
            _return.sid = fds::get_uuid();
            Error e = parent_.addRespClientSession(_return.sid, transport_);
            _return.status = e.GetErrno();
                        }

        virtual void EstablishSession(FDSP_SessionReqResp& _return,
                boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg) override
                        {
            EstablishSession(_return, *fdsp_msg);
                        }

    protected:
        netServerSessionEx<ReqProcessorT, ReqHandlerT, RespClientT> &parent_;
        TTransportPtr transport_;
    };

    private:
    /**
     * @brief Listener for events from Thrift Server
     */
    class ServerEventHandler : public TServerEventHandler
    {
    public:
        ServerEventHandler(netServerSessionEx<ReqProcessorT, ReqHandlerT, RespClientT> &parent)
    : parent_(parent)
    {
    }
        /**
         * Called when a new client has connected and is about to being processing.
         */
        virtual void* createContext(boost::shared_ptr<TProtocol> input,
                boost::shared_ptr<TProtocol> output) override
                        {
            fds_mutex::scoped_lock l(parent_.lock_);
            TTransportPtr transport = input->getTransport();
            boost::shared_ptr<FDSP_ServiceIf> iface(new FDSP_ConnectHandler(parent_, transport));
            FDSP_ServiceProcessor conn_processor(iface);
            bool ret = conn_processor.process(input, output, NULL);
            if (!ret) {
                //FDS_PLOG(g_fdslog) << "Failed to process conn request ";
            }
            return NULL;
                        }
        /**
         * Called when a client has finished request-handling to delete server
         * context.
         */
        virtual void deleteContext(void* serverContext,
                boost::shared_ptr<TProtocol>input,
                boost::shared_ptr<TProtocol>output)
        {
            fds_mutex::scoped_lock l(parent_.lock_);
            // TODO: Cleaning up session.  We dont have ip+port->sid mapping.
            // We may have to extend auth_pending_transports_ to contain this
            // info along with state information
        }
    private:
        netServerSessionEx<ReqProcessorT, ReqHandlerT, RespClientT> &parent_;
    };


    protected:
    TTransportFactory* getTransportFactory()
    {
        /* NOTE: if return a diffrent TTransportFactory, make sure you adjust
         * getTransportKey() to match as well
         */
        return new apache::thrift::transport::TBufferedTransportFactory();
    }

    static std::string getTransportKey(TTransportPtr transport)
    {
        /* What we get is TBufferedTransport.  We will extract TSocket from it */
        boost::shared_ptr<apache::thrift::transport::TBufferedTransport> buf_transport =
                boost::static_pointer_cast<apache::thrift::transport::TBufferedTransport>(transport);

        boost::shared_ptr<apache::thrift::transport::TSocket> sock =
                boost::static_pointer_cast<apache::thrift::transport::TSocket>\
                (buf_transport->getUnderlyingTransport());

        return getTransportKey(sock->getPeerAddress(), sock->getPeerPort());
    }

    static std::string getTransportKey(const std::string &ip, const int &port)
    {
        std::stringstream ret;
        // Convert any IPv4 mapped address to normal ipv4 address for the key
        std::string ip_addr = netSession::getIPV4FromMappedAddress(ip);
        ret << ip_addr << ":" << port;
        return ret.str();
    }

    protected:
    boost::shared_ptr<TThreadPoolServer> server_;
    boost::shared_ptr<ReqHandlerT> handler_;

    /* Lock to protect auth_pending_transports and respClients */
    fds_mutex lock_;

    // TODO:  We need to keep track of connections with the server
    /* Transports pending authorization */
    //std::unordered_map<std::string, TTransportPtr> auth_pending_transports_;

    std::unordered_map<std::string, boost::shared_ptr<RespClientT> > respClients_;

    /* TServer event handler */
    boost::shared_ptr<ServerEventHandler> event_handler_;
};

typedef netServerSessionEx<FDSP_DataPathReqProcessor, 
        FDSP_DataPathReqIf, FDSP_DataPathRespClient> netDataPathServerSession;
typedef netServerSessionEx<FDSP_MetaDataPathReqProcessor, 
        FDSP_MetaDataPathReqIf, FDSP_MetaDataPathRespClient> netMetaDataPathServerSession;
typedef netServerSessionEx<FDSP_ControlPathReqProcessor, 
        FDSP_ControlPathReqIf, FDSP_ControlPathRespClient> netControlPathServerSession;
typedef netServerSessionEx<FDSP_OMControlPathReqProcessor, 
        FDSP_OMControlPathReqIf, FDSP_OMControlPathRespClient> netOMControlPathServerSession;
typedef netServerSessionEx<FDSP_ConfigPathReqProcessor, 
        FDSP_ConfigPathReqIf, FDSP_ConfigPathRespClient> netConfigPathServerSession;
typedef netServerSessionEx<FDSP_MigrationPathReqProcessor,
        FDSP_MigrationPathReqIf, FDSP_MigrationPathRespClient> netMigrationPathServerSession;


inline std::ostream& operator<<(std::ostream& out, const netSession& ep) {
    FDS_ProtocolInterface::FDSP_MgrIdType local_mgr_id = ep.getLocalMgrId();
    out << "Network endpoint ";

    if (local_mgr_id == FDSP_DATA_MGR) {
        out << "DM";
    } else if (local_mgr_id == FDSP_STOR_MGR) {
        out << "SM";
    } else if (local_mgr_id == FDSP_ORCH_MGR) {
        out << "OM";
    } else {
        assert(local_mgr_id == FDSP_STOR_HVISOR);
        out << "SH";
    }
    out << " with IP " << ep.ip_addr
            << " and port " << ep.port_num;
    return out;
}

class netSessionTbl {
public:
    netSessionTbl(std::string _src_node_name,
            int _src_ipaddr,
            int _port,
            int _num_threads,
            FDSP_MgrIdType myMgrId)
: src_node_name(_src_node_name),
  src_ipaddr(_src_ipaddr),
  port(_port),
  localMgrId(myMgrId),
  num_threads(_num_threads) {
        sessionTblMutex = new fds_mutex("RPC Tbl mutex");
    }
    netSessionTbl(FDSP_MgrIdType myMgrId)
    : netSessionTbl("", 0, 0, 50, myMgrId) {
    }
    ~netSessionTbl();

    static string ipAddr2String(int ipaddr);
    static int ipString2Addr(string ipaddr_str);
    std::string getKey(std::string node_name, FDSP_MgrIdType remote_mgr_id);

    // Client Procedures
    // TODO: Change to return shared ptr
    template <class ClientSessionT, class RespHandlerT>
    ClientSessionT* startSession(int  dst_ipaddr, int port,
            FDSP_MgrIdType remote_mgr_id, int num_channels,
            boost::shared_ptr<RespHandlerT>respHandler)
    {
        std::string node_name = ipAddr2String(dst_ipaddr);
        return startSession<ClientSessionT, RespHandlerT>(node_name,
                port, remote_mgr_id,
                num_channels, respHandler);
    }

    template <class ClientSessionT, class RespHandlerT>
    ClientSessionT* startSession(const std::string& dst_node_name,
            int port, FDSP_MgrIdType remote_mgr_id,
            int num_channels, boost::shared_ptr<RespHandlerT>respHandler)
    {
        ClientSessionT* session = new ClientSessionT(dst_node_name, port, localMgrId, remote_mgr_id, respHandler);
        if (session == nullptr) {
            return nullptr;
        }
        bool ret = session->start();
        if (!ret) {
            delete session;
            FDS_PLOG_WARN(g_fdslog) << "Failed to start session";
            return nullptr;
        }

        std::string node_name_key = getKey(dst_node_name, remote_mgr_id);

        sessionTblMutex->lock();
        sessionTbl[node_name_key] = session;
        sessionTblMutex->unlock();

        return session;
    }

    void 	      endSession(int  dst_ip_addr, FDSP_MgrIdType);

    void 	      endSession(const std::string& dst_node_name, FDSP_MgrIdType);

    void 	      endSession(netSession *);

    /* Ends all client and server sessions in this table */
    void endAllSessions();

    // client side getSession
    netSession*       getSession(int dst_ip_addr, FDSP_MgrIdType mgrId);

    netSession*       getSession(const std::string& dst_node_name, FDSP_MgrIdType mgrId);

    // Server side getSession
    netSession*       getServerSession(int dst_ip_addr, FDSP_MgrIdType mgrId);

    netSession*       getServerSession(const std::string& dst_node_name, FDSP_MgrIdType mgrId);

    // TODO: Make this interface return shared_ptr
    template <class ServerSessionT, class ReqHandlerT>
    ServerSessionT* createServerSession(int local_ipaddr,
            int _port,
            std::string local_node_name,
            FDSP_MgrIdType remote_mgr_id,
            boost::shared_ptr<ReqHandlerT> reqHandler) {
        ServerSessionT* session;
        src_ipaddr = local_ipaddr;
        port = _port;
        src_node_name = local_node_name;

        session = new ServerSessionT(local_node_name, port, localMgrId,
                remote_mgr_id, num_threads, reqHandler);
        if ( session == nullptr ) {
            return nullptr;
        }
        session->setSessionRole(NETSESS_SERVER);
        std::string node_name_key = getKey(local_node_name, remote_mgr_id);

        sessionTblMutex->lock();
        sessionTbl[node_name_key] = session;
        sessionTblMutex->unlock();

#if 0
        // TODO:  Why do we need this for every server session
        threadManager = ThreadManager::newSimpleThreadManager(num_threads);
        threadFactory = boost::shared_ptr<PosixThreadFactory>(new PosixThreadFactory());
        threadManager->threadFactory(threadFactory);
        threadManager->start();
#endif

        return session;
    }


    // Blocking call equivalent to .run or .serve
    void              listenServer(netSession* server_session);

    void              endServerSession(netSession *server_session );

    virtual std::string log_string() {
        return "NetSessionTable";
    }

    std::string src_node_name;
    int src_ipaddr;
    int port;
    FDSP_MgrIdType localMgrId;

private: /* data */
    std::unordered_map<std::string, netSession*> sessionTbl;
    fds_mutex   *sessionTblMutex;

    int num_threads;

#if 0
    // Server Side Local variables
    boost::shared_ptr<ThreadManager> threadManager;
    boost::shared_ptr<PosixThreadFactory> threadFactory;
#endif
};

typedef boost::shared_ptr<netSessionTbl> netSessionTblPtr;


#endif
