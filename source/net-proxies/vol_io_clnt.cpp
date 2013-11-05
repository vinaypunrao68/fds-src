/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <fds_assert.h>
#include <net-proxies/vol_rpc_io.h>

namespace fds {

// ----------------------------------------------------------------------------
//
VolIOClientModule     gl_volIOClntMod("Vol IO RPC Client Module");

VolIOClientModule::VolIOClientModule(char const *const name)
    : Module(name)
{
}

VolIOClientModule::~VolIOClientModule()
{
}

// mod_init
// --------
//
int
VolIOClientModule::mod_init(SysParams const *const param)
{
    return 0;
}

// mod_startup
// -----------
//
void
VolIOClientModule::mod_startup()
{
}

// mod_shutdown
// ------------
//
void
VolIOClientModule::mod_shutdown()
{
}

// ----------------------------------------------------------------------------
//
VolAdminClientModule         gl_volAdminClntMod("Vol Admin RPC Client");

// ----------------------------------------------------------------------------
//
RPC_VolIOClient::RPC_VolIOClient(std::string serv_id)
    : VolIOClient(serv_id)
{
    net_clnt = clnt_create(serv_id.c_str(), OBJECT_SERVER, OBJ_SRV_VERS, "udp");
    fds_verify(net_clnt != nullptr);
}

RPC_VolIOClient::~RPC_VolIOClient()
{
    clnt_destroy(net_clnt);
}

// vrpc_obj_get
// ------------
//
bool
RPC_VolIOClient::vrpc_obj_get(obj_io_req_t *req, obj_io_resp_t *resp)
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
RPC_VolIOClient::vrpc_obj_put(obj_io_req_t *req, obj_io_resp_t *resp)
{
    if (obj_put_1(req, resp, net_clnt) == RPC_SUCCESS) {
        return true;
    }
    return false;
}

} // namespace fds
