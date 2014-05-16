/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#include <util/fds_stat.h>
#include <fds-probe/s3-probe.h>
#include <am-platform.h>
#include <endpoint-test.h>
#include <net/net-service.h>

namespace fds {

class ProbeSM_RPC : public FdsProcess
{
  public:
    virtual ~ProbeSM_RPC() {}
    ProbeSM_RPC(int argc, char **argv, Module **vec)
        : FdsProcess(argc, argv, "platform.conf", "fds.plat.", "probe-sm.log", vec) {}

    void proc_pre_startup() override {}
    void proc_pre_service() override {}
    int run() override {
        gl_ProbeTestSM.ep_serve();
        return 0;
    }
};

}  // namespace fds

int main(int argc, char **argv)
{
    fds::Module *srv_vec[] = {
        &fds::gl_fds_stat,
        &fds::gl_probeS3Eng,
        &fds::gl_AmPlatform,
        &fds::gl_NetService,
        &fds::gl_ProbeTestSM,
        NULL
    };
    fds::ProbeSM_RPC sm_rpc(argc, argv, srv_vec);
    return sm_rpc.main();
}

#if 0
int main(int argc, char **argv)
{
    PosixThreadFactory tfac(PosixThreadFactory::ROUND_ROBIN,
            PosixThreadFactory::NORMAL, 1, false);
    boost::shared_ptr<fds::ProbeServer> pr_server(new fds::ProbeServer());
    boost::shared_ptr<Thread>      mgr = tfac.newThread(pr_server);

    mgr->start();
    sleep(1000);
    mgr->join();
    return 0;
}

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
