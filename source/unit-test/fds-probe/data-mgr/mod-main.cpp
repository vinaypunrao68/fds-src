/*
 * Copyright 2014 Formation Data Systems, Inc.
 *
 * Template code to create a stand-alone unit test using the probe
 * framework to pump data to the test code.
 *
 */
#include <dm-probe.h>
#include <util/fds_stat.h>
#include <fds-probe/s3-probe.h>
#include <DataMgr.h>
#include <dm-platform.h>

namespace fds {

DataMgr *dataMgr;
static void run_dm_server(DataMgr *inst)
{
    inst->run();  //  not return
}

class DM_Probe : public DataMgr
{
  public:
    virtual ~DM_Probe() {}
    DM_Probe(int argc, char **argv, Platform *platform, Module **mod_vec)
        : DataMgr(argc, argv, platform, mod_vec) {}

    void proc_pre_startup() override
    {
        // Add your probe adapter(s) to S3 connector.
        gl_probeS3Eng.probe_add_adapter(&gl_Dm_ProbeMod);
        for (int i = 0; i < 0; i++) {
            gl_probeS3Eng.probe_add_adapter(gl_Dm_ProbeMod.pr_new_instance());
        }
    }
    void proc_pre_service() override
    {
        // Now run the S3 engine.
        fds_threadpool *pool = fds::gl_probeS3Eng.probe_get_thrpool();
        pool->schedule(run_dm_server, this);
    }
    int run() override
    {
        FDS_NativeAPI *api = new FDS_NativeAPI(FDS_NativeAPI::FDSN_AWS_S3);
        gl_probeS3Eng.run_server(api);
        return 0;
    }
};
}  // namespace fds

int main(int argc, char **argv)
{
    fds::Module *probe_vec[] = {
        &fds::gl_fds_stat,
        &fds::gl_probeS3Eng,
        &fds::gl_DmPlatform,
        &fds::gl_Dm_ProbeMod,
        NULL
    };
    fds::dataMgr = new fds::DM_Probe(argc, argv, &fds::gl_DmPlatform, probe_vec);
    int ret = fds::dataMgr->main();
    delete dataMgr;
    return ret;
}
