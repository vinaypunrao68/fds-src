/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <s3-thrift-probe.h>
#include <util/fds_stat.h>
#include <fds-probe/s3-probe.h>

int main(int argc, char **argv)
{
    fds::FDS_NativeAPI *api = new
                fds::FDS_NativeAPI(fds::FDS_NativeAPI::FDSN_AWS_S3);
    fds::Module *s3_thrift_probe_vec[] = {
        &fds::gl_fds_stat,
        &fds::gl_probeS3Eng,
        &fds::gl_thriftProbeMod,
        nullptr
    };
    fds::ModuleVector s3_thrift_probe(argc, argv, s3_thrift_probe_vec);

    s3_thrift_probe.mod_execute();
    fds::gl_probeS3Eng.probe_add_adapter(&fds::gl_thriftProbeMod);
    for (int i = 0; i < 20; i++) {
        fds::gl_probeS3Eng.probe_add_adapter(
            fds::gl_thriftProbeMod.pr_new_instance());
    }
    fds::gl_probeS3Eng.run_server(api);
    return 0;
}
