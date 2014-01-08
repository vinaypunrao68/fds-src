#include "gen/putget.pb.h"
#include "include/zhelpers.hpp"
#include <string>
extern "C" {
#include <czmq.h>
}
#include "comm_utils.h"

class SMServer : public Fds::ZMQRRWorkerAsyncServer {
public:
  SMServer(zctx_t *ctx, int worker_cnt)
: ZMQRRWorkerAsyncServer(ctx, worker_cnt)
  {
  }

  virtual void handle() {
    void *handler = zsocket_new (ctx_, ZMQ_DEALER);
    zsocket_connect (handler, "inproc://backend");

    while (true) {
      //  The DEALER socket gives us the reply envelope and message
      zmsg_t *msg = zmsg_recv (handler);
      zframe_t *identity = zmsg_pop (msg);
      zframe_t *content = zmsg_pop (msg);
      //zframe_print(content, "Req: ");
      assert (content);
      zmsg_destroy (&msg);

      std::string content_str((char *)(zframe_data(content)),
          zframe_size(content));
      Fds::FdsReq req;
      std::string resp_str;
      zframe_t *resp_frame;
      Fds::FdsResp resp;
      Fds::PutObjResp *po_resp;

      req.ParseFromString(content_str);

      switch (req.type()) {
      case Fds::FdsReq_Type_PutObjReq:
        // todo: convert to zero copy
        po_resp = new Fds::PutObjResp();
        po_resp->set_obj_id("1");
        resp.set_type(Fds::FdsResp_Type_PutObjResp);
        resp.set_allocated_po_resp(po_resp);
        resp.SerializeToString(&resp_str);
        resp_frame = zframe_new(resp_str.c_str(), resp_str.size());
        //zframe_print(resp_frame, "Resp: ");

        zframe_send (&identity, handler, ZFRAME_REUSE + ZFRAME_MORE);
        zframe_send (&resp_frame, handler, 0);

        break;

      default:
        assert(!"Unknown message");
        break;
      }

      zframe_destroy (&identity);
      zframe_destroy (&content);
    }
  }
};

int main() {
  zctx_t *ctx = zctx_new ();
  //server_task(NULL);

  SMServer server(ctx, 2);
  server.start(); // serve forever

  zctx_destroy (&ctx);
  return 0;
}
