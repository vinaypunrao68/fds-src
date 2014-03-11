/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <pm_probe.h>
#include <util/fds_stat.h>
#include <fds-probe/s3-probe.h>
#include <persistent_layer/dm_io.h>
#include <am-platform.h>

int main(int argc, char **argv)
{
    fds::FDS_NativeAPI *api = new
                fds::FDS_NativeAPI(fds::FDS_NativeAPI::FDSN_AWS_S3);
    fds::Module *s3_pm_probe_vec[] = {
        &fds::gl_fds_stat,
        &fds::gl_probeS3Eng,
        &fds::gl_AmPlatform,
        &fds::gl_PM_ProbeMod,
        &diskio::gl_dataIOMod,
        nullptr
    };
    fds::ModuleVector s3_pm_probe(argc, argv, s3_pm_probe_vec);

    s3_pm_probe.mod_execute();
    fds::gl_probeS3Eng.probe_add_adapter(&fds::gl_PM_ProbeMod);
    fds::gl_probeS3Eng.run_server(api);
    return 0;
}
