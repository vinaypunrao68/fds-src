#include "gen/putget.pb.h"
#include "include/zhelpers.hpp"
#include <string>
extern "C" {
#include <czmq.h>
}

static void server_worker (void *args, zctx_t *ctx, void *pipe);

void *server_task (void *args)
{
    //  Frontend socket talks to clients over TCP
    zctx_t *ctx = zctx_new ();
    void *frontend = zsocket_new (ctx, ZMQ_ROUTER);
    zsocket_bind (frontend, "tcp://*:5570");

    //  Backend socket talks to workers over inproc
    void *backend = zsocket_new (ctx, ZMQ_DEALER);
    zsocket_bind (backend, "inproc://backend");

    //  Launch pool of worker threads, precise number is not critical
    int thread_nbr;
    for (thread_nbr = 0; thread_nbr < 5; thread_nbr++)
        zthread_fork (ctx, server_worker, NULL);

    //  Connect backend to frontend via a proxy
    zmq_proxy (frontend, backend, NULL);

    zctx_destroy (&ctx);
    return NULL;
}

//  Each worker will deserialize the message and send a response back
static void
server_worker (void *args, zctx_t *ctx, void *pipe)
{
    void *worker = zsocket_new (ctx, ZMQ_DEALER);
    zsocket_connect (worker, "inproc://backend");

    while (true) {
        //  The DEALER socket gives us the reply envelope and message
        zmsg_t *msg = zmsg_recv (worker);
        zframe_t *identity = zmsg_pop (msg);
        zframe_t *content = zmsg_pop (msg);
        zframe_print(content, "Req: ");
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
          zframe_print(resp_frame, "Resp: ");

          zframe_send (&identity, worker, ZFRAME_REUSE + ZFRAME_MORE);
          zframe_send (&resp_frame, worker, 0);

          break;

        default:
          break;
        }

        zframe_destroy (&identity);
        zframe_destroy (&content);
    }
}

int main() {
    server_task(NULL);
    return 0;
}
