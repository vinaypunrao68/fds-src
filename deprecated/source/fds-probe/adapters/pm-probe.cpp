/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <iostream>
#include <string>
#include <pm_probe.h>
#include <persistent-layer/dm_io.h>
#include <persistent-layer/pm_unit_test.h>

namespace fds {

// req_complete
// ------------
//
void
DiskReqTest::req_complete()
{
    fdsio::Request::req_complete();
}

// req_verify
// ----------
//
void
DiskReqTest::req_verify()
{
}

// req_gen_pattern
// ---------------
//
void
DiskReqTest::req_gen_pattern()
{
}

probe_mod_param_t pm_probe_param =
{
    .pr_stat_cnt     = 0,
    .pr_inj_pts_cnt  = 0,
    .pr_inj_act_cnt  = 0,
    .pr_max_sec_tout = 0
};

PM_ProbeMod gl_PM_ProbeMod("PM Probe Adapter",
                           &pm_probe_param,
                           &diskio::gl_dataIOMod);

// pr_new_instance
// ---------------
//
ProbeMod *
PM_ProbeMod::pr_new_instance()
{
    return new PM_ProbeMod("PM Inst",
                           &pm_probe_param, &diskio::gl_dataIOMod);
}

// pr_intercept_request
// --------------------
//
void
PM_ProbeMod::pr_intercept_request(ProbeRequest *req)
{
}

// pr_put
// ------
//
void
PM_ProbeMod::pr_put(ProbeRequest *probe)
{
    DiskReqTest    *req;
    meta_vol_io_t  vio;
    meta_obj_id_t  oid;
    fds::ObjectBuf *buf;
    ProbeIORequest *io = reinterpret_cast<ProbeIORequest *>(probe);
    diskio::DataIO &pio = diskio::DataIO::disk_singleton();

    vio.vol_rsvd    = 0;
    buf = new ObjectBuf;
    (buf->data)->assign(io->pr_wr_buf, io->pr_wr_size);
    const unsigned int oid_size = sizeof(oid);
    for (unsigned int i = 0; i < oid_size; i++) {
       oid.metaDigest[i % oid_size] = (random() % 10) + 1;
    }
    req = new DiskReqTest(vio, oid, buf, true, diskio::diskTier);
    pio.disk_write(req);
    delete req;
    probe->pr_request_done();
}

// pr_get
// ------
//
void
PM_ProbeMod::pr_get(ProbeRequest *req)
{
    req->req_complete();
}

// pr_delete
// ---------
//
void
PM_ProbeMod::pr_delete(ProbeRequest *req)
{
    req->req_complete();
}

// pr_verify_request
// -----------------
//
void
PM_ProbeMod::pr_verify_request(ProbeRequest *req)
{
}

// pr_gen_report
// -------------
//
void
PM_ProbeMod::pr_gen_report(std::string *out)
{
}

// mod_init
// --------
//
int
PM_ProbeMod::mod_init(SysParams const *const param)
{
    Module::mod_init(param);
    return 0;
}

// mod_startup
// -----------
//
void
PM_ProbeMod::mod_startup()
{
    Module::mod_startup();
}

// mod_shutdown
// ------------
//
void
PM_ProbeMod::mod_shutdown()
{
}
}   // namespace fds
