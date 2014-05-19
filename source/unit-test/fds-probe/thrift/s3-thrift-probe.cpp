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
#include <net/RpcRequestPool.h>
#include <ProbeServiceSM.h>
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
    ProbeIORequest *io;

    io = static_cast<ProbeIORequest *>(req);
}

// pr_get
// ------
//
void
Thrift_ProbeMod::pr_get(ProbeRequest *req)
{
    ProbeIORequest  *io;

    io = static_cast<ProbeIORequest *>(req);
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
}

void errorCb(const Error &e, boost::shared_ptr<std::string> resp) {
    GLOGNORMAL;
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

    RpcRequestPool pool;
    fpi::SvcUuid myEpId;
    myEpId.svc_uuid = 0xabcdef;
    std::vector<fpi::SvcUuid> uuid_list;
    fpi::SvcUuid id1;
    fpi::SvcUuid id2;
    fpi::SvcUuid id3;
    fpi::SvcUuid id4;
    id1.svc_uuid = 0x1;
    id2.svc_uuid = 0x2;
    id3.svc_uuid = 0x3;
    id4.svc_uuid = 0xfedcba;

    uuid_list.push_back(id1);
    uuid_list.push_back(id2);
    uuid_list.push_back(id3);
    uuid_list.push_back(id4);

    boost::shared_ptr<fpi::ProbeGetMsgResp> arg1(new fpi::ProbeGetMsgResp());

    auto failoverReq = pool.newFailoverRpcRequest<fpi::ProbeServiceAMClient>(
        myEpId,
        uuid_list,
        static_cast<void(ProbeServiceAMClient::*)(boost::shared_ptr<fpi::ProbeGetMsgResp>&)>(&fpi::ProbeServiceAMClient::am_probe_put_resp), // NOLINT
            arg1);
    failoverReq->onSuccessCb(successCb);
    failoverReq->onErrorCb(errorCb);

    failoverReq->invoke();

    uuid_list.pop_back();
    auto errorReq = pool.newFailoverRpcRequest<fpi::ProbeServiceAMClient>(
        myEpId,
        uuid_list,
        static_cast<void(ProbeServiceAMClient::*)(boost::shared_ptr<fpi::ProbeGetMsgResp>&)>(&fpi::ProbeServiceAMClient::am_probe_put_resp), // NOLINT
            arg1);
    errorReq->onSuccessCb(successCb);
    errorReq->onErrorCb(errorCb);
    errorReq->invoke();

    std::cout << "In foo func " << p->am_func << std::endl;
    return this;
}

JsObject *
ProbeAmGetReqt::js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out)
{
    am_getmsg_reqt_t*p = am_getmsg_reqt();

    std::cout << "In GetReqt func " << p->am_func << std::endl;
    return this;
}

JsObject *
ProbeAmBar::js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out)
{
    am_bar_arg_t *p = am_probe_bar();

    std::cout << "In Bar func " << p->am_func << std::endl;
    return this;
}

JsObject *
ProbeAmPutMsg::js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out)
{
    am_putmsg_arg_t *p = am_probe_putmsg();

    std::cout << "In PutMsg func " << p->am_func << std::endl;
    return this;
}

}  // namespace fds
