/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 * Template code to create a stand-alone unit test using the probe
 * framework to pump data to the test code.
 *
 * Replace XX with your adapter name.
 */
#include <orch-mgr-probe.h>
#include <util/fds_stat.h>
#include <fds-probe/s3-probe.h>

int main(int argc, char **argv)
{
    fds::Module *probe_vec[] = {
        &fds::gl_fds_stat,
        &fds::gl_probeS3Eng,

        &fds::gl_OM_ProbeMod,
        nullptr
    };
    fds::ModuleVector my_probe(argc, argv, probe_vec);
    my_probe.mod_execute();

    /* Add your probe adapter(s) to S3 connector. */
    fds::gl_probeS3Eng.probe_add_adapter(&fds::gl_OM_ProbeMod);
    for (int i = 0; i < 0; i++) {
        fds::gl_probeS3Eng.probe_add_adapter(
            fds::gl_OM_ProbeMod.pr_new_instance());
    }
    /* TODO: fixme, we shouldn't need to do this again */
    fds::gl_OM_ProbeMod.mod_startup();

    /* Now run the S3 engine. */
    fds::gl_probeS3Eng.run_server(nullptr);
    return 0;
}
