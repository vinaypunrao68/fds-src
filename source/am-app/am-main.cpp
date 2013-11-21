/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <am-engine/am-engine.h>
#include <native_api.h>
extern "C" {
  extern void CreateSHMode(int argc, char *argv[]);
}

int main(int argc, char **argv)
{
  CreateSHMode(argc, argv);
    fds::FDS_NativeAPI *api = new
        fds::FDS_NativeAPI(fds::FDS_NativeAPI::FDSN_AWS_S3);

    fds::Module *am_mod_vec[] = {
        &fds::gl_AMEngine,
        nullptr
    };
    fds::ModuleVector am_module(argc, argv, am_mod_vec);

    am_module.mod_execute();
    fds::gl_AMEngine.run_server(api);

    // not reached!
    return 0;
}
