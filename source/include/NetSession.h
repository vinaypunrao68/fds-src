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
    int role; // Server or Client side binding
    FDS_ProtocolInterface::FDSP_MgrIdType localMgrId;
    FDS_ProtocolInterface::FDSP_MgrIdType remoteMgrId;
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
netDataPathClientSession(string ip_addr_str, 
                         int port, int num_threads, void *respSvrObj) 
        : netClientSession(ip_addr_str, port, FDSP_STOR_HVISOR, FDSP_STOR_MGR),
            fdspDPAPI(new FDSP_DataPathReqClient(protocol)) {

        fdspDataPathResp.reset(reinterpret_cast<FDSP_DataPathRespIf *>(respSvrObj));
        processor.reset(new FDSP_DataPathRespProcessor(fdspDataPathResp));
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
    
private:
    int num_threads;
    boost::shared_ptr<FDSP_DataPathReqClient> fdspDPAPI;
    
    boost::shared_ptr<FDSP_DataPathRespIf> fdspDataPathResp;
    boost::shared_ptr<Thread> recv_thread;
    boost::shared_ptr<fdspDataPathRespReceiver> msg_recv; 
    boost::shared_ptr<TProcessor> processor;
};

class netControlPathClientSession : public netClientSession { 
public:
    netControlPathClientSession(string ip_addr_str, 
                                int port, int num_threads, void *respSvrObj) 
            : netClientSession(ip_addr_str, port, FDSP_STOR_HVISOR, FDSP_STOR_MGR),
            fdspCPAPI(new FDSP_ControlPathReqClient(protocol)) {
        fdspControlPathResp.reset(reinterpret_cast<FDSP_ControlPathRespIf *>(respSvrObj));
        processor.reset(new FDSP_ControlPathRespProcessor(fdspControlPathResp));
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
        boost::shared_ptr<fdspDataPathRespReceiver> msg_recv; 
        shared_ptr<TProcessor> processor;

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
        netDataPathServerSession(string dest_node_name, int port, FDSP_MgrIdType local_mgr_id, 
                                 FDSP_MgrIdType remote_mgr_id, int num_threads,
                                 void *svrObj) : netServerSession(dest_node_name, port, local_mgr_id, num_threads) {
          handler = reinterpret_cast<FDSP_DataPathReqIf *> (svrObj);
          handlerFactory.reset(new FDSP_DataPathReqIfSingletonFactory(handler));
          processorFactory.reset(new FdsDataPathReqProcessorFactory(handlerFactory, setClient, this));
        }
 
    // Called from within thrift and the right context is passed - nothing to do in the application modules of thrift
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
    FDSP_DataPathReqIf* handler;
    boost::shared_ptr<FDSP_DataPathReqIfSingletonFactory> handlerFactory; 
    boost::shared_ptr<TProcessorFactory> processorFactory;
    boost::shared_ptr<TThreadPoolServer> server;
    boost::shared_ptr<TProtocol> protocol_;
    boost::shared_ptr<FDSP_DataPathRespClient> client;
};

class netControlPathServerSession : public netServerSession { 
public:
        netControlPathServerSession(string dest_node_name, int port, FDSP_MgrIdType local_mgr_id, 
                                 FDSP_MgrIdType remote_mgr_id, int num_threads,
                                 void *svrObj) : netServerSession(dest_node_name, port, local_mgr_id, num_threads),
                                 handler(req_iface),
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


inline std::ostream& operator<<(std::ostream& out, const netSession& ep) {
  out << "Network endpoint ";
  if (ep.mgrId == FDSP_DATA_MGR) {
    out << "DM";
  } else if (ep.mgrId == FDSP_STOR_MGR) {
    out << "SM";
  } else if (ep.mgrId == FDSP_ORCH_MGR) {
    out << "OM";
  } else {
    assert(ep.mgrId == FDSP_STOR_HVISOR);
    out << "SH";
  }
  out << " with IP " << ep.ip_addr
      << " and port " << ep.port_num;
  return out;
}

class netSessionTbl {
public :
    netSessionTbl(std::string _src_node_name, int _src_ipaddr, int _port, int _num_threads) :
        src_node_name(_src_node_name), src_ipaddr(_src_ipaddr), port(_port), num_threads(_num_threads) {
    }

    netSessionTbl() {
        sessionTblMutex = new fds_mutex("RPC Tbl mutex"); 
     };
    ~netSessionTbl();
    
    int src_ipaddr;
    std::string src_node_name;
    int port;
    int num_threads;
    FDSP_MgrIdType localMgrId;
    FDSP_MgrIdType remoteMgrId;

   // Server Side Local variables 

    boost::shared_ptr<ThreadManager> threadManager;
    boost::shared_ptr<PosixThreadFactory> threadFactory;

    std::unordered_map<std::string, netSession*> sessionTbl;
    fds_mutex   *sessionTblMutex;
    static string ipAddr2String(int ipaddr);
    std::string getKey(std::string node_name, FDSP_MgrIdType remote_mgr_id);

    netSession* setupClientSession(const std::string& dest_node_name, 
                                   int port, 
                                   FDS_ProtocolInterface::FDSP_MgrIdType local_mgr_id,
                                   FDS_ProtocolInterface::FDSP_MgrIdType remote_mgr_id,
                                   void* respSvrObj);

    netSession* setupServerSession(const std::string& dest_node_name, 
                                   int port, 
                                   FDS_ProtocolInterface::FDSP_MgrIdType local_mgr_id,
                                   FDS_ProtocolInterface::FDSP_MgrIdType remote_mgr_id,
                                   void* SvrObj) ;

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

    netSession*       getSession(string dst_node_name, FDSP_MgrIdType mgrId);

// Server side getSession
    netSession*       getServerSession(int dst_ip_addr, FDSP_MgrIdType mgrId);

    netSession*       getServerSession(const std::string& dst_node_name, FDSP_MgrIdType mgrId);
   
// Server Procedures
    // Create a new server session, pass in the remote_mgr_id that this service/server provides for
    netSession*       createServerSession(int  local_ipaddr, 
                                          int port, 
                                          std::string local_node_name,
                                          FDSP_MgrIdType remote_mgr_id, 
                                          void *respHandlerObj);

// Blocking call equivalent to .run or .serve
    void              listenServer(netSession* server_session);

    void              endServerSession(netSession *server_session );
};

#endif
