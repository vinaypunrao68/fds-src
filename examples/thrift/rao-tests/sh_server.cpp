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

#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <vector>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TThreadedServer.h>
#include "gen-cpp/sh_service.h"
#include "gen-cpp/sm_service.h"

using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::server;
using namespace apache::thrift::concurrency;


using namespace Sh;
using namespace Sm;
using namespace boost;

//class MyThread : public Runnable {
//public:
//  virtual void run() {
//    boost::shared_ptr<TTransport> socket(new TSocket("localhost", 9090));
//    boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
//    boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
//    CalculatorClient client(protocol);
//
//    try {
//      transport->open();
//      int res = client.add(1, 2);
//      transport->close();
//    } catch (TException &tx) {
//      printf("ERROR: %s\n", tx.what());
//    }
//  }
//};
//
//void do_concurrent_reqs(int t_cnt) {
//  clock_t t_start = clock();
//
//  std::vector< shared_ptr<Thread> > threads;
//  for (int i = 0; i < t_cnt; i++) {
//    PosixThreadFactory tFactory(PosixThreadFactory::ROUND_ROBIN, PosixThreadFactory::NORMAL, 1, false);
//    boost::shared_ptr<Runnable> r1(new MyThread());
//    shared_ptr<Thread> t1 = tFactory.newThread(r1);
//    threads.push_back(t1);
//    t1->start();
//  }
//
//  for (int i = 0; i < t_cnt; i++) {
//    threads[i]->join();
//  }
//
//  clock_t t_end = clock();
//  printf("elapsed: %d\n", t_end - t_start);
//}
//
//
//void do_serial_single_transport_reqs(int r_cnt) {
//  clock_t t_start = clock();
//  boost::shared_ptr<TTransport> socket(new TSocket("localhost", 9090));
//  boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
//  boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
//  CalculatorClient client(protocol);
//
//  try {
//    transport->open();
//    for (int i = 0; i < r_cnt; i++) {
//      int res = client.add(1, 2);
//    }
//    transport->close();
//  } catch (TException &tx) {
//    printf("ERROR: %s\n", tx.what());
//  }
//  clock_t t_end = clock();
//  printf("elapsed: %d\n", t_end - t_start);
//
//}

class SHHandler : public sh_serviceIf {
 public:
  SHHandler() {
  }
  ~SHHandler() {

  }

  virtual void put_object_done(const std::string& obj) {
    printf("Yay! receieved: %s\n", obj.c_str());
  }

 private:
};

class SHServer : public Runnable {
public:
  SHServer() {
    /* SH RPC handler */
    handler_.reset(new SHHandler());

    /* server config */
    boost::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());
    boost::shared_ptr<TProcessor> processor(new sh_serviceProcessor(handler_));
    boost::shared_ptr<TServerTransport> serverTransport(new TServerSocket(9090));
    boost::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
    server_.reset(new TThreadedServer(processor, serverTransport, transportFactory, protocolFactory));

    /* SM client information */
    boost::shared_ptr<TTransport> sm_socket(new TSocket("localhost", 9091));
    boost::shared_ptr<TBufferedTransport> sm_transport(new TBufferedTransport(sm_socket));
    boost::shared_ptr<TProtocol> sm_protocol(new TBinaryProtocol(sm_transport));
    sm_client_.reset(new sm_serviceClient(sm_protocol));
  }

  virtual void run() {
    printf("Starting the SH server...\n");
    server_->serve();
    printf("done.\n");
  }

  boost::shared_ptr<sm_serviceClient> get_sm_client() {
    return sm_client_;
  }

private:
  boost::shared_ptr<TThreadedServer> server_;
  boost::shared_ptr<SHHandler> handler_;
  boost::shared_ptr<sm_serviceClient> sm_client_;
};

int main(int argc, char** argv) {

  PosixThreadFactory tFactory(PosixThreadFactory::ROUND_ROBIN, PosixThreadFactory::NORMAL, 1, false);
  boost::shared_ptr<SHServer> sh_server(new SHServer());
  shared_ptr<Thread> t1 = tFactory.newThread(sh_server);
  t1->start();
  usleep(2000000);

  /* invoke a sm method */
  sh_server->get_sm_client()->getOutputProtocol()->getTransport()->open();
  sh_server->get_sm_client()->put_object("obj1");

  t1->join();
  return 0;
}

