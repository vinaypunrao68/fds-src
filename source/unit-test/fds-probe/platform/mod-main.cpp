/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 * shm-queue code to create a stand-alone unit test using the probe
 * framework to pump data to the test code.
 *
 */

#include <fds-probe-queue.h>
#include <util/fds_stat.h>
#include <fds-probe/s3-probe.h>

int main(int argc, char **argv)
{
    fds::FDS_NativeAPI *api = new
                fds::FDS_NativeAPI(fds::FDS_NativeAPI::FDSN_AWS_S3);
    fds::Module *probe_vec[] = {
        &fds::gl_fds_stat,
        &fds::gl_probeS3Eng,

        /* Add your vector and its dependencies here. */
        &fds::gl_shm_queue_ProbeMod,
        nullptr
    };
    fds::ModuleVector my_probe(argc, argv, probe_vec);
    my_probe.mod_execute();

    /* Add your probe adapter(s) to S3 connector. */
    fds::gl_probeS3Eng.probe_add_adapter(&fds::gl_shm_queue_ProbeMod);
    /*
    for (int i = 0; i < 100; i++) {
        fds::gl_probeS3Eng.probe_add_adapter(
            fds::gl_shm_queue_ProbeMod.pr_new_instance());
    }
    */
    fds::gl_probeS3Eng.probe_add_adapter(
        fds::gl_shm_queue_ProbeMod.pr_new_instance());
    fds::gl_shm_queue_ProbeMod.mod_startup();

    /* Now run the S3 engine. */
    fds::gl_probeS3Eng.run_server(api);
    return 0;
}
