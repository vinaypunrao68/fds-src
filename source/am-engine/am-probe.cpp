/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <am-engine/am-probe.h>
#include <fds-probe/s3-probe.h>
#include <am-engine/handlers/handlermappings.h>
#include <am-engine/handlers/responsehandler.h>

namespace fds {

static void
probeS3ServerEntry(ProbeS3Eng *s3Probe,
                   FDS_NativeAPI::ptr api) {
    // TODO(Andrew): Change the probe to take
    // a shared_ptr so we don't extract the raw ptr
    s3Probe->run_server(api.get());
}

AmProbe::AmProbe(const std::string &name,
                 probe_mod_param_t *param,
                 Module *owner)
        : ProbeMod(name.c_str(),
                   param,
                   owner) {
}

AmProbe::~AmProbe() {
}

int
AmProbe::mod_init(SysParams const *const param) {
    return 0;
}

void
AmProbe::mod_startup() {
}

void
AmProbe::mod_shutdown() {
    listen_thread->join();
}

void
AmProbe::init_server(FDS_NativeAPI::ptr api) {
    LOGDEBUG << "Starting AM test probe...";
    am_api = api;

    listen_thread.reset(new boost::thread(&probeS3ServerEntry,
                                          &gl_probeS3Eng,
                                          am_api));
}

probe_mod_param_t Am_probe_param =
{
    .pr_stat_cnt     = 0,
    .pr_inj_pts_cnt  = 0,
    .pr_inj_act_cnt  = 0,
    .pr_max_sec_tout = 0
};

// pr_new_instance
// ---------------
//
ProbeMod *
AmProbe::pr_new_instance()
{
    AmProbe *apm = new AmProbe("Am Client Inst", &Am_probe_param, NULL);
    apm->mod_init(mod_params);
    apm->mod_startup();

    return apm;
}

// pr_intercept_request
// --------------------
//
void
AmProbe::pr_intercept_request(ProbeRequest *req)
{
}

// pr_put
// ------
//
void
AmProbe::pr_put(ProbeRequest *probe)
{
}

// pr_get
// ------
//
void
AmProbe::pr_get(ProbeRequest *req)
{
}

// pr_delete
// ---------
//
void
AmProbe::pr_delete(ProbeRequest *req)
{
}

// pr_verify_request
// -----------------
//
void
AmProbe::pr_verify_request(ProbeRequest *req)
{
}

// pr_gen_report
// -------------
//
void
AmProbe::pr_gen_report(std::string *out)
{
}

AmProbe gl_AmProbe("Global Am test probe",
                   &Am_probe_param, NULL);

}  // namespace fds
