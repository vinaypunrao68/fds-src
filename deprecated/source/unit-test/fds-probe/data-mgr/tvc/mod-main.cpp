/*
 * Copyright 2014 Formation Data Systems, Inc.
 *
 * Template code to create a stand-alone unit test using the probe
 * framework to pump data to the test code.
 *
 */
#include <tvc-probe.h>
#include <util/fds_stat.h>
#include <fds-probe/s3-probe.h>
#include <platform/platform-lib.h>
#include <DataMgr.h>

namespace fds {

DataMgr * dataMgr = NULL;

class TvcProbeTest : public FdsProcess {
  public:
    TvcProbeTest(int argc, char **argv, Module **mod_vec)
            : FdsProcess(argc,
                         argv,
                         "platform.conf",
                         "fds.dm.",
                         "tvc-probe.log",
                         mod_vec) {}
    virtual ~TvcProbeTest() {}

    void proc_pre_startup() override
    {
        // Add your probe adapter(s) to S3 connector.
        gl_probeS3Eng.probe_add_adapter(&gl_TvcProbe);
        for (int i = 0; i < 0; i++) {
            gl_probeS3Eng.probe_add_adapter(gl_TvcProbe.pr_new_instance());
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
        &fds::gl_TvcProbe,
        NULL
    };
    fds::TvcProbeTest tvcTest(argc, argv, probe_vec);
    return tvcTest.main();
}
