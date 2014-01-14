/*
 * Simple async req/resp on the same socket
 * -- server side (SM)
 */

#include "DataPathReq.h"
#include "DataPathResp.h"

#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TTransportUtils.h>

#include <stdexcept>
#include <sstream>
#include <iostream>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;
using namespace ::apache::thrift::concurrency;

using boost::shared_ptr;

using namespace fds;
using namespace boost;
using namespace std;

typedef void (*set_client_t)(boost::shared_ptr<TTransport> transport, void* context);

class netSessionServer;
class DataPathReqHandler : virtual public DataPathReqIf {
 public:
  DataPathReqHandler() : pSession(NULL)
  {
    // Your initialization goes here
  }

  void setSessionServer(netSessionServer* sserv) {
    pSession = sserv;
  }

  void PutObject(const PutObjMessage& req); // implemetation is below in this file

public:
  netSessionServer* pSession; /* does not own */
};

/* new class is implemented so that we can create DataPathRespClient in DataPathReqHandler
 * when getProcessor is called, the transport is open and at this point we can create
 * DataPathRespClient */
class FdsDataPathReqProcessorFactory: public TProcessorFactory {
public:
  FdsDataPathReqProcessorFactory(const boost::shared_ptr<DataPathReqIfFactory> &handlerFactory,
				 set_client_t set_client_cb, void* context)
    : handler_factory(handlerFactory), set_client_hdlr(set_client_cb), context_(context) {}

  boost::shared_ptr<TProcessor> getProcessor(const TConnectionInfo& connInfo) 
  {
    set_client_hdlr(connInfo.transport, context_);

    ReleaseHandler<DataPathReqIfFactory> cleanup(handler_factory);
    boost::shared_ptr<DataPathReqIf> handler(handler_factory->getHandler(connInfo), cleanup);
    boost::shared_ptr<TProcessor> processor(new DataPathReqProcessor(handler));
    return processor;
  }

protected:
  boost::shared_ptr<DataPathReqIfFactory> handler_factory;

private:
  set_client_t set_client_hdlr;
  void* context_;
};

class netSessionServer {
  public:
    netSessionServer(const boost::shared_ptr<DataPathReqIf>& iface)
            : iface_(iface) {
      int port = 9091;
      boost::shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
      boost::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
      boost::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());
      
      boost::shared_ptr<DataPathReqIfSingletonFactory> handlerFactory(new DataPathReqIfSingletonFactory(iface_));
      boost::shared_ptr<TProcessorFactory> processorFactory(
							    new FdsDataPathReqProcessorFactory(handlerFactory, setClient, this));
      
      boost::shared_ptr<ThreadManager> threadManager = ThreadManager::newSimpleThreadManager(2);
      boost::shared_ptr<PosixThreadFactory> threadFactory = 
	boost::shared_ptr<PosixThreadFactory>(new PosixThreadFactory());
      threadManager->threadFactory(threadFactory);
      threadManager->start();
      
      server_.reset(new TThreadPoolServer(processorFactory, 
					  serverTransport,
					  transportFactory, 
					  protocolFactory, 
					  threadManager));
    }

  void run() {
    printf("Starting the server...\n");
    server_->serve();
  }

  static void setClient(boost::shared_ptr<TTransport> transport, void* context) {
    printf("netSessionServer: set DataPathRespClient\n");
    netSessionServer* self = reinterpret_cast<netSessionServer*>(context);
    self->setClientInternal(transport);
  }

  void setClientInternal(boost::shared_ptr<TTransport> transport) {
    printf("netSessionServer internal: set DataPathRespClient\n");
    protocol_.reset(new TBinaryProtocol(transport));
    client.reset(new DataPathRespClient(protocol_));
  }

  boost::shared_ptr<DataPathRespClient> getClient() {
    return client;
  }

  private:
  boost::shared_ptr<DataPathReqIf> iface_;
  boost::shared_ptr<TThreadPoolServer> server_;

  boost::shared_ptr<TProtocol> protocol_;
  boost::shared_ptr<DataPathRespClient> client;
};


void DataPathReqHandler::PutObject(const PutObjMessage& req) {
    // Your implementation goes here
    printf("Received PutObject: %s\n", req.data_obj.c_str());
    if (req.data_obj_len == 0) {
      throw TException("Data length is 0");
    }

    // send reply back
    if (pSession) {
      printf("Responding to PutObject request: \"%s\"\n", req.data_obj.c_str());
      PutObjMessage resp;
      resp.data_obj = req.data_obj;
      resp.data_obj_len = resp.data_obj.length();
      pSession->getClient()->PutObjectResp(resp);
    }
}



int main(int argc, char **argv) {

  boost::shared_ptr<DataPathReqHandler> handler(new DataPathReqHandler());
  boost::shared_ptr<netSessionServer> server(new netSessionServer(handler));

  server->run();
  printf("done.\n");
  return 0;
}



