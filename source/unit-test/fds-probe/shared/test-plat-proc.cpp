/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <fds_uuid.h>
#include <fds-probe/fds_probe.h>
#include <fds-probe/s3-probe.h>
#include <platform/platform-lib.h>
#include <net/net-service.h>
#include <platform.h>
#include <net-platform.h>
#include <util/fds_stat.h>

namespace fds {

FdsService::FdsService(int argc, char *argv[], const std::string &log, Module **vec)
{
    static Module *plat_vec[] = {
        &gl_NodePlatform,
        &gl_NetService,
        &gl_PlatformdNetSvc,
        NULL
    };
    svc_modules = Module::mod_cat(plat_vec, vec);
    init(argc, argv, "fds.plat.", log, &gl_NodePlatform, svc_modules);
}

FdsService::~FdsService()
{
    delete [] svc_modules;
}

void
FdsService::proc_pre_startup()
{
    PlatformProcess::proc_pre_startup();
}

void
FdsService::proc_pre_service()
{
    /* TODO(Vy/Rao): Total hack.  Most of the code platform implementers owned base async receiver
     * in the NodePlatform case that's not the case.  It's owned by PlatformdNetSvc.  We need to
     * fix this make sure all platform implementers are consistent in the paradigms they follow
     */
    gl_NodePlatform.setBaseAsyncSvcHandler(gl_PlatformdNetSvc.getPlatNetSvcHandler());
}

void
FdsService::plf_load_node_data()
{
    PlatformProcess::plf_load_node_data();
    if (plf_node_data.nd_has_data == 0) {
        NodeUuid uuid(fds_get_uuid64(get_uuid()));
        plf_node_data.nd_node_uuid = uuid.uuid_get_val();
        LOGNOTIFY << "NO UUID, generate one " << std::hex
            << plf_node_data.nd_node_uuid << std::dec;
    }
    // Make up some values for now.
    //
    plf_node_data.nd_has_data    = 1;
    plf_node_data.nd_magic       = 0xcafecaaf;
    plf_node_data.nd_node_number = 0;
    plf_node_data.nd_plat_port   = plf_mgr->plf_get_my_node_port();
    plf_node_data.nd_om_port     = plf_mgr->plf_get_om_ctrl_port();
    plf_node_data.nd_flag_run_sm = 1;
    plf_node_data.nd_flag_run_dm = 1;
    plf_node_data.nd_flag_run_am = 1;
    plf_node_data.nd_flag_run_om = 1;

    // TODO(Vy): deal with error here.
    //
    plf_db->set(plf_db_key,
                std::string((const char *)&plf_node_data, sizeof(plf_node_data)));
}

FdsProbeProcess::~FdsProbeProcess() {}
FdsProbeProcess::FdsProbeProcess(int argc, char *argv[],
                                 const std::string &cfg,
                                 const std::string &log,
                                 ProbeMod *probe, Platform *plat, Module **vec)
{
    static Module *fds_vec[] = {
        plat,
        &gl_NetService,
        &gl_fds_stat,
        &gl_probeS3Eng,
        probe,
        NULL
    };
    svc_probe   = probe;
    svc_modules = Module::mod_cat(fds_vec, vec);
    init(argc, argv, cfg, log, plat, svc_modules);
}

static void
run_probe_server(FdsProbeProcess *proc)
{
    FDS_NativeAPI *api = new FDS_NativeAPI(FDS_NativeAPI::FDSN_AWS_S3);
    gl_probeS3Eng.run_server(api);
}

void
FdsProbeProcess::proc_pre_startup()
{
    FdsService::proc_pre_startup();

    gl_probeS3Eng.probe_add_adapter(svc_probe);
    for (int i = 0; i < 10; i++) {
        gl_probeS3Eng.probe_add_adapter(svc_probe->pr_new_instance());
    }
}

void
FdsProbeProcess::proc_pre_service()
{
    fds_threadpool *pool = gl_probeS3Eng.probe_get_thrpool();
    pool->schedule(run_probe_server, this);
    FdsService::proc_pre_service();
}

ProbeProcess::~ProbeProcess() {}
ProbeProcess::ProbeProcess(int argc, char *argv[],
                           const std::string &log,
                           ProbeMod *probe, Module **vec, const std::string &cfg)
{
    static Module *plat_vec[] = {
        &gl_NodePlatform,
        &gl_NetService,
        &gl_PlatformdNetSvc,
        &gl_fds_stat,
        &gl_probeS3Eng,
        probe,
        NULL
    };
    svc_probe   = probe;
    svc_modules = Module::mod_cat(plat_vec, vec);
    init(argc, argv, cfg, log, &gl_NodePlatform, svc_modules);
}

}  // namespace fds
