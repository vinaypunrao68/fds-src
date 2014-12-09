/*
 * Copyright 2014 Formation Data Systems, Inc.
 *
 * Template code to create a stand-alone unit test using the probe
 * framework to pump data to the test code.
 *
 */
#include <sm-probe.h>
#include <util/fds_stat.h>
#include <fds-probe/s3-probe.h>
#include <policy_tier.h>
#include <StorMgr.h>
#include <sm-platform.h>

namespace fds {

static void run_sm_server(ObjectStorMgr *inst)
{
    inst->run();  //  not return
}

// TODO(Rao): Fix me
#if 0
class SM_Probe : public ObjectStorMgr
{
  public:
    virtual ~SM_Probe() {}
    SM_Probe(int argc, char **argv, Platform *platform, Module **mod_vec)
        : ObjectStorMgr(argc, argv, &gl_SmPlatform, mod_vec) {}

    void proc_pre_startup() override
    {
        /* Add your probe adapter(s) to S3 connector. */
        gl_probeS3Eng.probe_add_adapter(&gl_Sm_ProbeMod);
        for (int i = 0; i < 20; i++) {
            gl_probeS3Eng.probe_add_adapter(gl_Sm_ProbeMod.pr_new_instance());
        }
    }
    void proc_pre_service() override
    {
        /* Now run the S3 engine. */
        fds::fds_threadpool *pool = fds::gl_probeS3Eng.probe_get_thrpool();
        pool->schedule(fds::run_sm_server, fds::objStorMgr);
    }
    int run() override
    {
        FDS_NativeAPI *api = new FDS_NativeAPI(fds::FDS_NativeAPI::FDSN_AWS_S3);
        gl_probeS3Eng.run_server(api);
        return 0;
    }
};
#endif

}  // namespace fds

int main(int argc, char **argv)
{
// TODO(Rao): Fix me
#if 0
    fds::Module *probe_vec[] = {
        &fds::gl_fds_stat,
        &fds::gl_probeS3Eng,
        &diskio::gl_dataIOMod,
        &fds::gl_SmPlatform,
        &fds::gl_tierPolicy,
        &fds::gl_objStats,
        &fds::gl_Sm_ProbeMod,
        nullptr
    };
    fds::objStorMgr = new SM_Probe(argc, argv, &fds::gl_SmPlatform, probe_vec);
    int ret = fds::objStorMgr->main();
    delete fds::objStorMgr;

    return ret;
#endif
    return 0;
}
