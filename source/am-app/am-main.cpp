/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <am-engine/am-engine.h>

int main(int argc, char **argv)
{
    fds::Module *am_mod_vec[] = {
        &fds::gl_AMEngine,
        nullptr
    };
    fds::ModuleVector am_module(argc, argv, am_mod_vec);

    am_module.mod_execute();
    fds::gl_AMEngine.run_server();
    return 0;
}
