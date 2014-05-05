/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 */
#include <s3-thrift-probe.h>
#include <string>
#include <iostream>
#include <fds_assert.h>

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
    ProbePutMsg     msg;
    ProbeIORequest *io;

    io = static_cast<ProbeIORequest *>(req);
    msg.msg_data.assign(io->pr_wr_buf, io->pr_wr_size);
    rpc_client->probe_put(msg);
}

// pr_get
// ------
//
void
Thrift_ProbeMod::pr_get(ProbeRequest *req)
{
    ProbeIORequest  *io;
    ProbeGetMsgReqt  reqt;
    ProbeGetMsgResp  resp;

    io = static_cast<ProbeIORequest *>(req);
    reqt.msg_len = io->pr_wr_size;
    rpc_client->probe_get(resp, reqt);

    fds_verify((uint)resp.msg_len == io->pr_wr_size);
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
    rpc_sock.reset(new TSocket("localhost", 9000));
    rpc_trans.reset(new TFramedTransport(rpc_sock));
    rpc_proto.reset(new TBinaryProtocol(rpc_trans));
    rpc_client = new ProbeServiceSMClient(rpc_proto);
    try {
        rpc_trans->open();
    } catch(TException &tx) {  // NOLINT
        std::cout << "Error: " << tx.what() << std::endl;
    }
}

// mod_shutdown
// ------------
//
void
Thrift_ProbeMod::mod_shutdown()
{
    if (rpc_client != NULL) {
        rpc_trans->close();
        delete rpc_client;
    }
}

}  // namespace fds
