#ifndef __Net_Session_h__
#define __Net_Session_h__
#include <concurrency/Mutex.h>
#include <stdio.h>
#include <thread>
#include <iostream>
#include <fds_types.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <boost/thread.hpp>
#include <thrift/concurrency/ThreadManager.h>
// #include <thrift/concurrency/PosixThreadFactory.h>
// #include <thrift/protocol/TBinaryProtocol.h>
// #include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TThreadedServer.h>
// #include <thrift/transport/TServerSocket.h>
// #include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TTransportUtils.h>
#include <fdsp/FDSP_DataPathReq.h>
#include <fdsp/FDSP_DataPathResp.h>
#include <fdsp/FDSP_MetaDataPathReq.h>
#include <fdsp/FDSP_MetaDataPathResp.h>
#include <fdsp/FDSP_ConfigPathReq.h>
#include <fdsp/FDSP_ConfigPathResp.h>
#include <fdsp/FDSP_MetaSyncReq.h>
#include <fdsp/FDSP_MetaSyncResp.h>
#include <fdsp/FDSP_Service.h>

#include <fds_globals.h>
#include <NetSessRespClient.h>
#include <NetSessRespSvr.h>
#include <net/fdssocket.h>
#include <util/Log.h>
#include <fds_assert.h>
#include <fds_error.h>

#include <fds_uuid.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::concurrency;

namespace att = apache::thrift::transport;
namespace atc = apache::thrift::concurrency;

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


/**
 * TProcessor event handler.  Override this class if you want to peform
 * actions around functions executed by TProcessor.  This class just
 * overrides handlerError() to catch any exceptions thrown by functions
 * executed by TProcessor
 */
class FdsTProcessorEventHandler : public TProcessorEventHandler {
public:
    FdsTProcessorEventHandler();
    virtual void handlerError(void* ctx, const char* fn_name) override;
};

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

  void setSessionTblKey(const std::string &key) {
      session_tbl_key_ = key;
  }

  std::string getSessionTblKey() {
      return session_tbl_key_;
  }

 private:
  int role; // Server or Client side binding
  FDS_ProtocolInterface::FDSP_MgrIdType localMgrId;
  FDS_ProtocolInterface::FDSP_MgrIdType remoteMgrId;
  std::string session_tbl_key_;
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
class netClientSessionEx : public netSession , public net::SocketEventHandler {
 public:
  netClientSessionEx(string node_name, int port, FDSP_MgrIdType local_mgr,
                     FDSP_MgrIdType remote_mgr,
                     boost::shared_ptr<RespHandlerT> resp_handler)
      : netSession(node_name, port, local_mgr, remote_mgr),
            socket_(new fds::net::Socket(node_name, port)),
      transport_(new apache::thrift::transport::TFramedTransport(
              boost::dynamic_pointer_cast<apache::thrift::transport::TTransport>(socket_))),
      protocol_(new TBinaryProtocol(transport_)),
      session_id_(""),
      resp_handler_(resp_handler)
    {
        socket_->setEventHandler(this);
    }

  void onSocketDisconnect() {
      LOGCRITICAL << " Socket got disconnected";
  }

  virtual bool start()
  {
      try {
          if (!socket_->connect()) {
              LOGCRITICAL << "unable to connect to server";
              return false;
          }

          LOGDEBUG << "establish session";
          establishSession();

          /* Create the interface for issuing requests */
          req_client_.reset(new ReqClientT(protocol_));

          if (resp_handler_) {
              LOGDEBUG << "starting the recv thread" ;
              /* Create the interface for receiving responses */
              resp_processor_.reset(new RespProcessorT(resp_handler_));
              resp_processor_->setEventHandler(
                      boost::shared_ptr<FdsTProcessorEventHandler>(
                              new FdsTProcessorEventHandler()));
              recv_thread_.reset(new boost::thread(
                      &netClientSessionEx<ReqClientT,
                      RespProcessorT, RespHandlerT>::run, this));
          }
          return true;
      } catch(const att::TTransportException& e) {
          LOGCRITICAL << "error during network call : " << e.what();
      } catch (const std::exception &e) {
          LOGCRITICAL << e.what();
      } catch (...) {
          LOGCRITICAL << "unknown Exception";
      }
      return false;
  }

  void run() {
      LOGDEBUG << "run started" ;
      try {
          for (;socket_->peek() && resp_processor_->process(protocol_, protocol_, NULL);) ;
      } catch(const att::TTransportException& e) {
          LOGCRITICAL << "error during network call : " << e.what();
      }
      catch (TException& tx) {
          LOGWARN << "Receiver exception" << tx.what();
      } catch (...) {
          LOGWARN << "Uncaught exception ";
      }
      LOGDEBUG << "run ended..";
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

      LOGDEBUG << __FUNCTION__ << " sid: " << session_id_;
  }
 protected:
  boost::shared_ptr<fds::net::Socket> socket_;
  boost::shared_ptr<TTransport> transport_;
  boost::shared_ptr<TProtocol> protocol_;
  std::string session_id_;

  /* Thrift generated client interface */
  boost::shared_ptr<ReqClientT> req_client_;

  /* Thrift generated processor interface */
  boost::shared_ptr<RespProcessorT> resp_processor_;

  /* Response handler */
  boost::shared_ptr<RespHandlerT> resp_handler_;

  /* Responses to client requests are serviced on this thread */
  boost::shared_ptr<boost::thread> recv_thread_;
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
typedef netClientSessionEx<FDSP_MetaSyncReqClient,
        FDSP_MetaSyncRespProcessor, FDSP_MetaSyncRespIf> netMetaSyncClientSession;

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
      handler_(handler),
      lock_("netServerSession lock")
    {
        /* Keeping these local here.  Expose them if needed */
        boost::shared_ptr<TServerTransport> serverTransport;
        boost::shared_ptr<TTransportFactory> transportFactory;
        boost::shared_ptr<TProtocolFactory> protocolFactory;
//        boost::shared_ptr<PosixThreadFactory> threadFactory;
//        boost::shared_ptr<ThreadManager> threadManager;
        boost::shared_ptr<TProcessor> processor;

        serverTransport.reset(new apache::thrift::transport::TServerSocket(port));
        transportFactory.reset(getTransportFactory());
        protocolFactory.reset( new TBinaryProtocolFactory());

        processor.reset(new ReqProcessorT(handler_));
        processor->setEventHandler(
                boost::shared_ptr<FdsTProcessorEventHandler>(new FdsTProcessorEventHandler()));

        event_handler_.reset(new ServerEventHandler(*this));

//        threadManager = ThreadManager::newSimpleThreadManager(num_threads);
//        threadFactory = boost::shared_ptr<PosixThreadFactory>(new PosixThreadFactory());
//        threadManager->threadFactory(threadFactory);
//        threadManager->start();

        server_.reset(new TThreadedServer(processor,
                serverTransport, transportFactory, protocolFactory));
//        server_.reset(new TThreadPoolServer (processor,
//                                             serverTransport,
//                                             transportFactory,
//                                             protocolFactory,
//                                             threadManager));
        server_->setServerEventHandler(event_handler_);
    }

  virtual ~netServerSessionEx()
  {
      endSession();
  }

  virtual void endSession()
  {

      std::set<TTransportPtr, LtTransports> temp_connections;
      {
          /*
           * NOTE:  Below we will invoke close connection on all connections
           * in connections_ member which invokes deleteContext which
           * removes the connection from connections_.  To not cause a deadlock
           * we will copy connection_ and invoke connection->close() without holding
           * lock
           */
          fds_mutex::scoped_lock l(lock_);
          temp_connections = connections_;
      }
      server_->stop();

      /* Close all the connections. */
      for (auto itr : temp_connections) {
          itr->close();
      }

      /* Wait a while for connections to close.  If we don't wait deleteContext may
       * get invoked after netServerSessionEx object is destroyed.  If we must
       * remove the wait below then ensure to destruct netServerSessionEx object
       * after thrift server listen thread (in the blocking listen case it's
       * the thread that invoked netServerSessionEx::listenServer) has joined
       */
      int slept_time = 0;
      while (connections_.size() > 0 &&
             slept_time < 1000) {
          usleep(10);
          slept_time += 10;
      }
      // TODO(vy): this assertion is wrong fds_assert(connections_.size() == 0);

      if (listen_thread_) {
          listen_thread_->join();
      }
  }

  void listenServer()
  {
      try {
          LOGNORMAL << "Starting the server...";
          server_->serve();
      } catch(const att::TTransportException& e) {
          LOGERROR << "unable to start server : " << e.what();
          fds_panic("Unable to start a server...bailing out");
      }
  }

  /**
   * Runs listening on a separate thread
   */
  void listenServerNb()
  {
      std::cerr << "Starting the server on a separate thread..." << std::endl;
      listen_thread_.reset(new boost::thread(&TThreadedServer::serve, server_.get()));
  }

  /**
   * Associates transport t with response client
   * NOTE: This is invoked under lock
   */
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
     * NOTE: This is invoked under a lock
     */
    virtual void EstablishSession(FDSP_SessionReqResp& _return,
                                  const FDSP_MsgHdrType& fdsp_msg) override
    {
        /* Generate a uuid and add a session.  Any further authentication can be
         * done here
         */
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

 /* Add the new connection */
          TTransportPtr transport = input->getTransport();
          parent_.connections_.insert(transport);

          LOGDEBUG << "New connection request: " << getTransportKey(transport);

          /* Process the connection request */
          bool ret = false;
          try {
              boost::shared_ptr<FDSP_ServiceIf> iface(new FDSP_ConnectHandler(parent_, transport));
              FDSP_ServiceProcessor conn_processor(iface);
              ret = conn_processor.process(input, output, NULL);
          } catch(const att::TTransportException& e) {
              LOGERROR << "error during network call : " << e.what();
          }

          if (!ret) {
              LOGWARN << "Processing incoming connection request failed."
                "Closing the connection: " << getTransportKey(transport);
              /* NOTE: not calling deleteContext explicitly here.  It will be
               * invoked by the thrift server's Task::run()
               */
              transport->close();
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
          parent_.connections_.erase(input->getTransport());

          LOGDEBUG << "Removing connection: " << getTransportKey(input->getTransport());
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
      return new apache::thrift::transport::TFramedTransportFactory();
  }

  static std::string getTransportKey(TTransportPtr transport)
  {
      /* What we get is TFramedTransport.  We will extract TSocket from it */
      boost::shared_ptr<apache::thrift::transport::TFramedTransport> buf_transport =
          boost::static_pointer_cast<apache::thrift::transport::TFramedTransport>(transport);

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

  /**
   * Functor for comparing buffered transports
   */
  struct LtTransports {
      bool operator() (const TTransportPtr &t1, const TTransportPtr &t2) {
          return t1.get() < t2.get();
      }
  };

 protected:
  boost::shared_ptr<TThreadedServer> server_;
  boost::shared_ptr<ReqHandlerT> handler_;

  /* Lock to protect auth_pending_transports and respClients */
  fds_mutex lock_;

  /* Connected transports.  We assume the transports are buffered transports */
  std::set<TTransportPtr, LtTransports> connections_;

  std::unordered_map<std::string, boost::shared_ptr<RespClientT> > respClients_;

  /* TServer event handler */
  boost::shared_ptr<ServerEventHandler> event_handler_;

  /* For running the server listen on a seperate thread.  This way we don't
   * block the caller.
   */
  boost::shared_ptr<boost::thread> listen_thread_;
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
typedef netServerSessionEx<FDSP_MetaSyncReqProcessor,
        FDSP_MetaSyncReqIf, FDSP_MetaSyncRespClient> netMetaSyncServerSession;


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
    static string ipAddr2String(int ipaddr);
    static int ipString2Addr(string ipaddr_str);

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
       num_threads(_num_threads)
     {
        sessionTblMutex = new fds_mutex("RPC Tbl mutex");
     }

    netSessionTbl(FDSP_MgrIdType myMgrId)
    : netSessionTbl("", 0, 0, 50, myMgrId) {
    }
    virtual ~netSessionTbl();

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
    ClientSessionT* startSession(const std::string& dst_ip,
            int port, FDSP_MgrIdType remote_mgr_id,
            int num_channels, boost::shared_ptr<RespHandlerT>respHandler)
    {
        ClientSessionT* session = new ClientSessionT(dst_ip, port,
                localMgrId, remote_mgr_id, respHandler);
        if (session == nullptr) {
            return nullptr;
        }
        bool ret = session->start();
        if (!ret) {
            delete session;
            LOGWARN << "Failed to start session";
            return nullptr;
        }

        std::string key = getClientSessionKey(dst_ip, port);
        session->setSessionTblKey(key);

        sessionTblMutex->lock();
        sessionTbl[key] = session;
        sessionTblMutex->unlock();

        return session;
    }

    // TODO: Make this interface return shared_ptr
    template <class ServerSessionT, class ReqHandlerT>
    ServerSessionT* createServerSession(int local_ipaddr,
            int _port,
            std::string local_node_name,
            FDSP_MgrIdType remote_mgr_id,
            boost::shared_ptr<ReqHandlerT> reqHandler)
    {
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
        std::string key = getServerSessionKey(local_node_name, port);
        session->setSessionTblKey(key);

        sessionTblMutex->lock();
        sessionTbl[key] = session;
        sessionTblMutex->unlock();

        return session;
    }

    template <class SessionT>
    SessionT* getClientSession(const int &ip_addr, const int &port)
    {
        std::string ip_str = ipAddr2String(ip_addr);
        return getClientSession<SessionT>(ip_str, port);
    }

    template <class SessionT>
    SessionT* getClientSession(const std::string &ip, const int &port)
    {
        std::string key = getClientSessionKey(ip, port);
        return static_cast<SessionT*>(getSession(key));
    }

    bool clientSessionExists(const int &ip, const int &port);

    template <class SessionT>
    SessionT* getServerSession(const std::string &ip, const int &port)
    {
        std::string key = getServerSessionKey(ip, port);
        return static_cast<SessionT>(getSession(key));
    }

    void endClientSession(const int  &ip, const int &port);

    void endSession(const std::string& key);
    bool endSession(const netSession* session);

    /* Ends all client and server sessions in this table */
    void endAllSessions();

    // Blocking call equivalent to .run or .serve
    void listenServer(netSession* server_session);

    virtual std::string log_string() {
        return "NetSessionTable";
    }

 private: /* data */
    std::string getServerSessionKey(const std::string &ip, const int &port);
    std::string getClientSessionKey(const std::string &ip, const int &port);
    netSession* getSession(const std::string& key);

    std::string src_node_name;
    int src_ipaddr;
    int port;
    FDSP_MgrIdType localMgrId;
    std::unordered_map<std::string, netSession*> sessionTbl;
    fds_mutex   *sessionTblMutex;

    int num_threads;

};

typedef boost::shared_ptr<netSessionTbl> netSessionTblPtr;

#define METADATA_SESSION(session,tbl,ip,port) \
    netMetaDataPathClientSession *session = tbl->getClientSession<netMetaDataPathClientSession>(ip, port); \
    fds_verify(session != NULL);
#endif
