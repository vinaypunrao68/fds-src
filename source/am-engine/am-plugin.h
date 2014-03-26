/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef INCLUDE_AM_ENGINE_AM_PLUGIN_H_
#define INCLUDE_AM_ENGINE_AM_PLUGIN_H_

#include <unordered_map>

#include <fds_assert.h>
#include <am-engine/am-engine.h>
#include <concurrency/Mutex.h>

extern "C" {
#include <ngx_http.h>

extern void ngx_register_plugin(fds::FDS_NativeAPI::FDSN_ClientType clientType,
                                fds::AMEngine *engine);
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

    virtual fds_off_t ame_get_offset();

    // Map buffer from nginx for AMEngine module to use with fdsn.
    //
    ame_buf_t *ame_alloc_buf(int len, char **buf, int *got);

    inline char *ame_buf_info(ame_buf_t *buf, fds_uint32_t *len) {
        *len = buf->last - buf->pos;
        return (char *)buf->pos;
    }

    // Simple input buffer iteration to keep track of where we are with input
    // buffers.
    //
    inline void ame_save_input_buf(ame_chain_t *chain) {
        ame_in_chain = chain;
    }
    inline void ame_update_input_buf(int len) {
        // TODO: Not yet until we can handle chunk!
        // fds_verify(ame_in_chain != NULL);
        // fds_verify(ame_in_chain->buf != NULL);
        // ame_in_chain->buf->last += len;
        ame_temp_len = len;
    }

    /**
     * Gets our current offset into the buffer
     */
    inline char *ame_curr_input_buf(fds_uint32_t *len) {
        if (ame_in_chain != NULL) {
            return ame_buf_info(ame_in_chain->buf, len);
        }
        *len = 0;
        return NULL;
    }
    /**
     * Gets the current offset into the buffer and
     * moves the buffer pointer forward to next pos.
     *
     * @param[in/out] The size of the buffer to use. This
     * is also how far we move the pos ptr. If the
     * ptr cannot be moved len bytes, we set len to
     * how far we did move it.
     */
    inline char *ame_next_buf_ptr(fds_uint32_t *len) {
        fds_uint32_t remaining_len;
        char *buf_pos = ame_curr_input_buf(&remaining_len);
        if (buf_pos != NULL) {
            unsigned char *last  = ame_in_chain->buf->last;

            // Check if we're able to return len bytes
            if (last < ((unsigned char *)buf_pos + *len)) {
                *len = (last - (unsigned char *)buf_pos);
                fds_verify(*len == remaining_len);
                // The next buffer read will now just read
                // the end
                ame_in_chain->buf->pos = last;
            } else {
                // Move the next buffer read forward by len
                ame_in_chain->buf->pos += (*len);
            }
        }

        return buf_pos;
    }

    /**
     * Moves the chain to the next buffer
     */
    inline bool ame_next_input_buf() {
        if (ame_in_chain != NULL) {
            ame_in_chain = ame_in_chain->next;
        }
        return ame_in_chain != NULL ? true : false;
    }

    // Simple output buffer iteration to keep track of where we are with output
    // buffers.
    //
    inline void ame_push_output_buf(ame_buf_t *buf) {
        ame_out_buf = buf;
    }
    inline void ame_update_output_buf(int len) {
        // TODO: better buffer chunking API.
        ame_temp_len = len;
    }
    inline char *ame_curr_output_buf(ame_buf_t **buf, fds_uint32_t *len) {
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
    int                       ame_temp_len;

    typedef enum {OK, FAILED, WAITING} AME_Ctx_Ack;

  protected:
    AME_Ctx                  *ame_next;
    AME_Request              *ame_req;
    int                       ame_state;

    /// Protects the offset and map members
    fds_mutex                *ame_map_lock;
    /// Track which offsets we've received
    /// acks for
    fds_off_t                 ame_buf_off;
    typedef std::unordered_map<fds_off_t, AME_Ctx_Ack> AME_Ack_Map;
    AME_Ack_Map               ame_req_map;
    fds_uint32_t              ame_ack_count;

    // Hookup with nginx event loop.
    //
    int                       ame_epoll_fd;
    ngx_connection_t         *ame_connect;

    ame_buf_t                *ame_out_buf;
    ame_chain_t              *ame_in_chain;
    ame_chain_t               ame_output;
    ame_chain_t               ame_free_buf;

    friend class AME_CtxList;
    virtual ~AME_Ctx() {
        delete ame_map_lock;
    }
    void ame_init_ngx_ctx();

  public:
    Error ame_add_ctx_req(fds_off_t offset);
    Error ame_upd_ctx_req(fds_off_t offset,
                          AME_Ctx_Ack ack_status);
    AME_Ctx_Ack ame_check_status();
    void set_ack_count(fds_uint32_t count);
    void ame_mv_off(fds_uint32_t len);
    fds_off_t ame_get_off() const;
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

/**
 * Main entry to parse URI to implement FDS's REST API.
 * @param eng (i) - the engine to provide factory methods to create the
 *    right object.
 * @param req (i) - the raw http request received.
 * @return object that can handle the request.
 */
extern AME_Request *
ame_http_parse_url(AMEngine *eng, AME_HttpReq *req);

} // namespace fds

#endif /* INCLUDE_AM_ENGINE_AM_PLUGIN_H_ */
