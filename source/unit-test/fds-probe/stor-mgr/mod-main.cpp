/*
 * Copyright 2014 Formation Data Systems, Inc.
 *
 * Template code to create a stand-alone unit test using the probe
 * framework to pump data to the test code.
 *
 */
#include <sm-probe-client.h>
#include <util/fds_stat.h>
#include <fds-probe/s3-probe.h>

#include <fds_process.h>

int main(int argc, char **argv)
{
    fds::init_process_globals("sm-client-probe.log");

    fds::FDS_NativeAPI *api = new
                fds::FDS_NativeAPI(fds::FDS_NativeAPI::FDSN_AWS_S3);
    fds::Module *probe_vec[] = {
        &fds::gl_fds_stat,
        &fds::gl_probeS3Eng,
        &fds::gl_Sm_ProbeMod,
        nullptr
    };
    fds::ModuleVector my_probe(argc, argv, probe_vec);
    my_probe.mod_execute();

    /* Add your probe adapter(s) to S3 connector. */
    fds::gl_probeS3Eng.probe_add_adapter(&fds::gl_Sm_ProbeMod);
    for (int i = 0; i < 20; i++) {
        fds::gl_probeS3Eng.probe_add_adapter(
            fds::gl_Sm_ProbeMod.pr_new_instance());
    }
    /* TODO: fixme. Currently needed to setup json decoders */
    fds::gl_Sm_ProbeMod.mod_startup();

    /* Now run the S3 engine. */
    fds::gl_probeS3Eng.run_server(api);
    return 0;
}
