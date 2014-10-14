/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <fds-probe/fds_probe.h>

namespace fds {

ProbeRequest::ProbeRequest(int stat_cnt, size_t buf_siz, ProbeMod *mod)
    : fdsio::Request(true), pr_stat_cnt(stat_cnt), pr_mod(mod)
{
    if (stat_cnt > 0) {
        pr_stats = new fds_uint64_t [stat_cnt];
    } else {
        pr_stats = nullptr;
    }
    (pr_buf.data)->reserve(buf_siz);
}

ProbeRequest::~ProbeRequest()
{
    delete [] pr_stats;
}

void
ProbeRequest::pr_request_done()
{
    fdsio::Request::req_complete();
}

void
ProbeRequest::pr_stat_begin(int stat_idx)
{
}

void
ProbeRequest::pr_stat_end(int stat_idx)
{
}

void
ProbeRequest::pr_inj_point(probe_point_e point)
{
    pr_mod->pr_inj_point(point, this);
}

bool
ProbeRequest::pr_report_stat(probe_stat_rec_t *out, int rec_cnt)
{
    return false;
}

// ----------------------------------------------------------------------------
// Common Probe Module
// ----------------------------------------------------------------------------
ProbeMod::ProbeMod(char const *const name,
                   probe_mod_param_t *param,
                   Module            *owner)
    : Module(name), pr_queue(1, 0xffff), pr_param(param), pr_mod_owner(owner),
      pr_stats_info(nullptr), pr_inj_points(nullptr), pr_inj_actions(nullptr),
      pr_mtx("Probe Mtx"), pr_thrpool(nullptr), pr_parent(NULL)
{
    pr_objects = new JsObjManager();
}

ProbeMod::~ProbeMod()
{
    if (pr_thrpool != NULL) {
        delete pr_thrpool;
    }
    delete [] pr_stats_info;
    delete [] pr_inj_points;
    delete [] pr_inj_actions;
    delete pr_objects;
}

// pr_add_module
// -------------
//
void
ProbeMod::pr_add_module(ProbeMod *chain)
{
    pr_mtx.lock();
    pr_chain.push_front(chain);
    pr_mtx.unlock();

    chain->pr_parent = this;
}

// pr_get_module
// -------------
//
ProbeMod *
ProbeMod::pr_get_module()
{
    ProbeMod *chain;

    pr_mtx.lock();
    chain = pr_chain.front();
    pr_chain.pop_front();
    pr_mtx.unlock();

    return chain;
}

// pr_get_module
// -------------
//
void
ProbeMod::pr_get_module(fds_threadpool *pool,
                        ProbeMod *chain, ProbeRequest *req)
{
}

// pr_get_module_callback
// ----------------------
//
void
ProbeMod::pr_get_module_callback(ProbeRequest *req)
{
}

// pr_inj_point
// ------------
//
void
ProbeMod::pr_inj_point(probe_point_e point, ProbeRequest *req)
{
}

// pr_inj_trigger_pct_hit
// ----------------------
//
bool
ProbeMod::pr_inj_trigger_pct_hit(probe_point_e inj, ProbeRequest *req)
{
    return false;
}

// pr_inj_trigger_rand_hit
// -----------------------
//
bool
ProbeMod::pr_inj_trigger_rand_hit(probe_point_e inj, ProbeRequest *req)
{
    return false;
}

// pr_inj_trigger_freq_hit
// -----------------------
//
bool
ProbeMod::pr_inj_trigger_freq_hit(probe_point_e inj, ProbeRequest *req)
{
    return false;
}

// pr_inj_trigger_payload
// ----------------------
//
bool
ProbeMod::pr_inj_trigger_payload(probe_point_e inj, ProbeRequest *req)
{
    return false;
}

// pr_inj_trigger_io_attr
// ----------------------
//
bool
ProbeMod::pr_inj_trigger_io_attr(probe_point_e inj, ProbeRequest *req)
{
    return false;
}

// pr_inj_act_bailout
// ------------------
//
void
ProbeMod::pr_inj_act_bailout(ProbeRequest *req)
{
}

// pr_inj_act_panic
// ----------------
//
void
ProbeMod::pr_inj_act_panic(ProbeRequest *req)
{
}

// pr_inj_act_delay
// ----------------
//
void
ProbeMod::pr_inj_act_delay(ProbeRequest *req)
{
}

// pr_inj_act_corrupt
// ------------------
//
void
ProbeMod::pr_inj_act_corrupt(ProbeRequest *req)
{
}

// ----------------------------------------------------------------------------

ProbeIORequest::ProbeIORequest(int          stat_cnt,
                               size_t       buf_siz,
                               const char   *wr_buf,
                               ProbeMod     *mod,
                               ObjectID     *oid,
                               fds_uint64_t off,
                               fds_uint64_t vid,
                               fds_uint64_t voff)
    : ProbeRequest(stat_cnt, 0, mod),
      pr_oid(*oid), pr_vid(vid), pr_offset(off), pr_voff(voff),
      pr_wr_size(buf_siz), pr_wr_buf(wr_buf)
{
    if (wr_buf == nullptr) {
        (pr_buf.data)->reserve(buf_siz);
    }
}

// pr_alloc_req
// ------------
//
ProbeIORequest *
ProbeMod::pr_alloc_req(ObjectID      *oid,
                       fds_uint64_t  off,
                       fds_uint64_t  vid,
                       fds_uint64_t  voff,
                       size_t        buf_siz,
                       const char    *buf)
{
    return new ProbeIORequest(0, buf_siz, buf, this, oid, off, vid, voff);
}

}  // namespace fds
