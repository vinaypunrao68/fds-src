/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/transport/TSocket.h>

#include <iostream>
#include <stdexcept>
#include <sstream>

#include "gen-cpp/sh_service.h"
#include "gen-cpp/sm_service.h"

using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::server;
using namespace apache::thrift::concurrency;

using namespace Sm;
using namespace Sh;

using namespace boost;


class SMHandler : public sm_serviceIf {
 public:
  SMHandler() {
  }
  ~SMHandler() {

  }

  virtual void put_object(const std::string& objId) {
    printf("%s, objId: %s\n", __FUNCTION__, objId.c_str());

    if (!sh_transport_->isOpen()) {
      sh_transport_->open();
    }
    // doing async callback
    sh_client_->put_object_done(objId);
  }

  void set_sh_client(boost::shared_ptr<sh_serviceClient> sh_client) {
    sh_client_ = sh_client;
    sh_transport_ = sh_client->getOutputProtocol()->getTransport();
  }

 private:
  boost::shared_ptr<TTransport> sh_transport_;
  boost::shared_ptr<sh_serviceClient> sh_client_;
};

class SMServer : public Runnable {
public:
  SMServer() {
    /* SM RPC handler */
    handler_.reset(new SMHandler());

    /* server config */
    boost::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());
    boost::shared_ptr<TProcessor> processor(new sm_serviceProcessor(handler_));
    boost::shared_ptr<TServerTransport> serverTransport(new TServerSocket(9091));
    boost::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
    server_.reset(new TThreadedServer(processor, serverTransport, transportFactory, protocolFactory));

    /* SH client information */
    boost::shared_ptr<TTransport> sh_socket(new TSocket("localhost", 9090));
    boost::shared_ptr<TBufferedTransport> sh_transport(new TBufferedTransport(sh_socket));
    boost::shared_ptr<TProtocol> sh_protocol(new TBinaryProtocol(sh_transport));
    sh_client_.reset(new sh_serviceClient(sh_protocol));

    handler_->set_sh_client(sh_client_);
  }

  virtual void run() {
    printf("Starting the SM server...\n");
    server_->serve();
    printf("done.\n");
  }

  boost::shared_ptr<sh_serviceClient> get_sh_client() {
    return sh_client_;
  }

private:
  boost::shared_ptr<TThreadedServer> server_;
  boost::shared_ptr<SMHandler> handler_;
  boost::shared_ptr<sh_serviceClient> sh_client_;
};

int main(int argc, char** argv) {
  PosixThreadFactory tFactory(PosixThreadFactory::ROUND_ROBIN, PosixThreadFactory::NORMAL, 1, false);
  boost::shared_ptr<SMServer> sm_server(new SMServer());
  shared_ptr<Thread> t1 = tFactory.newThread(sm_server);
  t1->start();
  usleep(2000000);

  t1->join();
  return 0;
}
