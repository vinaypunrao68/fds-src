#ifndef __Net_Session_h__
#define __Net_Session_h__
#include <concurrency/Mutex.h>
#include <stdio.h>
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
#include <fds_globals.h>
#include <NetSessRespClient.h>
#include <NetSessRespSvr.h>
#include <util/Log.h>
#include <fds_assert.h>

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

namespace fds {
class FDSP_ServiceImpl : virtual public FDSP_ServiceIf {
 public:
  FDSP_ServiceImpl(); 
  FDSP_ServiceImpl(netServerSession *srvr_session, TTransportPtr t);

  void set_server_info(netServerSession *srvr_session, TTransportPtr t);
  virtual void set_server_session(netServerSession *srvr_session);

  /**
   * @brief We get this request after socket connect.  As part of this call
   * we will create session and associate the connection with the generated
   * session id.  This connection is also used for response client
   *
   * @param _return
   * @param fdsp_msg
   */
  virtual void EstablishSession(FDSP_SessionReqResp& _return,
                                const FDSP_MsgHdrType& fdsp_msg) override;
  virtual void EstablishSession(FDSP_SessionReqResp& _return,
                                boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg) override;

 protected: 
  netServerSession *srvr_session_;
  TTransportPtr transport_;
};
}


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

class netClientSession : public netSession { 
public:
netClientSession(string node_name, int port, FDSP_MgrIdType local_mgr,
                 FDSP_MgrIdType remote_mgr) 
        : netSession(node_name, port, local_mgr, remote_mgr),
            socket(new apache::thrift::transport::TSocket(node_name, port)),
            transport(new apache::thrift::transport::TBufferedTransport(
                    boost::dynamic_pointer_cast<apache::thrift::transport::TTransport>(socket))),
            protocol(new TBinaryProtocol(transport)),
            session_id_("")
    {
    }
    
    virtual ~netClientSession() {
        if (transport->isOpen())
            transport->close();
    }

    virtual void endSession() { 
        if (transport->isOpen())
            transport->close();
    }
    
    std::string getSessionId() {
        return session_id_; 
    }
    
protected:
    /**
     * @brief Once client connects the first thing it should do is invoked
     * this method to get a session id.  All futher communication should
     * use this session id
     *
     * @param client_if
     */
    virtual void establishSession(boost::shared_ptr<FDSP_ServiceIf> client_if)
    {
        FDSP_SessionReqResp session_info; 
        FDSP_MsgHdrType fdsp_msg;

        session_info.status = 0;
        fdsp_msg.src_node_name = socket->getPeerAddress();
        fdsp_msg.src_port = socket->getPeerPort();
        
        client_if->EstablishSession(session_info, fdsp_msg);
        // TODO: based on return code the do the appropriate
        fds_verify(session_info.status == 0 && !session_info.sid.empty());
        session_id_ = session_info.sid;

        FDS_PLOG(g_fdslog) << __FUNCTION__ << " sid: " << session_id_;
    }
protected:
    boost::shared_ptr<apache::thrift::transport::TSocket> socket;
    boost::shared_ptr<TTransport> transport;
    boost::shared_ptr<TProtocol> protocol;
    boost::shared_ptr<TThreadPoolServer> server;
    std::string session_id_;
};

class netDataPathClientSession : public netClientSession { 
public :
netDataPathClientSession(const std::string& ip_addr_str, 
                         int port,
                         FDS_ProtocolInterface::FDSP_MgrIdType local_mgr_id,
                         FDS_ProtocolInterface::FDSP_MgrIdType remote_mgr_id,
                         int num_threads,
                         void *respSvrObj) 
        : netClientSession(ip_addr_str, port, local_mgr_id,remote_mgr_id),
            fdspDPAPI(new FDSP_DataPathReqClient(protocol)),
            fdspDataPathResp(reinterpret_cast<FDSP_DataPathRespIf *>(respSvrObj)),
            processor(new FDSP_DataPathRespProcessor(fdspDataPathResp))
    {
                /* The first message sent to server is a connect request.  DO
                 * NOT start receive thread before establishing a connection
                 * session*/
        transport->open();
        //MCSUPPORT:
        establishSession(boost::dynamic_pointer_cast<FDSP_ServiceIf>(fdspDPAPI));

        PosixThreadFactory threadFactory(PosixThreadFactory::ROUND_ROBIN,
                                         PosixThreadFactory::NORMAL,
                                         num_threads,
                                         false);
        msg_recv.reset(new fdspDataPathRespReceiver(protocol, fdspDataPathResp));
        recv_thread = threadFactory.newThread(msg_recv);
        recv_thread->start();
    }

    ~netDataPathClientSession() {
    }

    void endSession() { 
        if (transport->isOpen())
            transport->close();
    }

    boost::shared_ptr<FDSP_DataPathReqClient> getClient() {
        return fdspDPAPI;
    }
    
private:
    int num_threads;
    boost::shared_ptr<FDSP_DataPathReqClient> fdspDPAPI;
    
    boost::shared_ptr<FDSP_DataPathRespIf> fdspDataPathResp;
    boost::shared_ptr<Thread> recv_thread;
    boost::shared_ptr<fdspDataPathRespReceiver> msg_recv; 
    boost::shared_ptr<TProcessor> processor;
};

class netMetaDataPathClientSession : public netClientSession { 
public :
netMetaDataPathClientSession(const std::string& ip_addr_str, 
                             int port,
                             FDS_ProtocolInterface::FDSP_MgrIdType local_mgr_id,
                             FDS_ProtocolInterface::FDSP_MgrIdType remote_mgr_id,
                             int num_threads,
                             void *respSvrObj) 
        : netClientSession(ip_addr_str, port, local_mgr_id,remote_mgr_id),
            fdspMDPAPI(new FDSP_MetaDataPathReqClient(protocol)),
            fdspMetaDataPathResp(reinterpret_cast<FDSP_MetaDataPathRespIf *>(respSvrObj)),
            processor(new FDSP_MetaDataPathRespProcessor(fdspMetaDataPathResp)) {

        /* The first message sent to server is a connect request.  DO
         * NOT start receive thread before establishing a connection session */
        transport->open();
        establishSession(boost::dynamic_pointer_cast<FDSP_ServiceIf>(fdspMDPAPI));

        PosixThreadFactory threadFactory(PosixThreadFactory::ROUND_ROBIN,
                                         PosixThreadFactory::NORMAL,
                                         num_threads,
                                         false);
        msg_recv.reset(new fdspMetaDataPathRespReceiver(protocol, fdspMetaDataPathResp));
        recv_thread = threadFactory.newThread(msg_recv);
        recv_thread->start();
    }
    ~netMetaDataPathClientSession() {
    }
    boost::shared_ptr<FDSP_MetaDataPathReqClient> getClient() {
        return fdspMDPAPI;
    }
    void endSession() {
        if (transport->isOpen()) 
            transport->close();
    }
    
private:
    int num_threads;
    boost::shared_ptr<FDSP_MetaDataPathReqClient> fdspMDPAPI;
    
    boost::shared_ptr<FDSP_MetaDataPathRespIf> fdspMetaDataPathResp;
    boost::shared_ptr<Thread> recv_thread;
    boost::shared_ptr<fdspMetaDataPathRespReceiver> msg_recv; 
    boost::shared_ptr<TProcessor> processor;
};


class netControlPathClientSession : public netClientSession { 
public:
netControlPathClientSession(const std::string& ip_addr_str, 
                            int port,
                            FDS_ProtocolInterface::FDSP_MgrIdType local_mgr_id,
                            FDS_ProtocolInterface::FDSP_MgrIdType remote_mgr_id,
                            int num_threads,
                            void *respSvrObj) 
            : netClientSession(ip_addr_str, port, local_mgr_id, remote_mgr_id),
            fdspCPAPI(new FDSP_ControlPathReqClient(protocol)),
            fdspControlPathResp(reinterpret_cast<FDSP_ControlPathRespIf *>(respSvrObj)),
            processor(new FDSP_ControlPathRespProcessor(fdspControlPathResp)) {
        /* The first message sent to server is a connect request.  DO
         * NOT start receive thread before establishing a connection session */
        transport->open();
        establishSession(boost::dynamic_pointer_cast<FDSP_ServiceIf>(fdspCPAPI));

        PosixThreadFactory threadFactory(PosixThreadFactory::ROUND_ROBIN,
                                         PosixThreadFactory::NORMAL,
                                         num_threads,
                                         false);
        msg_recv.reset(new fdspControlPathRespReceiver(protocol, fdspControlPathResp));
        recv_thread = threadFactory.newThread(msg_recv);
        recv_thread->start();
    }
    ~netControlPathClientSession() {
    }
    boost::shared_ptr<FDSP_ControlPathReqClient> getClient() {
        return fdspCPAPI;
    }
    void endSession() { 
        if (transport->isOpen())
            transport->close();
    }

private:
    boost::shared_ptr<FDSP_ControlPathReqClient> fdspCPAPI;
    int num_threads;
    boost::shared_ptr<FDSP_ControlPathRespIf> fdspControlPathResp;
    boost::shared_ptr<Thread> recv_thread;
    boost::shared_ptr<fdspControlPathRespReceiver> msg_recv; 
    boost::shared_ptr<TProcessor> processor;
};

class netOMControlPathClientSession : public netClientSession { 
public:
netOMControlPathClientSession(const std::string& ip_addr_str, 
                              int port,
                              FDS_ProtocolInterface::FDSP_MgrIdType local_mgr_id,
                              FDS_ProtocolInterface::FDSP_MgrIdType remote_mgr_id,
                              int num_threads,
                              void *respSvrObj) 
        : netClientSession(ip_addr_str, port, local_mgr_id, remote_mgr_id),
            fdspOMCPAPI(new FDSP_OMControlPathReqClient(protocol)),
            fdspOMControlPathResp(reinterpret_cast<FDSP_OMControlPathRespIf *>(respSvrObj)),
            processor(new FDSP_OMControlPathRespProcessor(fdspOMControlPathResp)) {
        /* The first message sent to server is a connect request.  DO
         * NOT start receive thread before establishing a connection session */
        transport->open();
        establishSession(boost::dynamic_pointer_cast<FDSP_ServiceIf>(fdspOMCPAPI));

        PosixThreadFactory threadFactory(PosixThreadFactory::ROUND_ROBIN,
                                         PosixThreadFactory::NORMAL,
                                         num_threads,
                                         false);
        msg_recv.reset(new fdspOMControlPathRespReceiver(protocol, fdspOMControlPathResp));
        recv_thread = threadFactory.newThread(msg_recv);
        recv_thread->start();
    }
    ~netOMControlPathClientSession() {
    }
    boost::shared_ptr<FDSP_OMControlPathReqClient> getClient() {
        return fdspOMCPAPI;
    }
    void endSession() { 
        if (transport->isOpen())
            transport->close();
    }

private:
    boost::shared_ptr<FDSP_OMControlPathReqClient> fdspOMCPAPI;
    int num_threads;
    boost::shared_ptr<FDSP_OMControlPathRespIf> fdspOMControlPathResp;
    boost::shared_ptr<Thread> recv_thread;
    boost::shared_ptr<fdspOMControlPathRespReceiver> msg_recv; 
    boost::shared_ptr<TProcessor> processor;
};


/* Assumes sync config path, so respSvrObj passed to the constructor is 
 * is ignored */
class netConfigPathClientSession : public netClientSession { 
public:
netConfigPathClientSession(const std::string& ip_addr_str, 
                           int port,
                           FDS_ProtocolInterface::FDSP_MgrIdType local_mgr_id,
                           FDS_ProtocolInterface::FDSP_MgrIdType remote_mgr_id,
                           int num_threads,
                           void *respSvrObj) 
            : netClientSession(ip_addr_str, port, local_mgr_id, remote_mgr_id),
            fdspConfAPI(new FDSP_ConfigPathReqClient(protocol)) {
        
        transport->open();
    }
    ~netConfigPathClientSession() {
    }
    boost::shared_ptr<FDSP_ConfigPathReqClient> getClient() {
        return fdspConfAPI;
    }
    void endSession() { 
        if (transport->isOpen())
            transport->close();
    }
    
private:
    boost::shared_ptr<FDSP_ConfigPathReqClient> fdspConfAPI;
    int num_threads;
};


class netServerSession: public netSession { 
public :
  netServerSession(string node_name, 
                   int port,
                   FDSP_MgrIdType local_mgr_id,
                   FDSP_MgrIdType remote_mgr_id,
                   int num_threads) : 
                   netSession(node_name, port, local_mgr_id, remote_mgr_id),
                   lock_("netServerSession lock")
    { 
       serverTransport.reset(new apache::thrift::transport::TServerSocket(port));
       transportFactory.reset(getTransportFactory());
       protocolFactory.reset( new TBinaryProtocolFactory());

       threadManager = ThreadManager::newSimpleThreadManager(num_threads);
       threadFactory = boost::shared_ptr<PosixThreadFactory>(new PosixThreadFactory());
       threadManager->threadFactory(threadFactory);
       threadManager->start();
       event_handler_.reset(new ServerEventHandler(*this));
  }

  virtual ~netServerSession() {
  }

  virtual void endSession() { 
  }

  virtual int addRespClientSession(const std::string &session_id, TTransportPtr t)
  {
  }
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
      std::stringstream ret;

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


  // TODO: Either make this pure virtual or move up setClientInternal
  // impelementations, which might be cleaner but a larger change
  // NOTE:  This method invoked under lock
  virtual void setClientInternal(const std::string &session_id,
                                 TTransportPtr tranport)
  {
  }

private:
  class ServerEventHandler : public TServerEventHandler {
   public:
    ServerEventHandler(netServerSession &parent)
        : parent_(parent)
    {
    }
    /**
     * Called when a new client has connected and is about to being processing.
     */
    virtual void* createContext(boost::shared_ptr<TProtocol> input,
                                boost::shared_ptr<TProtocol> output) override {
        fds_mutex::scoped_lock l(parent_.lock_);
        TTransportPtr transport = input->getTransport();
        boost::shared_ptr<FDSP_ServiceIf> iface(new fds::FDSP_ServiceImpl(&parent_, transport));
        FDSP_ServiceProcessor conn_processor(iface);
        bool ret = conn_processor.process(input, output, NULL);
        if (!ret) {
            FDS_PLOG(g_fdslog) << "Failed to process conn request "; 
        }
#if 0
        TTransportPt transport = input->getTransport();
        std::string key = getTransportKey(transport);
        /* This key must not exist prior */
        fds_verify(parent_.auth_pending_transports_.find(key) == 
                   parent_.auth_pending_transports_.end());
        parent_.auth_pending_transports_[key] = transport; 

        FDS_PLOG(g_fdslog) << __FUNCTION__ << " key: " << key;
#endif
        return NULL;
    }
    /**
     * Called when a client has finished request-handling to delete server
     * context.
     */
    virtual void deleteContext(void* serverContext,
                               boost::shared_ptr<TProtocol>input,
                               boost::shared_ptr<TProtocol>output) {
        fds_mutex::scoped_lock l(parent_.lock_);
        /*
        TTransportPtr transport = input->getTransport();
        std::string key = getTransportKey(transport);
        /* Transport may or may not exist in auth_pending_transports_,
         * we will remove anyways 
         */
        /*
        parent_.auth_pending_transports_.erase(key);
        FDS_PLOG(g_fdslog) << __FUNCTION__ << " key: " << key;*/
        // TODO: Cleaning up session.  We dont have ip+port->sid mapping.
        // We may have to extend auth_pending_transports_ to contain this
        // info along with state information
    }
   private:
    netServerSession &parent_;
  };
protected:
  boost::shared_ptr<TServerTransport> serverTransport;
  boost::shared_ptr<TTransportFactory> transportFactory;
  boost::shared_ptr<TProtocolFactory> protocolFactory;
  boost::shared_ptr<PosixThreadFactory> threadFactory;
  boost::shared_ptr<ThreadManager> threadManager;
  
  /* Lock to protect auth_pending_transports and respClients */
  fds_mutex lock_;
  /* Transports pending authorization */
  //std::unordered_map<std::string, TTransportPtr> auth_pending_transports_;
  /* TServer event handler */
  boost::shared_ptr<ServerEventHandler> event_handler_;
};





class netDataPathServerSession : public netServerSession { 
 public:
  netDataPathServerSession(string dest_node_name,
                         int port,
                         FDSP_MgrIdType local_mgr_id, 
                         FDSP_MgrIdType remote_mgr_id,
                         int num_threads,
                         void *svrObj)
        : netServerSession(dest_node_name, port, local_mgr_id, remote_mgr_id, num_threads),
            handler(reinterpret_cast<FDSP_DataPathReqIf *>(svrObj)),
            handlerFactory(new FDSP_DataPathReqIfSingletonFactory(handler)),
            processorFactory(new FdsDataPathReqProcessorFactory(handlerFactory, setClient, this)) {
    }

  ~netDataPathServerSession() {
      server->stop();
  }
 
  /* NOTE this is called under a lock */
  virtual int addRespClientSession(const std::string &session_id, TTransportPtr t) override
  {
      // TODO:  Do checks to make sure transport and session_id are unique 
         protocol_.reset(new TBinaryProtocol(t));
        dataPathRespClient dprespcli( new FDSP_DataPathRespClient(protocol_));
        respClient[session_id] = dprespcli;
        return 0;
  }
    // Called from within thrift and the right context is passed -
    // nothing to do in the application modules of thrift
    static void setClient(const boost::shared_ptr<TTransport> transport, void* context) {
#if 0
        printf("netSessionServer: set DataPathRespClient for new client session \n");
        netDataPathServerSession* self = reinterpret_cast<netDataPathServerSession *>(context);
        self->setClientInternal(transport);
#endif
    }

    void setClientInternal(const boost::shared_ptr<TTransport> transport) {
        printf("netSessionServer internal: set DataPathRespClient\n");
        protocol_.reset(new TBinaryProtocol(transport));
        boost::shared_ptr<apache::thrift::transport::TSocket> sock =
                boost::static_pointer_cast<apache::thrift::transport::TSocket>(transport);
        // Convert any IPv4 mapped address to normal ipv4 address for the key
        std::string peer_addr = sock->getPeerAddress();
        std::string peer_address;
        int port = sock->getPeerPort();
        char *paddr = (char *)peer_addr.data();
     
        if (strncmp(peer_addr.c_str(), "::ffff:", (sizeof("::ffff:") -1)) == 0 ) {
             paddr = paddr + sizeof("::ffff:") -1;
             peer_address.append(paddr);
        } else {
             peer_address =  peer_addr; 
        }
        std::cout << "netSessionServer internal: set DataPathRespClient "  << peer_address;
        dataPathRespClient dprespcli( new FDSP_DataPathRespClient(protocol_));
        respClient[peer_address.c_str()] = dprespcli;
    }

    boost::shared_ptr<FDSP_DataPathRespClient> getRespClient(const std::string& sid) {
        // TODO: range check
        dataPathRespClient dprespcli = respClient[sid];
        return dprespcli;
    }
    
    void listenServer() {         
        server.reset(new TThreadPoolServer (processorFactory,
                                            serverTransport,
                                            transportFactory,
                                            protocolFactory,
                                            threadManager));
        // MCSUPPORT:
        server->setServerEventHandler(event_handler_);
        
        printf("Starting the server...\n");
        server->serve();
    }

protected:
  // MCSUPPORT:
  virtual void setClientInternal(const std::string &session_id,
                                 TTransportPtr transport) override
  {
        protocol_.reset(new TBinaryProtocol(transport));
        dataPathRespClient dprespcli( new FDSP_DataPathRespClient(protocol_));
        respClient[session_id] = dprespcli;
  }

private:
    boost::shared_ptr<FDSP_DataPathReqIf> handler;
    boost::shared_ptr<FDSP_DataPathReqIfSingletonFactory> handlerFactory; 
    boost::shared_ptr<TProcessorFactory> processorFactory;
    boost::shared_ptr<TThreadPoolServer> server;
    boost::shared_ptr<TProtocol> protocol_;
    typedef boost::shared_ptr<FDSP_DataPathRespClient> dataPathRespClient;
    std::unordered_map<std::string, dataPathRespClient> respClient;
};

class netMetaDataPathServerSession : public netServerSession { 
 public:
  netMetaDataPathServerSession(string dest_node_name,
                             int port,
                             FDSP_MgrIdType local_mgr_id, 
                             FDSP_MgrIdType remote_mgr_id,
                             int num_threads,
                             void *svrObj)
        : netServerSession(dest_node_name, port, local_mgr_id, remote_mgr_id, num_threads),
            handler(reinterpret_cast<FDSP_MetaDataPathReqIf *>(svrObj)),
            handlerFactory(new FDSP_MetaDataPathReqIfSingletonFactory(handler)),
            processorFactory(new FdsMetaDataPathReqProcessorFactory(handlerFactory, setClient, this)) {
    }

   ~netMetaDataPathServerSession()
   {
       server->stop();
   }

   /* NOTE this is called under a lock */
   virtual int addRespClientSession(const std::string &session_id, TTransportPtr t) override
   {
       // TODO:  Do checks to make sure transport and session_id are unique 
       protocol_.reset(new TBinaryProtocol(t));
       metaDataPathRespClient dprespcli( new FDSP_MetaDataPathRespClient(protocol_));
       respClient[session_id] = dprespcli;
       return 0;
   }

    // Called from within thrift and the right context is passed -
    // nothing to do in the application modules of thrift
    static void setClient(const boost::shared_ptr<TTransport> transport, void* context) {
        /*
        printf("netSessionServer: set MetaDataPathRespClient\n");
        netMetaDataPathServerSession* self = reinterpret_cast<netMetaDataPathServerSession *>(context);
        self->setClientInternal(transport);
        */
    }

    void setClientInternal(const boost::shared_ptr<TTransport> transport) {
        /*
        protocol_.reset(new TBinaryProtocol(transport));
        boost::shared_ptr<apache::thrift::transport::TSocket> sock =
                boost::static_pointer_cast<apache::thrift::transport::TSocket>(transport);
        string peer_addr = sock->getPeerAddress();
        string peer_address = getIPV4FromMappedAddress(peer_addr);
        printf("netSessionServer internal: setting MetaDataPathRespClient for %s\n", peer_address.c_str());
        respClient[peer_address] = (metaDataPathRespClient)new FDSP_MetaDataPathRespClient(protocol_);
        */
    }

    boost::shared_ptr<FDSP_MetaDataPathRespClient> getRespClient(const std::string& sid) {
        // TODO: range check
        metaDataPathRespClient dprespcli = respClient[sid];
        return dprespcli;
    }
    
    void listenServer() {         
        server.reset(new TThreadPoolServer (processorFactory,
                                            serverTransport,
                                            transportFactory,
                                            protocolFactory,
                                            threadManager));

        server->setServerEventHandler(event_handler_);        
        printf("Starting the server...\n");
        server->serve();
    }
 
    void endSession() { 
    }

protected:
    virtual void setClientInternal(const std::string &session_id,
                                   TTransportPtr transport) override
    {
        protocol_.reset(new TBinaryProtocol(transport));
        metaDataPathRespClient dprespcli( new FDSP_MetaDataPathRespClient(protocol_));
        respClient[session_id] = dprespcli;
    }
    
private:
    boost::shared_ptr<FDSP_MetaDataPathReqIf> handler;
    boost::shared_ptr<FDSP_MetaDataPathReqIfSingletonFactory> handlerFactory; 
    boost::shared_ptr<TProcessorFactory> processorFactory;
    boost::shared_ptr<TThreadPoolServer> server;
    boost::shared_ptr<TProtocol> protocol_;
typedef boost::shared_ptr<FDSP_MetaDataPathRespClient> metaDataPathRespClient;
    std::unordered_map<std::string, metaDataPathRespClient> respClient;
};

class netControlPathServerSession : public netServerSession { 
public:
netControlPathServerSession(const std::string& dest_node_name,
                            int port,
                            FDSP_MgrIdType local_mgr_id, 
                            FDSP_MgrIdType remote_mgr_id,
                            int num_threads,
                            void *svrObj)
        : netServerSession(dest_node_name, port, local_mgr_id, remote_mgr_id, num_threads),
            handler(reinterpret_cast<FDSP_ControlPathReqIf *>(svrObj)),
            handlerFactory(new FDSP_ControlPathReqIfSingletonFactory(handler)),
            processorFactory(new FdsControlPathReqProcessorFactory(handlerFactory, setClient, this)) { 
    }

    ~netControlPathServerSession() {
    }

    /* NOTE this is called under a lock */
    virtual int addRespClientSession(const std::string &session_id, TTransportPtr t) override
    {
        // TODO:  Do checks to make sure transport and session_id are unique 
        protocol_.reset(new TBinaryProtocol(t));
        controlPathRespClient dprespcli( new FDSP_ControlPathRespClient(protocol_));
        respClient[session_id] = dprespcli;
        return 0;
    }
 
    // Called from within thrift and the right context is passed - nothing to do in the application modules of thrift
    static void setClient(const boost::shared_ptr<TTransport> transport, void* context) {
        /*
        printf("netSessionServer: set ControlPathRespClient\n");
        netControlPathServerSession* self = reinterpret_cast<netControlPathServerSession *>(context);
        self->setClientInternal(transport);
        */
    }

    void setClientInternal(const boost::shared_ptr<TTransport> transport) {
        /*
        printf("netSessionServer internal: set DataPathRespClient\n");
        protocol_.reset(new TBinaryProtocol(transport));
        boost::shared_ptr<apache::thrift::transport::TSocket> sock =
                boost::static_pointer_cast<apache::thrift::transport::TSocket>(transport);
        string peer_addr = sock->getPeerAddress();
        respClient[peer_addr] = (controlPathRespClient)new FDSP_ControlPathRespClient(protocol_);
        */
    }

    boost::shared_ptr<FDSP_ControlPathRespClient> getRespClient(const std::string& sid) {
        // TODO: range check
        controlPathRespClient dprespcli = respClient[sid];
        return dprespcli;
    }

    void listenServer() {         
        server.reset(new TThreadPoolServer (processorFactory,
                                            serverTransport,
                                            transportFactory,
                                            protocolFactory,
                                            threadManager));
        
        server->setServerEventHandler(event_handler_);
        printf("Starting the server...\n");
        server->serve();
    }
    void endSession() { 
        server->stop();
    }

protected:
    virtual void setClientInternal(const std::string &session_id,
                                   TTransportPtr transport) override
    {
        protocol_.reset(new TBinaryProtocol(transport));
        controlPathRespClient dprespcli( new FDSP_ControlPathRespClient(protocol_));
        respClient[session_id] = dprespcli;
    }
    
private:
    boost::shared_ptr<FDSP_ControlPathReqIf> handler;
    boost::shared_ptr<FDSP_ControlPathReqIfSingletonFactory> handlerFactory; 
    boost::shared_ptr<TProcessorFactory> processorFactory;
    boost::shared_ptr<TThreadPoolServer> server;
    boost::shared_ptr<TProtocol> protocol_;
    typedef boost::shared_ptr<FDSP_ControlPathRespClient> controlPathRespClient;
    std::unordered_map<std::string, controlPathRespClient> respClient;
};

class netOMControlPathServerSession : public netServerSession { 
public:
netOMControlPathServerSession(const std::string& dest_node_name,
                              int port,
                              FDSP_MgrIdType local_mgr_id, 
                              FDSP_MgrIdType remote_mgr_id,
                              int num_threads,
                              void *svrObj)
        : netServerSession(dest_node_name, port, local_mgr_id, remote_mgr_id, num_threads),
            handler(reinterpret_cast<FDSP_OMControlPathReqIf *>(svrObj)),
            handlerFactory(new FDSP_OMControlPathReqIfSingletonFactory(handler)),
            processorFactory(new FdsOMControlPathReqProcessorFactory(handlerFactory, setClient, this)) { 
    }

    ~netOMControlPathServerSession() {
    }
 
    /* NOTE this is called under a lock */
    virtual int addRespClientSession(const std::string &session_id, TTransportPtr t) override
    {
        // TODO:  Do checks to make sure transport and session_id are unique 
        protocol_.reset(new TBinaryProtocol(t));
        omControlPathRespClient dprespcli( new FDSP_OMControlPathRespClient(protocol_));
        respClient[session_id] = dprespcli;
        return 0;
    }

    // Called from within thrift and the right context is passed - nothing to do in the application modules of thrift
    static void setClient(const boost::shared_ptr<TTransport> transport, void* context) {
        /*
        printf("netSessionServer: set OMControlPathRespClient\n");
        netOMControlPathServerSession* self = reinterpret_cast<netOMControlPathServerSession *>(context);
        self->setClientInternal(transport);
        */
    }

    void setClientInternal(const boost::shared_ptr<TTransport> transport) {
        /*
        printf("netSessionServer internal: set OMControlPathRespClient\n");
        protocol_.reset(new TBinaryProtocol(transport));
        boost::shared_ptr<apache::thrift::transport::TSocket> sock =
                boost::static_pointer_cast<apache::thrift::transport::TSocket>(transport);
        string peer_addr = sock->getPeerAddress();
        respClient[peer_addr] = (omControlPathRespClient)new FDSP_OMControlPathRespClient(protocol_);
        */
    }

    boost::shared_ptr<FDSP_OMControlPathRespClient> getRespClient(const std::string& sid) {
        // TODO: range check
        omControlPathRespClient dprespcli = respClient[sid];
        return dprespcli;
    }
    
    void listenServer() {         
        server.reset(new TThreadPoolServer (processorFactory,
                                            serverTransport,
                                            transportFactory,
                                            protocolFactory,
                                            threadManager));
        
        server->setServerEventHandler(event_handler_);
        printf("Starting the server...\n");
        server->serve();
    }

    void endSession() { 
        server->stop();
    }

protected:
    virtual void setClientInternal(const std::string &session_id,
                                   TTransportPtr transport) override
    {
        protocol_.reset(new TBinaryProtocol(transport));
        omControlPathRespClient dprespcli( new FDSP_OMControlPathRespClient(protocol_));
        respClient[session_id] = dprespcli;
    }
    
private:
    boost::shared_ptr<FDSP_OMControlPathReqIf> handler;
    boost::shared_ptr<FDSP_OMControlPathReqIfSingletonFactory> handlerFactory; 
    boost::shared_ptr<TProcessorFactory> processorFactory;
    boost::shared_ptr<TThreadPoolServer> server;
    boost::shared_ptr<TProtocol> protocol_;
    typedef boost::shared_ptr<FDSP_OMControlPathRespClient> omControlPathRespClient;
    std::unordered_map<std::string, omControlPathRespClient> respClient;
};


/* Config Path is sync, so no response client here */ 
class netConfigPathServerSession : public netServerSession { 
public:
netConfigPathServerSession(const std::string& dest_node_name,
                           int port,
                           FDSP_MgrIdType local_mgr_id, 
                           FDSP_MgrIdType remote_mgr_id,
                           int num_threads,
                           void *svrObj)
        : netServerSession(dest_node_name, port, local_mgr_id, remote_mgr_id, num_threads),
            handler(reinterpret_cast<FDSP_ConfigPathReqIf *>(svrObj)),
            handlerFactory(new FDSP_ConfigPathReqIfSingletonFactory(handler)),
            processorFactory(new FDSP_ConfigPathReqProcessorFactory(handlerFactory)) { 
    }

    ~netConfigPathServerSession() {
    }
 
    void listenServer() {         
        server.reset(new TThreadPoolServer (processorFactory,
                                            serverTransport,
                                            transportFactory,
                                            protocolFactory,
                                            threadManager));
        
        printf("Starting the server...\n");
        server->serve();
    }

    void endSession() {
        server->stop();
    }

private:
    boost::shared_ptr<FDSP_ConfigPathReqIf> handler;
    boost::shared_ptr<FDSP_ConfigPathReqIfSingletonFactory> handlerFactory; 
    boost::shared_ptr<TProcessorFactory> processorFactory;
    boost::shared_ptr<TThreadPoolServer> server;
    boost::shared_ptr<Thread> listen_thread;
};



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
            num_threads(_num_threads),
            localMgrId(myMgrId) {
                sessionTblMutex = new fds_mutex("RPC Tbl mutex");
            }
netSessionTbl(FDSP_MgrIdType myMgrId)
        : netSessionTbl("", 0, 0, 10, myMgrId) {
    }
    ~netSessionTbl();
    
    int src_ipaddr;
    std::string src_node_name;
    int port;
    FDSP_MgrIdType localMgrId;

    static string ipAddr2String(int ipaddr);
    static int ipString2Addr(string ipaddr_str);
    std::string getKey(std::string node_name, FDSP_MgrIdType remote_mgr_id);

    // Client Procedures
    /*
     * TODO: Let's not use a void *. Let's used smart pointer polymorphism.
     */
    netSession*       startSession(int  dst_ipaddr, int port, 
                                   FDSP_MgrIdType mgr_id, int num_channels, void *respSvrObj);

    netSession*       startSession(const std::string& dst_node_name,
                                   int port, FDSP_MgrIdType mgr_id, 
                                   int num_channels, void *respSvr);

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
   
// Server Procedures
    // Create a new server session, pass in the remote_mgr_id that this service/server provides for
    netSession*       createServerSession(int local_ipaddr, 
                                          int _port, 
                                          std::string local_node_name,
                                          FDSP_MgrIdType remote_mgr_id, 
                                          void *respHandlerObj);

// Blocking call equivalent to .run or .serve
    void              listenServer(netSession* server_session);

    void              endServerSession(netSession *server_session );

private:
    netSession* setupClientSession(const std::string& dest_node_name, 
                                   int port, 
                                   FDS_ProtocolInterface::FDSP_MgrIdType local_mgr_id,
                                   FDS_ProtocolInterface::FDSP_MgrIdType remote_mgr_id,
                                   void* respSvrObj);

    netSession* setupServerSession(const std::string& dest_node_name, 
                                   int port, 
                                   FDS_ProtocolInterface::FDSP_MgrIdType local_mgr_id,
                                   FDS_ProtocolInterface::FDSP_MgrIdType remote_mgr_id,
                                   void* SvrObj);

private: /* data */
    std::unordered_map<std::string, netSession*> sessionTbl;
    fds_mutex   *sessionTblMutex;

    int num_threads;

   // Server Side Local variables 
    boost::shared_ptr<ThreadManager> threadManager;
    boost::shared_ptr<PosixThreadFactory> threadFactory;
};


#endif
