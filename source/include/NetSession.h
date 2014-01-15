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
#include <NetSessRespClient.h>
#include <NetSessRespSvr.h>


using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::concurrency;

using namespace FDS_ProtocolInterface;
using namespace std;
using namespace fds;

typedef void  (*sessionErrorCallback)(string ip_addr, 
                                      FDSP_MgrIdType mgrId, 
                                      int channel, int errno, 
                                      std::string errMsg); 

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
    void setSessionErrHandler(sessionErrorCallback cback);
    
    ~netSession();
    void     endSession();
    
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

private:
    int role; // Server or Client side binding
    FDS_ProtocolInterface::FDSP_MgrIdType localMgrId;
    FDS_ProtocolInterface::FDSP_MgrIdType remoteMgrId;
};

class netClientSession : virtual public netSession { 
public:
netClientSession(string node_name, int port, FDSP_MgrIdType local_mgr,
                 FDSP_MgrIdType remote_mgr) 
        : netSession(node_name, port, local_mgr, remote_mgr),
            socket(new TSocket(node_name, port)),
            transport(new TBufferedTransport(socket)),
            protocol(new TBinaryProtocol(transport)) {
    }
    
    ~netClientSession() {
    }
    
protected:
    boost::shared_ptr<TTransport> socket;
    boost::shared_ptr<TTransport> transport;
    boost::shared_ptr<TProtocol> protocol;
    boost::shared_ptr<TThreadPoolServer> server;
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
            processor(new FDSP_DataPathRespProcessor(fdspDataPathResp)) {

        PosixThreadFactory threadFactory(PosixThreadFactory::ROUND_ROBIN,
                                         PosixThreadFactory::NORMAL,
                                         num_threads,
                                         false);
        msg_recv.reset(new fdspDataPathRespReceiver(protocol, fdspDataPathResp));
        recv_thread = threadFactory.newThread(msg_recv);
        recv_thread->start();
        transport->open();
    }
    ~netDataPathClientSession() {
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

        PosixThreadFactory threadFactory(PosixThreadFactory::ROUND_ROBIN,
                                         PosixThreadFactory::NORMAL,
                                         num_threads,
                                         false);
        msg_recv.reset(new fdspMetaDataPathRespReceiver(protocol, fdspMetaDataPathResp));
        recv_thread = threadFactory.newThread(msg_recv);
        recv_thread->start();
        transport->open();
    }
    ~netMetaDataPathClientSession() {
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

        PosixThreadFactory threadFactory(PosixThreadFactory::ROUND_ROBIN,
                                         PosixThreadFactory::NORMAL,
                                         num_threads,
                                         false);
        msg_recv.reset(new fdspControlPathRespReceiver(protocol, fdspControlPathResp));
        recv_thread = threadFactory.newThread(msg_recv);
        recv_thread->start();
        transport->open();
    }
    ~netControlPathClientSession() {
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
        
        PosixThreadFactory threadFactory(PosixThreadFactory::ROUND_ROBIN,
                                         PosixThreadFactory::NORMAL,
                                         num_threads,
                                         false);
        msg_recv.reset(new fdspOMControlPathRespReceiver(protocol, fdspOMControlPathResp));
        recv_thread = threadFactory.newThread(msg_recv);
        recv_thread->start();
        transport->open();
    }
    ~netOMControlPathClientSession() {
        transport->close();
    }
    boost::shared_ptr<FDSP_OMControlPathReqClient> getClient() {
        return fdspOMCPAPI;
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
        transport->close();
    }
    
private:
    boost::shared_ptr<FDSP_ConfigPathReqClient> fdspConfAPI;
    int num_threads;
};


class netServerSession: public netSession { 
public :
  netServerSession(string node_name, int port, FDSP_MgrIdType local_mgr_id, int num_threads) : 
                   netSession(node_name, port, local_mgr_id, local_mgr_id) { 
       serverTransport.reset(new TServerSocket(port));
       transportFactory.reset( new TBufferedTransportFactory());
       protocolFactory.reset( new TBinaryProtocolFactory());

       threadManager = ThreadManager::newSimpleThreadManager(num_threads);
       threadFactory = boost::shared_ptr<PosixThreadFactory>(new PosixThreadFactory());
       threadManager->threadFactory(threadFactory);
       threadManager->start();
  }

  ~netServerSession() {
  }

protected:
  boost::shared_ptr<TServerTransport> serverTransport;
  boost::shared_ptr<TTransportFactory> transportFactory;
  boost::shared_ptr<TProtocolFactory> protocolFactory;
  boost::shared_ptr<PosixThreadFactory> threadFactory;
  boost::shared_ptr<ThreadManager> threadManager;
};

class netDataPathServerSession : public netServerSession { 
public:
netDataPathServerSession(string dest_node_name,
                         int port,
                         FDSP_MgrIdType local_mgr_id, 
                         FDSP_MgrIdType remote_mgr_id,
                         int num_threads,
                         void *svrObj)
        : netServerSession(dest_node_name, port, local_mgr_id, num_threads),
            handler(reinterpret_cast<FDSP_DataPathReqIf *>(svrObj)),
            handlerFactory(new FDSP_DataPathReqIfSingletonFactory(handler)),
            processorFactory(new FdsDataPathReqProcessorFactory(handlerFactory, setClient, this)) {
    }
 
    // Called from within thrift and the right context is passed -
    // nothing to do in the application modules of thrift
    static void setClient(const boost::shared_ptr<TTransport> transport, void* context) {
        printf("netSessionServer: set DataPathRespClient\n");
        netDataPathServerSession* self = reinterpret_cast<netDataPathServerSession *>(context);
        self->setClientInternal(transport);
    }

    void setClientInternal(const boost::shared_ptr<TTransport> transport) {
        printf("netSessionServer internal: set DataPathRespClient\n");
        protocol_.reset(new TBinaryProtocol(transport));
        client.reset(new FDSP_DataPathRespClient(protocol_));
    }

    boost::shared_ptr<FDSP_DataPathRespClient> getClient() {
        return client;
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
    
private:
    boost::shared_ptr<FDSP_DataPathReqIf> handler;
    boost::shared_ptr<FDSP_DataPathReqIfSingletonFactory> handlerFactory; 
    boost::shared_ptr<TProcessorFactory> processorFactory;
    boost::shared_ptr<TThreadPoolServer> server;
    boost::shared_ptr<TProtocol> protocol_;
    boost::shared_ptr<FDSP_DataPathRespClient> client;
};

class netMetaDataPathServerSession : public netServerSession { 
public:
netMetaDataPathServerSession(string dest_node_name,
                             int port,
                             FDSP_MgrIdType local_mgr_id, 
                             FDSP_MgrIdType remote_mgr_id,
                             int num_threads,
                             void *svrObj)
        : netServerSession(dest_node_name, port, local_mgr_id, num_threads),
            handler(reinterpret_cast<FDSP_MetaDataPathReqIf *>(svrObj)),
            handlerFactory(new FDSP_MetaDataPathReqIfSingletonFactory(handler)),
            processorFactory(new FdsMetaDataPathReqProcessorFactory(handlerFactory, setClient, this)) {
    }
    
    // Called from within thrift and the right context is passed -
    // nothing to do in the application modules of thrift
    static void setClient(const boost::shared_ptr<TTransport> transport, void* context) {
        printf("netSessionServer: set MetaDataPathRespClient\n");
        netMetaDataPathServerSession* self = reinterpret_cast<netMetaDataPathServerSession *>(context);
        self->setClientInternal(transport);
    }

    void setClientInternal(const boost::shared_ptr<TTransport> transport) {
        printf("netSessionServer internal: set MetaDataPathRespClient\n");
        protocol_.reset(new TBinaryProtocol(transport));
        client.reset(new FDSP_MetaDataPathRespClient(protocol_));
    }

    boost::shared_ptr<FDSP_MetaDataPathRespClient> getClient() {
        return client;
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
    
private:
    boost::shared_ptr<FDSP_MetaDataPathReqIf> handler;
    boost::shared_ptr<FDSP_MetaDataPathReqIfSingletonFactory> handlerFactory; 
    boost::shared_ptr<TProcessorFactory> processorFactory;
    boost::shared_ptr<TThreadPoolServer> server;
    boost::shared_ptr<TProtocol> protocol_;
    boost::shared_ptr<FDSP_MetaDataPathRespClient> client;
};

class netControlPathServerSession : public netServerSession { 
public:
netControlPathServerSession(const std::string& dest_node_name,
                            int port,
                            FDSP_MgrIdType local_mgr_id, 
                            FDSP_MgrIdType remote_mgr_id,
                            int num_threads,
                            void *svrObj)
        : netServerSession(dest_node_name, port, local_mgr_id, num_threads),
            handler(reinterpret_cast<FDSP_ControlPathReqIf *>(svrObj)),
            handlerFactory(new FDSP_ControlPathReqIfSingletonFactory(handler)),
            processorFactory(new FdsControlPathReqProcessorFactory(handlerFactory, setClient, this)) { 
    }

    ~netControlPathServerSession() {
    }
 
    // Called from within thrift and the right context is passed - nothing to do in the application modules of thrift
    static void setClient(const boost::shared_ptr<TTransport> transport, void* context) {
        printf("netSessionServer: set ControlPathRespClient\n");
        netControlPathServerSession* self = reinterpret_cast<netControlPathServerSession *>(context);
        self->setClientInternal(transport);
    }

    void setClientInternal(const boost::shared_ptr<TTransport> transport) {
        printf("netSessionServer internal: set DataPathRespClient\n");
        protocol_.reset(new TBinaryProtocol(transport));
        client.reset(new FDSP_ControlPathRespClient(protocol_));
    }

    boost::shared_ptr<FDSP_ControlPathRespClient> getClient() {
        return client;
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
    
private:
    boost::shared_ptr<FDSP_ControlPathReqIf> handler;
    boost::shared_ptr<FDSP_ControlPathReqIfSingletonFactory> handlerFactory; 
    boost::shared_ptr<TProcessorFactory> processorFactory;
    boost::shared_ptr<TThreadPoolServer> server;
    boost::shared_ptr<TProtocol> protocol_;
    boost::shared_ptr<FDSP_ControlPathRespClient> client;
};

class netOMControlPathServerSession : public netServerSession { 
public:
netOMControlPathServerSession(const std::string& dest_node_name,
                              int port,
                              FDSP_MgrIdType local_mgr_id, 
                              FDSP_MgrIdType remote_mgr_id,
                              int num_threads,
                              void *svrObj)
        : netServerSession(dest_node_name, port, local_mgr_id, num_threads),
            handler(reinterpret_cast<FDSP_OMControlPathReqIf *>(svrObj)),
            handlerFactory(new FDSP_OMControlPathReqIfSingletonFactory(handler)),
            processorFactory(new FdsOMControlPathReqProcessorFactory(handlerFactory, setClient, this)) { 
    }

    ~netOMControlPathServerSession() {
    }
 
    // Called from within thrift and the right context is passed - nothing to do in the application modules of thrift
    static void setClient(const boost::shared_ptr<TTransport> transport, void* context) {
        printf("netSessionServer: set OMControlPathRespClient\n");
        netOMControlPathServerSession* self = reinterpret_cast<netOMControlPathServerSession *>(context);
        self->setClientInternal(transport);
    }

    void setClientInternal(const boost::shared_ptr<TTransport> transport) {
        printf("netSessionServer internal: set OMControlPathRespClient\n");
        protocol_.reset(new TBinaryProtocol(transport));
        client.reset(new FDSP_OMControlPathRespClient(protocol_));
    }

    boost::shared_ptr<FDSP_OMControlPathRespClient> getClient() {
        return client;
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
    
private:
    boost::shared_ptr<FDSP_OMControlPathReqIf> handler;
    boost::shared_ptr<FDSP_OMControlPathReqIfSingletonFactory> handlerFactory; 
    boost::shared_ptr<TProcessorFactory> processorFactory;
    boost::shared_ptr<TThreadPoolServer> server;
    boost::shared_ptr<TProtocol> protocol_;
    boost::shared_ptr<FDSP_OMControlPathRespClient> client;
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
        : netServerSession(dest_node_name, port, local_mgr_id, num_threads),
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
    
private:
    boost::shared_ptr<FDSP_ConfigPathReqIf> handler;
    boost::shared_ptr<FDSP_ConfigPathReqIfSingletonFactory> handlerFactory; 
    boost::shared_ptr<TProcessorFactory> processorFactory;
    boost::shared_ptr<TThreadPoolServer> server;
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
    ~netSessionTbl() {
    }
    
    int src_ipaddr;
    std::string src_node_name;
    int port;
    int num_threads;
    FDSP_MgrIdType localMgrId;

    static string ipAddr2String(int ipaddr);
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

   // Server Side Local variables 
    boost::shared_ptr<ThreadManager> threadManager;
    boost::shared_ptr<PosixThreadFactory> threadFactory;
};

#endif
