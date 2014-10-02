/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <pm_probe.h>
#include <util/fds_stat.h>
#include <fds-probe/s3-probe.h>
#include <persistent-layer/dm_io.h>
#include <am-platform.h>

namespace fds {

class PM_Probe : public FdsProcess
{
  public:
    virtual ~PM_Probe() {}
    PM_Probe(int argc, char **argv, Module **mod_vec)
        : FdsProcess(argc, argv, "platform.conf", "fds.plat.", "s3-pm.log", mod_vec) {}

    void proc_pre_startup() override
    {
        gl_probeS3Eng.probe_add_adapter(&gl_PM_ProbeMod);
        for (int i = 0; i < 1000; i++) {
            gl_probeS3Eng.probe_add_adapter(gl_PM_ProbeMod.pr_new_instance());
        }
    }
    void proc_pre_service() override
    {
    }
    int run() override
    {
        FDS_NativeAPI *api = new FDS_NativeAPI(FDS_NativeAPI::FDSN_AWS_S3);
        fds::gl_probeS3Eng.run_server(api);
        return 0;
    }
};
}  // namespace fds

int main(int argc, char **argv)
{
    fds::Module *s3_pm_probe_vec[] = {
        &fds::gl_fds_stat,
        &fds::gl_probeS3Eng,
        &fds::gl_AmPlatform,
        &fds::gl_PM_ProbeMod,
        &diskio::gl_dataIOMod,
        nullptr
    };
    fds::PM_Probe s3_pm_probe(argc, argv, s3_pm_probe_vec);
    return s3_pm_probe.main();
}
