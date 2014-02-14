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

namespace fds {

extern OrchMgr *orchMgr;
OM_Module       gl_OMModule("OM");

static void run_om_server(OrchMgr *inst)
{
    inst->run();   //  not return.
}

OM_Module *
OM_Module::om_singleton()
{
    return &gl_OMModule;
}

}  // namespace fds

int main(int argc, char **argv)
{
    fds::orchMgr     = new fds::OrchMgr(argc, argv, "orch_mgr.conf", "fds.om.");
    fds::gl_orch_mgr = fds::orchMgr;

    fds::Module *probe_vec[] = {
        &fds::gl_fds_stat,
        &fds::gl_probeS3Eng,
        fds::orchMgr,
        &fds::gl_OMModule,
        &fds::gl_OM_ProbeMod,
        nullptr
    };
    fds::orchMgr->setup(argc, argv, probe_vec);
    fds::fds_threadpool *pool = fds::gl_probeS3Eng.probe_get_thrpool();

    /* Add your probe adapter(s) to S3 connector. */
    fds::gl_probeS3Eng.probe_add_adapter(&fds::gl_OM_ProbeMod);
    for (int i = 0; i < 0; i++) {
        fds::gl_probeS3Eng.probe_add_adapter(
            fds::gl_OM_ProbeMod.pr_new_instance());
    }
    /* TODO: fixme, we shouldn't need to do this again */
    fds::gl_OM_ProbeMod.mod_startup();

    /* Now run the S3 engine. */
    pool->schedule(fds::run_om_server, fds::orchMgr);
    fds::gl_probeS3Eng.run_server(nullptr);
    return 0;
}
