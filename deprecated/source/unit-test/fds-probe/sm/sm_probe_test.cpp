/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <sm_probe.h>
#include <fds_module.h>
#include <persistent-layer/dm_io.h>
#include <iostream>

// This is the example of stand-alone SM unit test if SM code were structured
// with FDS Module.
//
int
main(int argc, char **argv)
{
    fds::Module *sm_probe_vec[] = {
        &diskio::gl_dataIOMod,
        &fds::gl_probeBlkLib,
        nullptr
    };
    fds::ModuleVector sm_probe(argc, argv, sm_probe_vec);

    sm_probe.mod_execute();
    fds::gl_probeBlkLib.probe_run_main(&fds::gl_SM_ProbeMod);
    return 0;
}
