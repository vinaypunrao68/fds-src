/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <am-engine/s3connector.h>
#include <am-engine/atmos-connector.h>
#include <util/fds_stat.h>
#include <native_api.h>
#include <fds_process.h>

extern "C" {
  extern void CreateStorHvisorS3(int argc, char *argv[]);
}

int main(int argc, char **argv)
{
    fds::init_process_globals("am.log");
    CreateStorHvisorS3(argc, argv);
    fds::FDS_NativeAPI *api = new
        fds::FDS_NativeAPI(fds::FDS_NativeAPI::FDSN_AWS_S3);
    fds::FDS_NativeAPI *api_atmos = new
        fds::FDS_NativeAPI(fds::FDS_NativeAPI::FDSN_EMC_ATMOS);

    fds::Module *am_mod_vec[] = {
        &fds::gl_fds_stat,
        &fds::gl_AMEngineS3,
        &fds::gl_AMEngineAtmos,
        nullptr
    };
    fds::ModuleVector am_module(argc, argv, am_mod_vec);

    am_module.mod_execute();
    fds::gl_AMEngineS3.init_server(api);
    fds::gl_AMEngineAtmos.init_server(api_atmos);

    fds::AMEngine::run_all_servers();

    // not reached!
    return 0;
}
