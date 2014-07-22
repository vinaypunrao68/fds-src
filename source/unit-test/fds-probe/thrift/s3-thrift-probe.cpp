/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 */
#include <vector>
#include <s3-thrift-probe.h>
#include <string>
#include <iostream>
#include <fds_assert.h>
#include <fds_typedefs.h>
#include <endpoint-test.h>
#include <net/SvcRequestPool.h>
#include <fdsp/fds_service_types.h>
#include <util/Log.h>

using namespace ::fpi;                 // NOLINT

namespace fds {

static probe_mod_param_t thrift_probe_param =
{
    .pr_stat_cnt     = 0,
    .pr_inj_pts_cnt  = 0,
    .pr_inj_act_cnt  = 0,
    .pr_max_sec_tout = 0
};

Thrift_ProbeMod gl_thriftProbeMod("Thrift Probe Adapter",
                                  &thrift_probe_param, nullptr);

// pr_new_instance
// ---------------
//
Thrift_ProbeMod *
Thrift_ProbeMod::pr_new_instance()
{
    Thrift_ProbeMod *adapter =
        new Thrift_ProbeMod("Probe Adapter Inst", &thrift_probe_param, nullptr);

    adapter->mod_init(mod_params);
    adapter->mod_startup();
    return adapter;
}

// pr_intercept_request
// --------------------
//
void
Thrift_ProbeMod::pr_intercept_request(ProbeRequest *req)
{
}

// pr_put
// ------
//
void
Thrift_ProbeMod::pr_put(ProbeRequest *req)
{
    ProbeIORequest    *io;
    fpi::SvcUuid       peer;
    boost::shared_ptr<fpi::PutObjectMsg>  put;

    io  = static_cast<ProbeIORequest *>(req);
    peer.svc_uuid = 0xabcdef;
    auto rpc = gSvcRequestPool->newEPSvcRequest(peer);

    put.reset(new fpi::PutObjectMsg());
    rpc->setPayload(FDSP_MSG_TYPEID(fpi::PutObjectMsg), put);
    rpc->invoke();
}

// pr_get
// ------
//
void
Thrift_ProbeMod::pr_get(ProbeRequest *req)
{
    ProbeIORequest    *io;
    fpi::SvcUuid       peer;
    boost::shared_ptr<fpi::GetObjectMsg>  get;

    io = static_cast<ProbeIORequest *>(req);
    peer.svc_uuid = 0xabcdef;
    auto rpc = gSvcRequestPool->newEPSvcRequest(peer);

    get.reset(new fpi::GetObjectMsg());
    rpc->setPayload(FDSP_MSG_TYPEID(fpi::GetObjectMsg), get);
    rpc->invoke();
}

// pr_delete
// ---------
//
void
Thrift_ProbeMod::pr_delete(ProbeRequest *req)
{
}

// pr_verify_request
// -----------------
//
void
Thrift_ProbeMod::pr_verify_request(ProbeRequest *req)
{
}

// pr_gen_report
// -------------
//
void
Thrift_ProbeMod::pr_gen_report(std::string *out)
{
}

// mod_init
// --------
//
int
Thrift_ProbeMod::mod_init(SysParams const *const param)
{
    Module::mod_init(param);
    return 0;
}

// mod_startup
// -----------
//
void
Thrift_ProbeMod::mod_startup()
{
    JsObjManager  *mgr;

    if (pr_parent != NULL) {
        mgr = pr_parent->pr_get_obj_mgr();
        mgr->js_register_template(new ProbeAmApiTempl(mgr));
    }
}

// mod_shutdown
// ------------
//
void
Thrift_ProbeMod::mod_shutdown()
{
}

void successCb(boost::shared_ptr<std::string> resp) {
    GLOGNORMAL;
    std::cout << "On sucess callback" << std::endl;
}

void errorCb(const Error &e, boost::shared_ptr<std::string> resp) {
    GLOGNORMAL;
    std::cout << "On error callback" << std::endl;
}
/*
 * ------------------------------------------------------------------------------------
 * Probe Control Path
 * ------------------------------------------------------------------------------------
 */
JsObject *
ProbeAmFoo::js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out)
{
    am_foo_arg_t *p = am_probe_foo();

    return this;
}

JsObject *
ProbeAmGetReqt::js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out)
{
    fpi::SvcUuid      peer;
    am_getmsg_reqt_t *p = am_getmsg_reqt();
    boost::shared_ptr<fpi::GetObjectMsg>  get;

    peer.svc_uuid = 0xabcdef;
    auto rpc = gSvcRequestPool->newEPSvcRequest(peer);

    get.reset(new fpi::GetObjectMsg());
    rpc->setPayload(FDSP_MSG_TYPEID(fpi::GetObjectMsg), get);
    rpc->invoke();
    return this;
}

JsObject *
ProbeAmBar::js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out)
{
    am_bar_arg_t *p = am_probe_bar();

    return this;
}

JsObject *
ProbeAmPutMsg::js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out)
{
    fpi::SvcUuid      peer;
    am_putmsg_arg_t  *p = am_probe_putmsg();
    boost::shared_ptr<fpi::PutObjectMsg>  put;

    peer.svc_uuid = 0xabcdef;
    auto rpc = gSvcRequestPool->newEPSvcRequest(peer);

    put.reset(new fpi::PutObjectMsg());
    put->data_obj.reserve(p->msg_len);
    put->data_obj.resize(p->msg_len);
    put->origin_timestamp = 0xdeaddeadbeefbeef;
    put->data_obj_len     = p->msg_len;
    put->volume_id        = 0xcafeefac;

    rpc->setPayload(FDSP_MSG_TYPEID(fpi::PutObjectMsg), put);
    rpc->invoke();

    return this;
}

}  // namespace fds
