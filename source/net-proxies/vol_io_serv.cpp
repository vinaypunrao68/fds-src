/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <net-proxies/vol_rpc_io.h>
#include <stdio.h>
#include <stdlib.h>
#include <rpc/pmap_clnt.h>
#include <string.h>
#include <memory.h>
#include <sys/socket.h>
#include <netinet/in.h>

extern void object_server_1(struct svc_req *rqstp, register SVCXPRT *transp);

namespace fds {

// ----------------------------------------------------------------------------
// Singleton volume server module.
//
ProbeIOServerModule     gl_probeIOServMod("Probe Server IO RPC");

ProbeIOServerModule::ProbeIOServerModule(char const *const name)
    : Module(name), net_server(nullptr)
{
}

ProbeIOServerModule::~ProbeIOServerModule()
{
}

// mod_init
// --------
//
int
ProbeIOServerModule::mod_init(SysParams const *const param)
{
    return 0;
}

// mod_startup
// -----------
//
void
ProbeIOServerModule::mod_startup()
{
}

// mod_shutdown
// ------------
//
void
ProbeIOServerModule::mod_shutdown()
{
}

// ----------------------------------------------------------------------------
//
RPC_ProbeIOServer::RPC_ProbeIOServer(std::string svc_name,
                                     ProbeIOServer &server)
    : net_svc_name(svc_name), net_server(server), net_transp(nullptr)
{
    pmap_unset(OBJECT_SERVER, OBJ_SRV_VERS);
    net_transp = svcudp_create(RPC_ANYSOCK);
    if (net_transp == nullptr) {
        perror("Failed to create UDP service: ");
        exit(1);
    }
    if (!svc_register(net_transp, OBJECT_SERVER,
                      OBJ_SRV_VERS, object_server_1, IPPROTO_UDP)) {
        perror("Failed to register volume IO service: ");
        exit(1);
    }
    gl_probeIOServMod.net_server = &server;
}

RPC_ProbeIOServer::~RPC_ProbeIOServer()
{
}

// vol_io_run
// ----------
//
void
RPC_ProbeIOServer::vol_io_run(RPC_ProbeIOServer *server)
{
    server->vol_io_server();
}

// vol_io_server
// -------------
//
void
RPC_ProbeIOServer::vol_io_server()
{
    svc_run();
}

// obj_get_1_svc
// -------------
//
bool
obj_get_1_svc(obj_io_req_t *argp, obj_io_resp_t *result, struct svc_req *rqstp)
{
    bool ret = gl_probeIOServMod.net_server->vsvc_obj_get(0, argp, result);
    return ret;
}

// obj_put_1_svc
// -------------
//
bool
obj_put_1_svc(obj_io_req_t *argp, obj_io_resp_t *result, struct svc_req *rqstp)
{
    bool ret = gl_probeIOServMod.net_server->vsvc_obj_put(0, argp, result);
    return ret;
}

// object_server_1_freeresult
// --------------------------
//
int
object_server_1_freeresult(SVCXPRT *transp, xdrproc_t xdr_result, caddr_t result)
{
    xdr_free(xdr_result, result);
    return 1;
}

} // namespace fds
