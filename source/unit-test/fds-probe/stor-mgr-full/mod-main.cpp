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
#include <fds_process.h>

namespace fds {

static void run_sm_server(ObjectStorMgr *inst)
{
    inst->run();  //  not return
}

}  // namespace fds

int main(int argc, char **argv)
{
    fds::Module *probe_vec[] = {
        &fds::gl_fds_stat,
        &fds::gl_probeS3Eng,
        &diskio::gl_dataIOMod,
        &fds::gl_tierPolicy,
        &fds::gl_objStats,
        &fds::gl_Sm_ProbeMod,
        nullptr
    };
    fds::objStorMgr = new ObjectStorMgr(argc, argv, "sm.conf", "fds.sm.", probe_vec);
    fds::objStorMgr->setup();
    fds::fds_threadpool *pool = fds::gl_probeS3Eng.probe_get_thrpool();

    /* Add your probe adapter(s) to S3 connector. */
    fds::gl_probeS3Eng.probe_add_adapter(&fds::gl_Sm_ProbeMod);
    for (int i = 0; i < 20; i++) {
        fds::gl_probeS3Eng.probe_add_adapter(
            fds::gl_Sm_ProbeMod.pr_new_instance());
    }
    /* TODO: fixme. Currently needed to setup json decoders */
    fds::gl_Sm_ProbeMod.mod_startup();

    /* Now run the S3 engine. */
    pool->schedule(fds::run_sm_server, fds::objStorMgr);
    fds::gl_probeS3Eng.run_server(nullptr);
    return 0;
}
