/*
 * Simple async req/resp on the same socket
 * -- client side (AM)
 */

#include <stdio.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "gen-cpp/DataPathResp.h"
#include "gen-cpp/DataPathReq.h"


using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::concurrency;

using namespace fds;

class DataPathRespHandler: virtual public DataPathRespIf {
public:
  DataPathRespHandler() {
  }

  void PutObjectResp(const PutObjMessage& req) {
    printf("Received PutObjectResp for object \"%s\" \n", req.data_obj.c_str());
    if (req.data_obj_len == 0) {
      throw TException("Data length is 0");
    }
  }
};

class ResponseReceiver: public Runnable {
public:
  ResponseReceiver(boost::shared_ptr<TProtocol> prot,
		   boost::shared_ptr<DataPathRespHandler> hdlr)
    : prot_(prot),
      processor_(new DataPathRespProcessor(hdlr))
  {
  }

  void run() {
    /* wait for connection to be established */
    /* for now just busy wait */
    while (!prot_->getTransport()->isOpen()) {
      printf("ResponseReceiver: waiting for transport...\n");
      usleep(500);
    }

    try {
      for (;;) {
	if (!processor_->process(prot_, prot_, NULL) || 
	    !prot_->getTransport()->peek()) {
	  break;
	}
      }
    }
    catch (TException& tx) {
      printf("Response Receiver exception");
    }
  }


public:
  boost::shared_ptr<TProtocol> prot_;
  boost::shared_ptr<TProcessor> processor_;
};


int main(int argc, char** argv) {
  boost::shared_ptr<TTransport> socket(new TSocket("localhost", 9091));
  boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
  boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
  DataPathReqClient client(protocol);

  /* start thread that will receive responses to data requests */
  PosixThreadFactory threadFactory(PosixThreadFactory::ROUND_ROBIN, 
				   PosixThreadFactory::NORMAL,
				   1, 
				   false);
  boost::shared_ptr<DataPathRespHandler> handler(new DataPathRespHandler());
  boost::shared_ptr<ResponseReceiver> msg_recv(new ResponseReceiver(protocol, handler));
  boost::shared_ptr<Thread> th = threadFactory.newThread(msg_recv);
  th->start();

  boost::posix_time::ptime start_time;
  int total_msgs = 10;
  int done_msgs = 0;

  try {
    transport->open();

    start_time = boost::posix_time::microsec_clock::local_time();
    for (int i = 0; i < total_msgs; i++) {

      PutObjMessage msg;
      msg.data_obj = "Put object # " + std::to_string(i+1);
      msg.data_obj_len = msg.data_obj.length();
      try {
	client.PutObject(msg);
      } catch (TException &exn) {
	printf("Exception: %s\n", exn.what());
	break;
      }

      done_msgs++;
    }
    boost::posix_time::time_duration elapsed = 
      boost::posix_time::microsec_clock::local_time() - start_time;
    printf("Sent %d async messages, took %d microseconds = %.2f QPS\n", 
	   done_msgs, (int)elapsed.total_microseconds(), 
	   1000000.0 * (double)done_msgs / ((double)elapsed.total_microseconds()));

    /* wait for a bit before closing so we receive all responses */
    sleep(2);

    transport->close();
    th->join();

  } catch (TException &tx) {
    printf("ERROR: %s\n", tx.what());
    th->join();
  }

};
