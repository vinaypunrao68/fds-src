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
}  // namespace fds

int main(int argc, char **argv)
{
    fds::FDS_NativeAPI *api = new fds::FDS_NativeAPI(fds::FDS_NativeAPI::FDSN_AWS_S3);
    fds::Module *probe_vec[] = {
        &fds::gl_fds_stat,
        &fds::gl_probeS3Eng,
        &fds::gl_DmPlatform,
        &fds::gl_Dm_ProbeMod,
        NULL
    };

    fds::dataMgr = new DataMgr(argc, argv, &fds::gl_DmPlatform, probe_vec);
    fds::dataMgr->setup();
    fds::fds_threadpool *pool = fds::gl_probeS3Eng.probe_get_thrpool();

    // Add your probe adapter(s) to S3 connector.
    fds::gl_probeS3Eng.probe_add_adapter(&fds::gl_Dm_ProbeMod);
    for (int i = 0; i < 0; i++) {
        fds::gl_probeS3Eng.probe_add_adapter(
            fds::gl_Dm_ProbeMod.pr_new_instance());
    }
    // TODO(Vy): fixme. Currently needed to setup json decoders
    fds::gl_Dm_ProbeMod.mod_startup();

    // Now run the S3 engine.
    pool->schedule(fds::run_dm_server, fds::dataMgr);
    fds::gl_probeS3Eng.run_server(api);

    return 0;
}
