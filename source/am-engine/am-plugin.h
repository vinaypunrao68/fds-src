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
    void ame_register_handler(void (*handler)(AME_Request *req));
    void ame_notify_handler();
    void ame_unregister_ctx();

  protected:
    AME_Ctx                  *ame_next;
    AME_Request              *ame_req;
    ngx_http_request_t       *ame_ngx_req;
    void                    (*ame_handler_fn)(AME_Request *req);
    int                       ame_epoll_fd;

    friend class AME_CtxList;
    ~AME_Ctx() {}
};

class AME_CtxList
{
  public:
    AME_CtxList(int elm);

    AME_Ctx *ame_get_ctx();
    void     ame_put_ctx(AME_Ctx *ctx);
  private:
    int                       ame_free_ctx_cnt;
    AME_Ctx                  *ame_arr_ctx;
    AME_Ctx                  *ame_free_ctx;

    ~AME_CtxList() {}
};

} // namespace fds

#endif /* INCLUDE_AM_ENGINE_AM_PLUGIN_H_ */
