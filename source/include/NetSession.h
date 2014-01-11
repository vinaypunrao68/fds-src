#ifndef __Net_Session_h__
#define __Net_Session_h__
#include <fdsp/FDSP_DataPathReq.h>
#include <fdsp/FDSP_DataPathResp.h>
#include <fdsp/FDSP_MetaDataPathReq.h>
#include <fdsp/FDSP_MetaDataPathResp.h>
#include <concurrency/Mutex.h>
#include <fds_types.h>
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TTransportUtils.h>
#include <NetSessRespClient.h>
#include <NetSessRespSvr.h>


using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;
using namespace ::apache::thrift::concurrency;

using namespace FDS_ProtocolInterface;
using namespace std;
using namespace fds;

typedef void  (*sessionErrorCallback)(string ip_addr, 
                                      FDSP_MgrIdType mgrId, 
                                      int channel, int errno, 
                                      std::string errMsg); 


class netClientSession : public netSession { 

public:
        boost::shared_ptr<TTransport> socket;
        boost::shared_ptr<TTransport> transport;
        boost::shared_ptr<TProtocol> protocol;
        boost::shared_ptr<TThreadPoolServer> server;

        netClientSession(string ip_addr_str, int port, FDSP_MgrIdType local_mgr,
                         FDSP_MgrIdrType remote_mgr) : netSession(node_name, port, local_mgr, remote_mgr) {
            boost::shared_ptr<TTransport> socket = new TSocket(ip_addr_str, port);
            boost::shared_ptr<TTransport> transport = new TBufferedTransport(socket);
            boost::shared_ptr<TProtocol> protocol = new TBinaryProtocol(transport);
        }

        ~netClientSession() {
           delete protocol;
           delete transport;
           delete socket;
        }
};

class netDataPathClientSession : public netClientSession { 
public :
	FDSP_DataPathReq  fdspDPAPI;
        int num_threads;
        boost::shared_ptr<FDSP_DataPathResp> fdspDataPathResp;
        boost::shared_ptr<Thread> th;
        boost::shared_ptr<fdspDataPathRespReceiver> msg_recv; 
        shared_ptr<TProcessor> processor;

        netDataPathClientSession(string ip_addr_str, 
                                 int port, int num_threads) 
                                 : netClientSession(ip_addr_str, port, FDSP_STOR_HVISOR, FDSP_STOR_MGR)  { 
           fdspDPAPI = new FDSP_DataPathReq;
           fdspDataPathResp = new FDSP_DataPathResp;
            shared_ptr<TProcessor> processor = new FDSP_DataPathRespProcessor(fdspDataPathResp);
            PosixThreadFactory threadFactory(PosixThreadFactory::ROUND_ROBIN,
                                   PosixThreadFactory::NORMAL,
                                   num_threads,
                                   false);
            msg_recv = new fdspDataPathRespReceiver(protocol, fdspDataPathResp);
            th = threadFactory.newThread(msg_recv);
            th->start();
           transport->open();
        }
        ~netDataPathClientSession() {
           transport->close();
        }
};

class netMetaDataPathClientSession : public netClientSession { 
public:
	FDSP_MetaDataPathReq  fdspMetaDataReq;
        FDSP_MetaDataPathResp fdspMetaDataPathResp;
        boost::shared_ptr<Thread> th;
        boost::shared_ptr<fdspMetaDataPathRespReceiver> msg_recv; 
        shared_ptr<TProcessor> processor;
        netMetaDataPathClientSession(string ip_addr_str, int port) : 
                                    netClientSession(ip_addr_str, 
                                                     port, 
                                                     FDSP_STOR_HVISOR, 
                                                     FDSP_DATA_MGR) {
           fdspMetaDataPathReq = new FDSP_MetaDataPathReq;
           fdspMetaDataPathResp = new FDSP_MetaDataPathResp;
            processor = new FDSP_MetaDataPathRespProcessor(fdspMetaDataPathResp);
            PosixThreadFactory threadFactory(PosixThreadFactory::ROUND_ROBIN,
                                   PosixThreadFactory::NORMAL,
                                   num_threads,
                                   false);
            msg_recv = new fdspMetaDataPathRespReceiver(protocol, fdspMetaDataPathResp);
            th = threadFactory.newThread(msg_recv);
            th->start();
            transport->open();
        }
        ~netMetaDataPathClientSession();
};

class netControlPathClientSession : public netClientSession { 
public:
	FDSP_ControlPathReq  fdspControlPathReq;
        FDSP_ControlPathResp fdspControlPathResp;
        boost::shared_ptr<Thread> th;
        boost::shared_ptr<fdspControlPathRespReceiver> msg_recv; 
        shared_ptr<TProcessor> processor;
        netControlPathClientSession(string ip_addr_str, int port) :
                                    netClientSession(ip_addr_str, 
                                                     port, 
                                                     FDSP_ORCH_MGR, 
                                                     FDSP_DATA_MGR) {
           fdspControlPathReq = new FDSP_ControlPathReq;
           fdspControlPathResp = new FDSP_ControlPathResp;
           processor = new FDSP_ControlPathRespProcessor(fdspControlDataPathResp);
           PosixThreadFactory threadFactory(PosixThreadFactory::ROUND_ROBIN,
                                   PosixThreadFactory::NORMAL,
                                   num_threads,
                                   false);
           msg_recv = new fdspControlDataPathRespReceiver(protocol, fdspControlDataPathResp);
           th = threadFactory.newThread(msg_recv);
           th->start();
           transport->open();
        }
        ~netControlPathClientSession() {
           transport->close();
        }
};

class netConfigPathClientSession : public netClientSession { 
public:
	FDSP_ConfigPathReq  fdspConfigPathReq;
        FDSP_ConfigPathResp fdspConfigPathResp;
        boost::shared_ptr<Thread> th;
        boost::shared_ptr<fdspConfigPathRespReceiver> msg_recv; 
        shared_ptr<TProcessor> processor;

        netConfigPathClientSession() : 
                                    netClientSession(ip_addr_str, 
                                                     port, 
                                                     FDSP_CLI_MGR, 
                                                     FDSP_ORCH_MGR) {
           fdspConfigPathReq = new FDSP_ConfigPathReq;
           fdspConfigPathResp = new FDSP_ConfigPathResp;
           processor = new FDSP_ConfigPathRespProcessor(fdspConfigPathResp);
           PosixThreadFactory threadFactory(PosixThreadFactory::ROUND_ROBIN,
                                   PosixThreadFactory::NORMAL,
                                   num_threads,
                                   false);
           msg_recv = new fdspConfigPathRespReceiver(protocol, fdspConfigPathResp);
           th = threadFactory.newThread(msg_recv);
           th->start();
           transport->open();
        }
        ~netConfigPathClientSession() {
           transport->close();
        }
};


class netServerSession: public netSession { 
public :
  boost::shared_ptr<TServerTransport> serverTransport;
  boost::shared_ptr<TTransportFactory> transportFactory;
  boost::shared_ptr<TProtocolFactory> protocolFactory;
  boost::shared_ptr<ThreadManager> threadManager;
  boost::shared_ptr<PosixThreadFactory> threadFactory;

  netServerSession(int num_threads) { 
       serverTransport  = new TServerSocket(port);
       transportFactory = new TBufferedTransportFactory();
       protocolFactory = new TBinaryProtocolFactory();


       threadManager = ThreadManager::newSimpleThreadManager(num_threads);
       boost::shared_ptr<PosixThreadFactory> threadFactory =
       boost::shared_ptr<PosixThreadFactory>(new PosixThreadFactory());
       threadManager->threadFactory(threadFactory);
       threadManager->start();
  }

  ~netServerSession() {
  }
};

class netDataPathServerSession : public netServerSession { 
public:
	FDSP_DataPathReq  fdspDPAPI;
        FDSP_DataPathResp fdspDataPathResp;
        boost::shared_ptr<FDSP_DataPathReq> handler;
        boost::shared_ptr<FDSP_DataPathReqIfSingletonFactory> handlerFactory; 
        boost::shared_ptr<TProcessorFactory> processorFactory;
        TThreadPoolServer *server;

        netDataPathServerSession() {
          handler = new FDSP_DataPathReqHandler();
          handlerFactory = (new FDSP_DataPathReqIfSingletonFactory(handler));
          processorFactory = (new FdsDataPathReqProcessorFactory(handlerFactory));
        }
        void listenServer() { 

          server = new TThreadPoolServer (processorFactory,
                                          serverTransport,
                                          transportFactory,
                                          protocolFactory,
                                          threadManager);

          printf("Starting the server...\n");
          server.serve();
        }

        ~netDataPathServerSession() {
        }

};

class netMetaDataPathServerSession : public netServerSession { 
public:
	FDSP_MetaDataPathReq  fdspMetaDataReq;
        FDSP_MetaDataPathResp fdspMetaDataPathResp;
        boost::shared_ptr<FDSP_MetaDataPathReq> handler;
        boost::shared_ptr<FDSP_MetaDataPathReqIfSingletonFactory> handlerFactory; 
        boost::shared_ptr<TProcessorFactory> processorFactory;
        TThreadPoolServer *server;

        netMetaDataServerSession() {
          handler = new FDSP_MetaDataPathReqHandler();
          handlerFactory = (new FDSP_MetaDataPathReqIfSingletonFactory(handler));
          processorFactory = (new FdsMetaDataPathReqProcessorFactory(handlerFactory));
        }

        void listenServer() { 

          server = new TThreadPoolServer (processorFactory,
                                          serverTransport,
                                          transportFactory,
                                          protocolFactory,
                                          threadManager);

          printf("Starting the server...\n");
          server.serve();
        }

        ~netMetaDataServerSession() {
        }

};

class netControlPathServerSession : public netServerSession { 
public:
	FDSP_ControlPathReq  fdspControlPathReq;
        FDSP_ControlPathResp fdspControlPathResp;
        boost::shared_ptr<FDSP_ControlPathReq> handler;
        boost::shared_ptr<FDSP_ControlPathReqIfSingletonFactory> handlerFactory; 
        boost::shared_ptr<TProcessorFactory> processorFactory;
        TThreadPoolServer *server;

        netControlPathServerSession() {
          handler = new FDSP_ControlPathReqHandler();
          handlerFactory = (new FDSP_ControlPathReqIfSingletonFactory(handler));
          processorFactory = (new FdsControlPathReqProcessorFactory(handlerFactory));
        }

        void listenServer() { 

          server = new TThreadPoolServer (processorFactory,
                                          serverTransport,
                                          transportFactory,
                                          protocolFactory,
                                          threadManager);

          printf("Starting the server...\n");
          server->serve();
        }

        ~netControlPathServerSession() {
        }

};

class netConfigPathServerSession : public netServerSession { 
public:
	FDSP_ConfigPathReq  fdspConfigPathReq;
        FDSP_ConfigPathResp fdspConfigPathResp;
        boost::shared_ptr<FDSP_ConfigPathReq> handler;
        boost::shared_ptr<FDSP_ConfigPathReqIfSingletonFactory> handlerFactory; 
        boost::shared_ptr<TProcessorFactory> processorFactory;
        TThreadPoolServer *server;
        netConfigPathServerSession() { 
          handler = new FDSP_ConfigPathReqHandler();
          handlerFactory = (new FDSP_ConfigPathReqIfSingletonFactory(handler));
          processorFactory = (new FdsConfigPathReqProcessorFactory(handlerFactory));
        }

        void listenServer() { 

          server = new TThreadPoolServer (processorFactory,
                                          serverTransport,
                                          transportFactory,
                                          protocolFactory,
                                          threadManager);

          printf("Starting the server...\n");
          server->serve();
        }

        ~netConfigPathServerSession() {
        }

};

class netSession {
public:
        netSession();
	netSession(const std::string& ip_addr_str_arg, int port, 
                         std::string node_name,
                         FDS_ProtocolInterface::FDSP_MgrIdType mgr_id);
	netSession(int  ip_addr_int, int port,
                         std::string node_name,
                         FDS_ProtocolInterface::FDSP_MgrIdType mgr_id);
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
        FDS_ProtocolInterface::FDSP_MgrIdType mgrId;
        sessionErrorCallback cback;

        netSession& operator=(const netSession& rhs) {
          if (this != &rhs) {
            node_index = rhs.node_index;
            proto_type = rhs.proto_type;
            ip_addr_str = rhs.ip_addr_str;
            ip_addr    = rhs.ip_addr;
            mgrId      = rhs.mgrId;
          }
          return *this;
        }
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

   // Server Side Local variables 

    boost::shared_ptr<ThreadManager> threadManager;
    boost::shared_ptr<PosixThreadFactory> threadFactory;

    std::unordered_map<std::string dest_name_key, netSession *>    sessionTbl;
    fds_mutex   *sessionTblMutex;

    netSession* setupClientSession(std::string dest_node_name, 
                             int port, 
                             FDS_ProtocolInterface::FDSP_MgrIdType local_mgr_id,
                             FDS_ProtocolInterface::FDSP_MgrIdType remote_mgr_id) ;

    netSession* setupServerSession(std::string dest_node_name, 
                             int port, 
                             FDS_ProtocolInterface::FDSP_MgrIdType local_mgr_id,
                             FDS_ProtocolInterface::FDSP_MgrIdType remote_mgr_id) ;

// Client Procedures
    netSession*       startSession(int  dst_ipaddr, int port, 
                                   FDSP_MgrIdType mgr_id, int num_channels);

    netSession*       startSession(std::string dst_node_name, 
                                   int port, FDSP_MgrIdType mgr_id, 
                                   int num_channels);

    void 	      endSession(int  dst_ip_addr, FDSP_MgrIdType);

    void 	      endSession(string  dst_node_name, FDSP_MgrIdType);

    void 	      endSession(netSession *);

// client side getSession
    netSession*       getSession(int dst_ip_addr, FDSP_MgrIdType mgrId);

    netSession*       getSession(string dst_node_name, FDSP_MgrIdType mgrId);

// Server side getSession
    netSession*       getServerSession(std::string dst_node_name, FDSP_MgrIdType mgrId);

    netSession*       getServerSession(std::string dst_node_name, FDSP_MgrIdType mgrId);
   
// Server Procedures
    // Create a new server session, pass in the remote_mgr_id that this service/server provides for
    netSession*       createServerSession(int  local_ipaddr, 
                                          int port, 
                                          std::string local_node_name,
                                          FDSP_MgrIdType remote_mgr_id);

// Blocking call equivalent to .run or .serve
    void              listenServer(netSession* server_session);

    void              endServerSession(netSession *server_session );
};

#endif
