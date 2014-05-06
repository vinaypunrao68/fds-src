/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <fds_process.h>
#include <s3-thrift-probe.h>
#include <util/fds_stat.h>
#include <fds-probe/s3-probe.h>
#include <endpoint-test.h>

namespace fds {

class AM_Probe : public FdsProcess
{
  public:
    virtual ~AM_Probe() {}
    AM_Probe(int argc, char **argv, Module **mod_vec)
        : FdsProcess(argc, argv, "platform.conf", "fds.plat.", "probe-am.log", mod_vec) {}

    void proc_pre_startup() override
    {
        gl_probeS3Eng.probe_add_adapter(&gl_thriftProbeMod);
        for (int i = 0; i < 1000; i++) {
            gl_probeS3Eng.probe_add_adapter(gl_thriftProbeMod.pr_new_instance());
        }
    }
    void proc_pre_service() override {}
    int run()
    {
        FDS_NativeAPI *api = new FDS_NativeAPI(FDS_NativeAPI::FDSN_AWS_S3);
        gl_probeS3Eng.run_server(api);
        return 0;
    }
};

}  // namespace fds

int main(int argc, char **argv)
{
    fds::Module *s3_thrift_probe_vec[] = {
        &fds::gl_fds_stat,
        &fds::gl_probeS3Eng,
        &fds::gl_thriftProbeMod,
        &fds::gl_ProbeTestAM,
        &fds::gl_ProbeSvcTestAM,
        NULL
    };
    fds::AM_Probe am_probe(argc, argv, s3_thrift_probe_vec);
    return am_probe.main();
}
