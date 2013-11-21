/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <fds-probe/s3-probe.h>

int main(int argc, char **argv)
{
    fds::Module *s3_pm_probe_vec[] = {
        &fds::gl_AMEngine,
        nullptr
    };
    fds::ModuleVector s3_pm_probe(argc, argv, s3_pm_probe_vec);

    s3_pm_probe.mod_execute();
    fds::gl_AMEngine.run_server(nullptr);
    return 0;
}
