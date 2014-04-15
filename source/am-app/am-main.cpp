/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <string>
#include <am-engine/s3connector.h>
#include <am-engine/atmos-connector.h>
#include <am-engine/xdi-server.h>
#include <util/fds_stat.h>
#include <native_api.h>
#include <am-platform.h>

extern "C" {
extern void CreateStorHvisorS3(int argc, char **argv);
}

namespace fds {

class AM_Process : public PlatformProcess
{
  public:
    virtual ~AM_Process() {}
    AM_Process(int argc, char **argv,
               Platform *platform, Module **mod_vec)
        : PlatformProcess(argc, argv, "fds.am.", "am.log", platform, mod_vec) {}

    void proc_setup() override
    {
        int    argc;
        char **argv;

        PlatformProcess::proc_setup();
        FDS_NativeAPI *api = new FDS_NativeAPI(FDS_NativeAPI::FDSN_AWS_S3);
        FDS_NativeAPI *api_atmos = new FDS_NativeAPI(FDS_NativeAPI::FDSN_EMC_ATMOS);
        FDS_NativeAPI *api_xdi = new FDS_NativeAPI(FDS_NativeAPI::FDSN_AWS_S3);

        gl_AMEngineS3.init_server(api);
        gl_AMEngineAtmos.init_server(api_atmos);
        gl_XdiServer.init_server(api_xdi);

        argv = mod_vectors_->mod_argv(&argc);
        CreateStorHvisorS3(argc, argv);
    }
    int run() override {
        AMEngine::run_all_servers();
        return 0;
    }
};

}  // namespace fds

extern "C" {
    extern void CreateStorHvisorS3(int argc, char *argv[]);
}

int main(int argc, char **argv)
{
    fds::Module *am_mod_vec[] = {
        &fds::gl_fds_stat,
        &fds::gl_AmPlatform,
        &fds::gl_AMEngineS3,
        &fds::gl_AMEngineAtmos,
        &fds::gl_XdiServer,
        nullptr
    };
    fds::AM_Process am_process(argc, argv, &fds::gl_AmPlatform, am_mod_vec);
    return am_process.main();
}
