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

    virtual int  ame_register_ctx();
    virtual void ame_unregister_ctx();
    virtual void ame_notify_handler();
    virtual void ame_invoke_handler();
    virtual void ame_free_context();

    // Map buffer from nginx for AMEngine module to use with fdsn.
    //
    ame_buf_t *ame_alloc_buf(int len, char **buf, int *got);
    inline char *ame_buf_info(ame_buf_t *buf, int *len)
    {
        *len = buf->last - buf->pos;
        return (char *)buf->pos;
    }

    // Simple input buffer iteration to keep track of where we are with input
    // buffers.
    //
    inline void ame_save_input_buf(ame_chain_t *chain)
    {
        ame_in_chain = chain;
    }
    inline char *ame_curr_input_buf(int *len)
    {
        if (ame_in_chain != NULL) {
            return ame_buf_info(ame_in_chain->buf, len);
        }
        *len = 0;
        return NULL;
    }
    inline bool ame_next_input_buf()
    {
        if (ame_in_chain != NULL) {
            ame_in_chain = ame_in_chain->next;
        }
        return ame_in_chain != NULL ? true : false;
    }

    // Simple output buffer iteration to keep track of where we are with output
    // buffers.
    //
    inline void ame_push_output_buf(ame_buf_t *buf)
    {
        ame_out_buf = buf;
    }
    inline char *ame_curr_output_buf(ame_buf_t **buf, int *len)
    {
        *buf = ame_out_buf;
        if (ame_out_buf != NULL) {
            return ame_buf_info(ame_out_buf, len);
        }
        *len = 0;
        return NULL;
    }
    // This call must be made only by nginx event loop thread.
    void ame_pop_output_buf();

    // Temp. code, will remove
    char                     *ame_temp_buf;

  protected:
    AME_Ctx                  *ame_next;
    AME_Request              *ame_req;
    int                       ame_state;

    // Hookup with nginx event loop.
    //
    int                       ame_epoll_fd;
    ngx_connection_t         *ame_connect;

    ame_buf_t                *ame_out_buf;
    ame_chain_t              *ame_in_chain;
    ame_chain_t               ame_output;
    ame_chain_t               ame_free_buf;

    friend class AME_CtxList;
    virtual ~AME_Ctx() {}
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
