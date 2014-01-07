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
#include <chrono>
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

// globals
static int total = 100; // total # of put reqs to send
static int total_rcvd = 0; // total # of responses for put req received
static int put_obj_size = 1 << 20; // payload size of put req
static auto start_time = std::chrono::high_resolution_clock::now();
static auto end_time = std::chrono::high_resolution_clock::now();


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
    total_rcvd++;
    if (total_rcvd == total) {
      end_time = std::chrono::high_resolution_clock::now();
      std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end_time-start_time).count() << std::endl;
    }
  }

 private:
};

class ShServerEventHandler : public TServerEventHandler {
 public:
  virtual void preServe() {
    printf("ShServerEventHandler::%s\n", __FUNCTION__);
  }
  virtual void* createContext(boost::shared_ptr<TProtocol> input,
                              boost::shared_ptr<TProtocol> output) {
    printf("ShServerEventHandler::%s\n", __FUNCTION__);
    return NULL;
  }
  virtual void deleteContext(void* serverContext,
                             boost::shared_ptr<TProtocol>input,
                             boost::shared_ptr<TProtocol>output) {
    printf("ShServerEventHandler::%s\n", __FUNCTION__);
  }
  virtual void processContext(void* serverContext,
                              boost::shared_ptr<TTransport> transport) {
    printf("ShServerEventHandler::%s\n", __FUNCTION__);
  }
};

class ShProcessorEventHandler : public TProcessorEventHandler {
 public:
  virtual void* getContext(const char* fn_name, void* serverContext) {
    printf("ShProcessorEventHandler::%s\n", __FUNCTION__);
    return NULL;
  }

  /**
   * Expected to free resources associated with a context.
   */
  virtual void freeContext(void* ctx, const char* fn_name) {
    printf("ShProcessorEventHandler::%s\n", __FUNCTION__);
  }
  virtual void preRead(void* ctx, const char* fn_name) {
    printf("ShProcessorEventHandler::%s\n", __FUNCTION__);
  }
  virtual void postRead(void* ctx, const char* fn_name, uint32_t bytes) {
    printf("ShProcessorEventHandler::%s\n", __FUNCTION__);
  }
  virtual void preWrite(void* ctx, const char* fn_name) {
    printf("ShProcessorEventHandler::%s\n", __FUNCTION__);
  }
  virtual void postWrite(void* ctx, const char* fn_name, uint32_t bytes) {
    printf("ShProcessorEventHandler::%s\n", __FUNCTION__);
  }
  virtual void asyncComplete(void* ctx, const char* fn_name) {
    printf("ShProcessorEventHandler::%s\n", __FUNCTION__);
  }
  virtual void handlerError(void* ctx, const char* fn_name) {
    printf("ShProcessorEventHandler::%s\n", __FUNCTION__);
  }
};

class SHServer : public Runnable {
public:
  SHServer() {
    /* SH RPC handler */
    handler_.reset(new SHHandler());

    /* server config */
    boost::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());
    boost::shared_ptr<TProcessor> processor(new sh_serviceProcessor(handler_));
    boost::shared_ptr<TServerTransport> serverTransport(new TServerSocket(9092));
    //boost::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
    //boost::shared_ptr<TTransportFactory> transportFactory(new TTransportFactory());
    boost::shared_ptr<TTransportFactory> transportFactory(new TFramedTransportFactory());
    server_.reset(new TThreadedServer(processor, serverTransport, transportFactory, protocolFactory));

//    boost::shared_ptr<TServerEventHandler> serverEventHandler(new ShServerEventHandler());
//    server_->setServerEventHandler(serverEventHandler);
//    boost::shared_ptr<ShProcessorEventHandler> processorEventHandler(new ShProcessorEventHandler());
//    processor->setEventHandler(processorEventHandler);

    /* SM client information */
    boost::shared_ptr<TTransport> sm_socket(new TSocket("localhost", 9091));
//    boost::shared_ptr<TBufferedTransport> sm_transport(new TBufferedTransport(sm_socket));
    boost::shared_ptr<TFramedTransport> sm_transport(new TFramedTransport(sm_socket));
    boost::shared_ptr<TProtocol> sm_protocol(new TBinaryProtocol(sm_transport));
//    boost::shared_ptr<TProtocol> sm_protocol(new TBinaryProtocol(sm_socket));
    sm_client_.reset(new sm_serviceClient(sm_protocol));
  }

  virtual void run() {
    server_->serve();
  }

  boost::shared_ptr<sm_serviceClient> get_sm_client() {
    return sm_client_;
  }

private:
  boost::shared_ptr<TThreadedServer> server_;
  boost::shared_ptr<SHHandler> handler_;
  boost::shared_ptr<sm_serviceClient> sm_client_;
};

boost::shared_ptr<sm_serviceClient> get_client(const std::string &ip , int port) {
  boost::shared_ptr<sm_serviceClient> sm_client;
  boost::shared_ptr<TTransport> sm_socket(new TSocket(ip, port));
  //    boost::shared_ptr<TBufferedTransport> sm_transport(new TBufferedTransport(sm_socket));
  boost::shared_ptr<TFramedTransport> sm_transport(new TFramedTransport(sm_socket));
  boost::shared_ptr<TProtocol> sm_protocol(new TBinaryProtocol(sm_transport));
  //    boost::shared_ptr<TProtocol> sm_protocol(new TBinaryProtocol(sm_socket));
  sm_client.reset(new sm_serviceClient(sm_protocol));
  return sm_client;
}

std::string get_put_obj(int obj_size) {
  return std::string(obj_size, 'o');
}

int main(int argc, char** argv) {

  PosixThreadFactory tFactory(PosixThreadFactory::ROUND_ROBIN, PosixThreadFactory::NORMAL, 1, false);
  boost::shared_ptr<SHServer> sh_server(new SHServer());
  boost::shared_ptr<Thread> t1 = tFactory.newThread(sh_server);
  t1->start();
  usleep(1000000);

  /* invoke a sm method */
  boost::shared_ptr<sm_serviceClient> c1 = sh_server->get_sm_client();
  c1->getOutputProtocol()->getTransport()->open();
  int cnt = 0;
  start_time = std::chrono::high_resolution_clock::now();
  while (cnt < total) {
    c1->put_object(/*std::string(put_obj_size, 'o')*/get_put_obj(put_obj_size));
    cnt++;
  }
//  usleep(1000000);
//
//  boost::shared_ptr<sm_serviceClient> c2 = get_client("localhost", 9091);
//  c2->getOutputProtocol()->getTransport()->open();
//  c2->put_object("obj2");
//  usleep(1000000);
//
//  boost::shared_ptr<sm_serviceClient> c3 = get_client("localhost", 9091);
//  c3->getOutputProtocol()->getTransport()->open();
//  c3->put_object("obj3");
//  usleep(1000000);
//
//  boost::shared_ptr<sm_serviceClient> c4 = get_client("localhost", 9091);
//  c4->getOutputProtocol()->getTransport()->open();
//  c4->put_object("obj4");
//  usleep(1000000);

  t1->join();
  return 0;
}

