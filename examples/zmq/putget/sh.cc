#include <string>
#include <chrono>
extern "C" {
#include <czmq.h>
}
#include "gen/putget.pb.h"
#include "include/zhelpers.hpp"
#include "comm_utils.h"

// globals
static int total = 100; // total # of put reqs to send
static int total_rcvd = 0; // total # of responses for put req received
static int put_obj_size = 1 << 20; // payload size of put req
static auto start_time = std::chrono::high_resolution_clock::now();
static auto end_time = std::chrono::high_resolution_clock::now();

// todo: Create a generc poller class that runs on its own thread
static void*
poller_thread(void *arg) {
  zmq::socket_t *socket = static_cast<zmq::socket_t*>(arg);
  zmq::pollitem_t items [] = { {*socket, 0, ZMQ_POLLIN, 0} };

  while (1) {
    int rc = zmq::poll (&items [0], 1, -1);
    // todo: handle error
    assert(rc >= 0);
    if (items [0].revents & ZMQ_POLLIN) {
      zmq::message_t message;
      Fds::FdsResp resp;
      socket->recv(&message);
      rc = Fds::zmq_msg_to_fds_proto<Fds::FdsResp>(message, resp);
      assert(rc >= 0);
      //std::cout << resp.DebugString() << std::endl;
      total_rcvd++;
      if (total_rcvd == total) {
        end_time = std::chrono::high_resolution_clock::now();
        std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end_time-start_time).count() << std::endl;
        break;
      }
    }
  }
  return NULL;
}

Fds::PutObjReq* create_put_object(int obj_size)
{
  Fds::PutObjReq *po_req = new Fds::PutObjReq();
  po_req->set_obj_id("id");
  po_req->set_content(std::string(obj_size, 'o'));
  return po_req;
}

int main(int argc, char* argv[]) {
  // Verify that the version of the library that we linked against is
  // compatible with the version of the headers we compiled against.
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  zmq::context_t context(1);

  Fds::ZMQAsyncEndPoint sm_client(context, "tcp://localhost:5570");

  pthread_t poller;
  pthread_create (&poller, NULL, poller_thread, &sm_client.get_socket());

  /*
   * Keep sending PutReqs to sm at 1sec interval.  Note response is
   * handled on poller thread
   */
  int cnt = 0;
  start_time = std::chrono::high_resolution_clock::now();
  while (cnt < total) {
    Fds::FdsReq req;
    req.set_type(Fds::FdsReq_Type_PutObjReq);
    req.set_allocated_po_req(create_put_object(put_obj_size));

    sm_client.send_msg(req);
    cnt++;
  }

  pthread_join(poller, NULL);
  // Optional:  Delete all global objects allocated by libprotobuf.
  google::protobuf::ShutdownProtobufLibrary();

  return 0;
}
