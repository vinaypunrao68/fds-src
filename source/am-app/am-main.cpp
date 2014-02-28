/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <string>
#include <am-engine/s3connector.h>
#include <am-engine/atmos-connector.h>
#include <util/fds_stat.h>
#include <native_api.h>
#include <fds_process.h>

namespace fds {

class AM_Process : public FdsProcess
{
  public:
    virtual ~AM_Process() {}
    AM_Process(int argc, char **argv,
               const std::string &def_cfg_file,
               const std::string &base_path, Module **mod_vec)
        : FdsProcess(argc, argv, def_cfg_file, base_path, mod_vec) {}

    void setup() override
    {
        FDS_NativeAPI *api = new FDS_NativeAPI(FDS_NativeAPI::FDSN_AWS_S3);
        FDS_NativeAPI *api_atmos = new FDS_NativeAPI(FDS_NativeAPI::FDSN_EMC_ATMOS);

        FdsProcess::setup();
        gl_AMEngineS3.init_server(api);
        gl_AMEngineAtmos.init_server(api_atmos);
    }
    void run() override {
        AMEngine::run_all_servers();
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
        &fds::gl_AMEngineS3,
        &fds::gl_AMEngineAtmos,
        nullptr
    };
    fds::AM_Process am_process(argc, argv, "am.conf", "fds.am.", am_mod_vec);
    CreateStorHvisorS3(argc, argv);

    am_process.setup();
    am_process.run();
    // not reached!
    return 0;
}
