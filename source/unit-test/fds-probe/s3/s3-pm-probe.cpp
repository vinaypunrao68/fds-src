/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <pm_probe.h>
#include <fds-probe/s3-probe.h>
#include <persistent_layer/dm_io.h>

int main(int argc, char **argv)
{
    fds::Module *s3_pm_probe_vec[] = {
        &fds::gl_probeS3Eng,
        &fds::gl_PM_ProbeMod,
        &diskio::gl_dataIOMod,
        nullptr
    };
    fds::ModuleVector s3_pm_probe(argc, argv, s3_pm_probe_vec);

    s3_pm_probe.mod_execute();
    fds::gl_probeS3Eng.run_server(nullptr);
    return 0;
}
