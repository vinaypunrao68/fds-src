/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#include <unistd.h>
#include <arpa/inet.h>
#include <ProbeService.h>
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TTransportUtils.h>
#include <stdexcept>
#include <sstream>
#include <iostream>

using namespace ::apache::thrift;                 // NOLINT
using namespace ::apache::thrift::protocol;       // NOLINT
using namespace ::apache::thrift::transport;      // NOLINT
using namespace ::apache::thrift::server;         // NOLINT
using namespace ::apache::thrift::concurrency;    // NOLINT

class ProbeServiceHandler : virtual public ProbeServiceIf {
  public:
    ProbeServiceHandler() {
        // Your initialization goes here
    }

    // Don't do anything here. This stub is just to keep cpp compiler happy
    void probe_put(const ProbePutMsg& reqt) {}            // NOLINT
    void probe_get(ProbeGetMsgResp& _return,              // NOLINT
                   const ProbeGetMsgReqt& reqt) {}

    void probe_put(boost::shared_ptr<ProbePutMsg>& reqt)  // NOLINT
    {
        // We received the data to exercise sending over the network.
    }

    void probe_get(ProbeGetMsgResp& ret,                      // NOLINT
                   boost::shared_ptr<ProbeGetMsgReqt>& reqt)  // NOLINT
    {
        ret.msg_len = reqt->msg_len;
        ret.msg_data.reserve(reqt->msg_len);
    }
};

class ProbeServer : public Runnable {
  public:
    ProbeServer();
    virtual ~ProbeServer() {}

    void run() {
        pr_server->serve();
    }

  private:
    boost::shared_ptr<TServer>              pr_server;
    boost::shared_ptr<ProbeServiceHandler>  pr_handler;
};

ProbeServer::ProbeServer()
{
    pr_handler.reset(new ProbeServiceHandler());
    boost::shared_ptr<TProtocolFactory> proto_fac(new TBinaryProtocolFactory());
    boost::shared_ptr<TProcessor>
        processor(new ProbeServiceProcessor(pr_handler));
    boost::shared_ptr<TServerTransport> trans(new TServerSocket(9000));
    boost::shared_ptr<TTransportFactory>
        trans_fac(new TFramedTransportFactory());
    pr_server.reset(new TThreadedServer(processor,
                trans, trans_fac, proto_fac));
}

int main(int argc, char **argv)
{
    PosixThreadFactory tfac(PosixThreadFactory::ROUND_ROBIN,
            PosixThreadFactory::NORMAL, 1, false);
    boost::shared_ptr<ProbeServer> pr_server(new ProbeServer());
    boost::shared_ptr<Thread>      mgr = tfac.newThread(pr_server);

    mgr->start();
    sleep(1000);
    mgr->join();
    return 0;
}

#if 0
int
main(int argc, char **argv)
{
    int                                   port = 9000;
    boost::shared_ptr<ProbeSeviceHandler> hdler(new ProbeSeviceHandler());
    boost::shared_ptr<TServerTransport>   trans(new TServerSocket(port));
    boost::shared_ptr<TProtocolFactory>   prfac(new TBinaryProtocolFactory());

    return 0;
}
#endif
