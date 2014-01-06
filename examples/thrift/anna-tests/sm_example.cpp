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

class DataPathReqHandler : virtual public DataPathReqIf {
 public:
  DataPathReqHandler()
  {
    // Your initialization goes here
  }

  void setClient(boost::shared_ptr<TTransport> trans)
  {
    printf("DataPathReqHandler: set DataPathRespClient\n");
    protocol_.reset(new TBinaryProtocol(trans));
    client.reset(new DataPathRespClient(protocol_));
  }

  void PutObject(const PutObjMessage& req) {
    // Your implementation goes here
    printf("Received PutObject: %s\n", req.data_obj.c_str());
    if (req.data_obj_len == 0) {
      throw TException("Data length is 0");
    }

    // send reply back
    if (client) {
      printf("Responding to PutObject request: \"%s\"\n", req.data_obj.c_str());
      PutObjMessage resp;
      resp.data_obj = req.data_obj;
      resp.data_obj_len = resp.data_obj.length();
      client->PutObjectResp(resp);
    }
  }

public:
  boost::shared_ptr<TProtocol> protocol_;
  boost::shared_ptr<DataPathRespClient> client;
};

/* new class is implemented so that we can create DataPathRespClient in DataPathReqHandler
 * when getProcessor is called, the transport is open and at this point we can create
 * DataPathRespClient */
class FdsDataPathReqProcessorFactory: public TProcessorFactory {
public:
  FdsDataPathReqProcessorFactory(const boost::shared_ptr<DataPathReqIfFactory> &handlerFactory)
    : handler_factory(handlerFactory) {}

  boost::shared_ptr<TProcessor> getProcessor(const TConnectionInfo& connInfo) 
  {
    DataPathReqHandler* ptr = dynamic_cast<DataPathReqHandler*>(handler_factory->getHandler(connInfo));
    ptr->setClient(connInfo.transport);

    ReleaseHandler<DataPathReqIfFactory> cleanup(handler_factory);
    boost::shared_ptr<DataPathReqIf> handler(handler_factory->getHandler(connInfo), cleanup);
    boost::shared_ptr<TProcessor> processor(new DataPathReqProcessor(handler));
    return processor;
  }

protected:
  boost::shared_ptr<DataPathReqIfFactory> handler_factory;
};


int main(int argc, char **argv) {
  int port = 9091;
  boost::shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
  boost::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
  boost::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

  boost::shared_ptr<DataPathReqHandler> handler(new DataPathReqHandler());
  boost::shared_ptr<DataPathReqIfSingletonFactory> handlerFactory(new DataPathReqIfSingletonFactory(handler));
  boost::shared_ptr<TProcessorFactory> processorFactory(new FdsDataPathReqProcessorFactory(handlerFactory));

  boost::shared_ptr<ThreadManager> threadManager = ThreadManager::newSimpleThreadManager(2);
  boost::shared_ptr<PosixThreadFactory> threadFactory = 
    boost::shared_ptr<PosixThreadFactory>(new PosixThreadFactory());
  threadManager->threadFactory(threadFactory);
  threadManager->start();

  TThreadPoolServer server(processorFactory, 
			   serverTransport,
			   transportFactory, 
			   protocolFactory, 
			   threadManager);

  printf("Starting the server...\n");
  server.serve();
  printf("done.\n");
  return 0;
}

