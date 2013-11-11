/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <fds_assert.h>
#include <net-proxies/vol_rpc_io.h>

namespace fds {

// ----------------------------------------------------------------------------
//
ProbeIOClientModule            gl_probeIOClntMod("Probe IO RPC Client Module");

ProbeIOClientModule::ProbeIOClientModule(char const *const name)
    : Module(name)
{
}

ProbeIOClientModule::~ProbeIOClientModule()
{
}

// mod_init
// --------
//
int
ProbeIOClientModule::mod_init(SysParams const *const param)
{
    return 0;
}

// mod_startup
// -----------
//
void
ProbeIOClientModule::mod_startup()
{
}

// mod_shutdown
// ------------
//
void
ProbeIOClientModule::mod_shutdown()
{
}

// ----------------------------------------------------------------------------
//
ProbeAdminClientModule         gl_probeAdminClntMod("Probe Admin RPC Client");

ProbeAdminClientModule::ProbeAdminClientModule(char const *const name)
    : Module(name)
{
}

ProbeAdminClientModule::~ProbeAdminClientModule()
{
}

int
ProbeAdminClientModule::mod_init(SysParams const *const param)
{
    return 0;
}

void
ProbeAdminClientModule::mod_startup()
{
}

void
ProbeAdminClientModule::mod_shutdown()
{
}

// ----------------------------------------------------------------------------
//
RPC_ProbeIOClient::RPC_ProbeIOClient(std::string serv_id)
    : ProbeIOClient(serv_id)
{
    net_clnt = clnt_create(serv_id.c_str(), OBJECT_SERVER, OBJ_SRV_VERS, "udp");
    fds_verify(net_clnt != nullptr);
}

RPC_ProbeIOClient::~RPC_ProbeIOClient()
{
    clnt_destroy(net_clnt);
}

// vrpc_obj_get
// ------------
//
bool
RPC_ProbeIOClient::vrpc_obj_get(obj_io_req_t *req, obj_io_resp_t *resp)
{
    if (obj_get_1(req, resp, net_clnt) == RPC_SUCCESS) {
        return true;
    }
    return false;
}

// vrpc_obj_put
// ------------
//
bool
RPC_ProbeIOClient::vrpc_obj_put(obj_io_req_t *req, obj_io_resp_t *resp)
{
    if (obj_put_1(req, resp, net_clnt) == RPC_SUCCESS) {
        return true;
    }
    return false;
}

} // namespace fds
