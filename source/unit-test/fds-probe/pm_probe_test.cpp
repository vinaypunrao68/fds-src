/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <pm_probe.h>
#include <fds_module.h>
#include <persistent_layer/dm_io.h>
#include <iostream>

using namespace std;

int
main(int argc, char **argv)
{
    fds::Module *pm_probe_vec[] = {
        &diskio::gl_dataIOMod,
        &fds::gl_probeBlkLib,
        nullptr
    };
    fds::ModuleVector pm_probe(argc, argv, pm_probe_vec);

    pm_probe.mod_execute();
    fds::gl_probeBlkLib.probe_run_main(&fds::gl_PM_ProbeMod);
    return 0;
}
