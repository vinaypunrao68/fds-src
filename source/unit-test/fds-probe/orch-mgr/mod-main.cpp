/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 * Template code to create a stand-alone unit test using the probe
 * framework to pump data to the test code.
 *
 * Replace XX with your adapter name.
 */
#include <orchMgr.h>
#include <orch-mgr-probe.h>
#include <util/fds_stat.h>
#include <fds-probe/s3-probe.h>
#include <orch-mgr/om-service.h>
#include <om-discovery.h>
#include <kvstore/configdbmodule.h>
#include <om-platform.h>
#include <net/net-service.h>

namespace fds {

extern OrchMgr *orchMgr;
OM_Module       gl_OMModule("OM");

static void run_probe_server(void *arg)
{
    FDS_NativeAPI *api = new FDS_NativeAPI(fds::FDS_NativeAPI::FDSN_AWS_S3);
    gl_probeS3Eng.run_server(api);
}

OM_Module *
OM_Module::om_singleton()
{
    return &gl_OMModule;
}

class OM_Probe : public OrchMgr
{
  public:
    virtual ~OM_Probe() {}
    OM_Probe(int argc, char **argv, Module **mod_vec)
        : OrchMgr(argc, argv, &gl_OmPlatform, mod_vec) {}

    void proc_pre_startup() override
    {
        /* Add your probe adapter(s) to S3 connector. */
        gl_probeS3Eng.probe_add_adapter(&fds::gl_OM_ProbeMod);
        for (int i = 0; i < 0; i++) {
            gl_probeS3Eng.probe_add_adapter(gl_OM_ProbeMod.pr_new_instance());
        }
        OrchMgr::proc_pre_startup();
    }
    void proc_pre_service() override
    {
        fds_threadpool *pool = gl_probeS3Eng.probe_get_thrpool();
        pool->schedule(run_probe_server, orchMgr);

        OrchMgr::proc_pre_service();
    }
};
}  // namespace fds

int main(int argc, char **argv)
{
    fds::Module *probe_vec[] = {
        &fds::gl_fds_stat,
        &fds::gl_OmPlatform,
        &fds::gl_NetService,
        &fds::gl_probeS3Eng,
        &fds::gl_OMModule,
        &fds::gl_OM_ProbeMod,
        &fds::gl_configDB,
        &fds::gl_OmDiscoveryMod,
        nullptr
    };
    fds::orchMgr = new fds::OM_Probe(argc, argv, probe_vec);
    fds::gl_orch_mgr = fds::orchMgr;

    int ret = fds::orchMgr->main();
    delete fds::orchMgr;
    return ret;
}
