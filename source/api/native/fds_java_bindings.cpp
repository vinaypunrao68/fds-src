/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <StorHvisorCPP.h>
#include <util/fds_stat.h>
#include <native_api.h>
#include <fds_process.h>
#include "./com_formationds_nativeapi_NativeApi.h"


JNIEXPORT void JNICALL Java_com_formationds_nativeapi_NativeApi_init(JNIEnv *env, jobject o) {
    int argc = 0;
    char *argv[] = {};
    fds::init_process_globals("am.log");
    // CreateStorHvisorS3(argc, argv);
    CreateSHMode(argc, argv, NULL, NULL, false, 0, 0);
    fds::FDS_NativeAPI *api = new fds::FDS_NativeAPI(fds::FDS_NativeAPI::FDSN_AWS_S3);

    fds::Module *am_mod_vec[] = {
        &fds::gl_fds_stat,
        nullptr
    };
    fds::ModuleVector am_module(0, NULL, am_mod_vec);

    am_module.mod_execute();
    // fds::gl_AMEngineS3.run_server(api);    
}

int main(int argc, char *argv[]) {
    return 0;
}
