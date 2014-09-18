/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <plat-probe.h>
#include <util/fds_stat.h>
#include <fds-probe/s3-probe.h>
#include <fds_process.h>
#include <disk.h>
#include <platform.h>
#include <net-platform.h>
#include <net/net-service.h>

namespace fds {

static void
run_probe_server(NodePlatformProc *proc)
{
    FDS_NativeAPI *api = new FDS_NativeAPI(FDS_NativeAPI::FDSN_AWS_S3);
    gl_probeS3Eng.run_server(api);
}

class PlatProbeProcess : public NodePlatformProc
{
  public:
    virtual ~PlatProbeProcess() {}
    PlatProbeProcess(int argc, char **argv, Module **vec)
        : NodePlatformProc(argc, argv, vec) {}

    void proc_pre_startup() override
    {
        gl_probeS3Eng.probe_add_adapter(&gl_PlatProbeMod);
        for (int i = 0; i < 10; i ++) {
            gl_probeS3Eng.probe_add_adapter(gl_PlatProbeMod.pr_new_instance());
        }
        NodePlatformProc::proc_pre_startup();
    }
    void proc_pre_service() override
    {
        fds_threadpool *pool = fds::gl_probeS3Eng.probe_get_thrpool();
        pool->schedule(run_probe_server, this);
        NodePlatformProc::proc_pre_service();
    }
};

}  // namespace fds

int main(int argc, char **argv)
{
    fds::Module *probe_vec[] = {
        &fds::gl_NodePlatform,
        &fds::gl_DiskPlatMod,
        &fds::gl_NetService,
        &fds::gl_PlatformdNetSvc,
        &fds::gl_fds_stat,
        &fds::gl_probeS3Eng,
        &fds::gl_PlatProbeMod,
        NULL
    };
    fds::PlatProbeProcess probe(argc, argv, probe_vec);
    return probe.main();
}
