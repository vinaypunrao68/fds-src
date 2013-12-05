/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef INCLUDE_AM_ENGINE_AM_PLUGIN_H_
#define INCLUDE_AM_ENGINE_AM_PLUGIN_H_

#include <am-engine/am-engine.h>

extern "C" {
#include <ngx_http.h>

extern void ngx_register_plugin(fds::AMEngine *engine);
extern int  ngx_main(int argc, char const **argv);
extern void ngx_ame_handler(ngx_http_request_t *r);
}

namespace fds {

class AME_CtxList;
class AME_Ctx
{
  public:
    AME_Ctx();

    void ame_register_ctx();
    void ame_register_handler(void (*handler)(AME_Ctx *req));
    void ame_notify_handler();
    void ame_unregister_ctx();

  protected:
    // TODO: check if we need to abstract this class with pure handlers to
    // handle event's loop stop/resume model.
    //
    AME_Ctx                  *ame_next;
    AME_Request              *ame_req;
    void                    (*ame_handler_fn)(AME_Ctx *ctx);
    int                       ame_state;

    // Hookup with nginx event loop.
    //
    int                       ame_epoll_fd;
    ngx_connection_t         *ame_connect;
    ngx_chain_t              *ame_out_bufs;
    ngx_chain_t              *ame_busy_bufs;
    ngx_chain_t              *ame_free_bufs;

    friend class AME_CtxList;
    ~AME_Ctx() {}
    void ame_init_ngx_ctx();
};

class AME_CtxList
{
  public:
    AME_CtxList(int elm);

    AME_Ctx *ame_get_ctx(AME_Request *req);
    void     ame_put_ctx(AME_Ctx *ctx);
  private:
    int                       ame_free_ctx_cnt;
    AME_Ctx                  *ame_arr_ctx;
    AME_Ctx                  *ame_free_ctx;

    ~AME_CtxList() {}
};

} // namespace fds

#endif /* INCLUDE_AM_ENGINE_AM_PLUGIN_H_ */
