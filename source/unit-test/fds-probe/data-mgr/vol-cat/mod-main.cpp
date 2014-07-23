/*
 * Copyright 2014 Formation Data Systems, Inc.
 *
 * Template code to create a stand-alone unit test using the probe
 * framework to pump data to the test code.
 *
 */
#include <vol-cat-probe.h>
#include <util/fds_stat.h>
#include <fds-probe/s3-probe.h>
#include <platform/platform-lib.h>

namespace fds {

class VolCatProbeTest : public FdsProcess {
  public:
    VolCatProbeTest(int argc, char **argv, Module **mod_vec)
            : FdsProcess(argc,
                         argv,
                         "platform.conf",
                         "fds.dm.",
                         "volcat-probe.log",
                         mod_vec) {}
    virtual ~VolCatProbeTest() {}

    void proc_pre_startup() override
    {
        // Add your probe adapter(s) to S3 connector.
        gl_probeS3Eng.probe_add_adapter(&gl_VolCatProbe);
        for (int i = 0; i < 0; i++) {
            gl_probeS3Eng.probe_add_adapter(gl_VolCatProbe.pr_new_instance());
        }
    }
    void proc_pre_service() override
    {
    }
    int run() override
    {
        // TODO(Andrew): I think we only pass the native API ptr
        // because the am-engine backend expects it.
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
        &fds::gl_DmVolCatMod,
        &fds::gl_VolCatProbe,
        NULL
    };
    fds::VolCatProbeTest volcatTest(argc, argv, probe_vec);
    return volcatTest.main();
}
