/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 * Template code to create a stand-alone unit test using the probe
 * framework to pump data to the test code.
 *
 * Replace XX with your adapter name.
 */
#include <template_probe.h>
#include <util/fds_stat.h>
#include <fds-probe/s3-probe.h>
#include <persistent-layer/dm_io.h>

int main(int argc, char **argv)
{
    fds::Module *probe_vec[] = {
        &fds::gl_fds_stat,
        &fds::gl_probeBlkLib,
        &diskio::gl_dataIOMod,

        /* Add your vector and its dependencies here. */
        &fds::gl_XX_ProbeMod,
        nullptr
    };
    fds::ModuleVector my_probe(argc, argv, probe_vec);
    my_probe.mod_execute();

    /* Add your probe adapter(s) to block connector. */
    fds::gl_probeBlkLib.probe_run_main(&fds::gl_XX_ProbeMod);
    return 0;
}
