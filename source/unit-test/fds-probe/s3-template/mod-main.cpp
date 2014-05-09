/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 * Template code to create a stand-alone unit test using the probe
 * framework to pump data to the test code.
 *
 * Replace XX with your adapter name.
 */
#include <template_probe.h>
#include <util/fds_stat.h>
#include <fds-probe/s3-probe.h>
#include <fds_process.h>

namespace fds {

class ProbeProcess : public FdsProcess
{
  public:
    virtual ~ProbeProcess() {}
    ProbeProcess(int argc, char **argv, Module **vec)
        : FdsProcess(argc, argv, "platform.conf", "fds.plat.", "probe-s3.log", vec) {}

    void proc_pre_startup() override
    {
        gl_probeS3Eng.probe_add_adapter(&gl_XX_ProbeMod);
        for (int i = 0; i < 0; i ++) {
            gl_probeS3Eng.probe_add_adapter(gl_XX_ProbeMod.pr_new_instance());
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
    fds::Module *probe_vec[] = {
        &fds::gl_fds_stat,
        &fds::gl_probeS3Eng,

        /* Add your vector and its dependencies here. */
        &fds::gl_XX_ProbeMod,
        NULL
    };
    fds::ProbeProcess probe(argc, argv, probe_vec);
    return probe.main();
}
