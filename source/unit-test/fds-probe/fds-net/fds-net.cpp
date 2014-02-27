/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <fds-net-probe.h>
#include <util/fds_stat.h>
#include <fds-probe/s3-probe.h>
#include <fds_process.h>

int main(int argc, char **argv)
{
    fds::init_process_globals("fds-net-global.log");

    fds::FDS_NativeAPI *api = new
                fds::FDS_NativeAPI(fds::FDS_NativeAPI::FDSN_AWS_S3);
    fds::Module *s3_fds_net_probe_vec[] = {
        &fds::gl_fds_stat,
        &fds::gl_probeS3Eng,
        &fds::gl_fdsNetProbeMod,
        nullptr
    };
    fds::ModuleVector fdsNetProbeVec(argc, argv, s3_fds_net_probe_vec);
    fdsNetProbeVec.mod_execute();

    fds::gl_probeS3Eng.probe_add_adapter(&fds::gl_fdsNetProbeMod);
    for (int i = 0; i < 4; i++) {
        fds::gl_probeS3Eng.probe_add_adapter(
            fds::gl_fdsNetProbeMod.pr_new_instance());
    }
    fds::gl_probeS3Eng.run_server(api);
    return 0;
}
