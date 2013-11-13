/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef INCLUDE_NET_PROXY_VOL_RPC_IO_H_
#define INCLUDE_NET_PROXY_VOL_RPC_IO_H_

#include <net-proxies/iomsg.h>
#include <string>
#include <fds_assert.h>
#include <fds_module.h>

struct SVCXPRT;

namespace fds {

typedef unsigned int          rpc_msg_id_t;

// -----------------------------------------------------------------------------
// Abstract client-side protocol class for FDS Probe's data path.
// -----------------------------------------------------------------------------
class ProbeIOClient
{
  public:
    ProbeIOClient(std::string serv_id) : net_serv_id(serv_id) {}
    ~ProbeIOClient() {}

    virtual bool vclnt_obj_get(obj_io_req_t *req, obj_io_resp_t *resp) = 0;
    virtual bool vclnt_obj_put(obj_io_req_t *req, obj_io_resp_t *resp) = 0;
  protected:
    std::string              net_serv_id;
};

// Abstract server-side protocol class for data path's volume IO RPC.
// Server-side async RPC adapter.  XXX: not in use.
//
class ProbeIOServerAsync
{
  public:
    ProbeIOServerAsync() {}
    ~ProbeIOServerAsync() {}

    virtual bool vsvc_obj_response(rpc_msg_id_t id, obj_io_resp_t *resp) = 0;
};

// Server-side processing function & async RPC adapter.
//
class ProbeIOServer
{
  public:
    ProbeIOServer(std::string svc_name) : net_svc_name(svc_name) {}
    ~ProbeIOServer() {}

    virtual bool
    vsvc_obj_get(rpc_msg_id_t id, obj_io_req_t *req, obj_io_resp_t *resp) = 0;

    virtual bool
    vsvc_obj_put(rpc_msg_id_t id, obj_io_req_t *req, obj_io_resp_t *resp) = 0;

    // Register async RPC adapter.
    //
    inline void
    vsvc_register_async(ProbeIOServerAsync *async)
    {
        net_svc_async = async;
    }
    // Common async RPC response.
    //
    inline bool
    vsvc_obj_response(rpc_msg_id_t id, obj_io_resp_t *resp)
    {
        fds_assert(net_svc_async != nullptr);
        return net_svc_async->vsvc_obj_response(id, resp);
    }

  protected:
    std::string              net_svc_name;
    ProbeIOServerAsync       *net_svc_async;
};

// -----------------------------------------------------------------------------
// Abstract client-side protocol class for FDS Probe's control path.
// -----------------------------------------------------------------------------
class ProbeAdminClient
{
  public:
    ProbeAdminClient(std::string serv_id) : net_serv_id(serv_id) {}
    ~ProbeAdminClient() {}

    virtual bool vclnt_vol_create(vol_create_t *req, obj_ctrl_resp_t *resp) = 0;
    virtual bool vclnt_vol_delete(vol_delete_t *req, obj_ctrl_resp_t *resp) = 0;
  protected:
    std::string              net_serv_id;
};

// Abstract server-side protocol class for admin path's volume RPC.
//
class ProbeAdminServer
{
  public:
    ProbeAdminServer(std::string svc_name) : net_svc_name(svc_name) {}
    ~ProbeAdminServer() {}

    virtual bool
    vsvc_vol_create(rpc_msg_id_t, vol_create_t *, obj_ctrl_resp_t *) = 0;

    virtual bool
    vsvc_vol_delete(rpc_msg_id_t, vol_delete_t *, obj_ctrl_resp_t *) = 0;

  protected:
    std::string              net_svc_name;
};

// -----------------------------------------------------------------------------
// Transport specific adapters
// -----------------------------------------------------------------------------

// Sun's ONC RPC IO client adapter.
//
class RPC_ProbeIOClient : public ProbeIOClient
{
  public:
    RPC_ProbeIOClient(std::string serv_id);
    ~RPC_ProbeIOClient();

    bool vrpc_obj_get(obj_io_req_t *req, obj_io_resp_t *resp);
    bool vrpc_obj_put(obj_io_req_t *req, obj_io_resp_t *resp);

  protected:
    CLIENT                   *net_clnt;
};

// Sun's ONC RPC IO server adapter.
//
class ProbeIOServerModule;
class RPC_ProbeIOServer
{
  public:
    RPC_ProbeIOServer(std::string svc_name, ProbeIOServer &server);
    ~RPC_ProbeIOServer();

    // vol_io_run
    // ----------
    // Wrapper for threadpool interface.
    //
    static void vol_io_run(RPC_ProbeIOServer *server);

  protected:
    std::string              net_svc_name;
    ProbeIOServer            &net_server;
    SVCXPRT                  *net_transp;

  private:
    void probe_io_server();
};

// Sun's ONC RCP admin client adapter.
//
class RPC_ProbeAdminClient : ProbeAdminClient
{
  public:
    RPC_ProbeAdminClient(std::string serv_id);
    ~RPC_ProbeAdminClient();

    virtual bool vclnt_vol_create(vol_create_t *req, obj_ctrl_resp_t *resp);
    virtual bool vclnt_vol_delete(vol_delete_t *req, obj_ctrl_resp_t *resp);

  protected:
    CLIENT                   *net_clnt;
};

// Sun's ONC RPC admin server adapter.
//
class RPC_ProbeAdminServer
{
  public:
    RPC_ProbeAdminServer(std::string svc_name, ProbeAdminServer &server);
    ~RPC_ProbeAdminServer();

  protected:
    std::string              net_svc_name;
    ProbeAdminServer         &net_server;
    SVCXPRT                  *net_transp;

  private:
    void probe_admin_server();
};

// -----------------------------------------------------------------------------
// FDS Probe server and client modules in data path.
// -----------------------------------------------------------------------------
class ProbeIOClientModule : public virtual Module
{
  public:
    ProbeIOClientModule(char const *const name);
    ~ProbeIOClientModule();

    virtual int  mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

    ProbeIOClient            *net_clnt;
  private:
    RPC_ProbeIOClient        *net_rpc;
};

class ProbeIOServerModule : public virtual Module
{
  public:
    ProbeIOServerModule(char const *const name);
    ~ProbeIOServerModule();

    virtual int  mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

    ProbeIOServer              *net_server;

  private:
    RPC_ProbeIOServer          *net_rpc;
};

// -----------------------------------------------------------------------------
// FDS Probe server and client modules in admin path.
// -----------------------------------------------------------------------------
class ProbeAdminClientModule : public virtual Module
{
  public:
    ProbeAdminClientModule(char const *const name);
    ~ProbeAdminClientModule();

    virtual int  mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

  private:
};

class ProbeAdminServerModule : public virtual Module
{
  public:
    ProbeAdminServerModule(char const *const name);
    ~ProbeAdminServerModule();

    virtual int  mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

    ProbeAdminServer         *net_server;

  private:
    RPC_ProbeIOServer        *net_rpc;
};

extern ProbeIOClientModule     gl_probeIOClntMod;
extern ProbeIOServerModule     gl_probeIOServMod;
extern ProbeAdminClientModule  gl_probeAdminClntMod;
extern ProbeAdminServerModule  gl_probeAdminServMod;

} // namespace fds

#endif /* INCLUDE_NET_PROXY_VOL_RPC_IO_H_ */
