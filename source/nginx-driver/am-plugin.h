/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef INCLUDE_NGINX_DRIVER_AM_PLUGIN_H_
#define INCLUDE_NGINX_DRIVER_AM_PLUGIN_H_

#include <unordered_map>

#include <fds_assert.h>
#include <nginx-driver/am-engine.h>
#include <concurrency/Mutex.h>

extern "C" {
#include <ngx_http.h>
#include <ngx_files.h>

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

                // Move to the next chain buffer to read any remaining data
                fds_bool_t moved = ame_next_input_buf();
                if (moved == true) {
                    LOGDEBUG << "Moved buf ptr to next chain link";
                }
            } else {
                // Move the next buffer read forward by len
                ame_in_chain->buf->pos += (*len);
            }
        } else {
            *len = 0;
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

    typedef enum {OK, FAILED, WAITING, SENT} AME_Ctx_Ack;
    /// Defines how much data was requested by
    /// the connector. ALL means no specific size,
    /// just all of the data. SPECIFIC means a
    /// specific length was requested
    typedef enum {ALL, SPECIFIC} AME_Ctx_Get_Len;

  protected:
    AME_Ctx                  *ame_next;
    AME_Request              *ame_req;
    int                       ame_state;

    /// Protects the offset and map members
    fds_mutex                *ame_map_lock;
    /// Tracks current offset in to a stream
    fds_off_t                 ame_buf_off;
    typedef std::map<fds_off_t, AME_Ctx_Ack> AME_Ack_Map;
    /// Track which offsets we've received
    /// acks for
    AME_Ack_Map               ame_req_map;

    /*
     * Contextual fields for put()
     */
    /// Tracks all puts requests that need to be
    /// sent/received to fulfill the request
    fds_uint32_t              ame_ack_count;
    typedef std::pair<char *, fds_uint32_t> AME_Buf_Desc;
    typedef std::list<AME_Buf_Desc> AME_Alloc_List;
    /// Tracks buffers AME allocated to free later
    AME_Alloc_List            ame_alloc_bufs;

    /*
     * Contextual fields for get()
     */
    /// Tracks if the header was sent or not
    fds_bool_t                ame_header_sent;
    typedef std::pair<ame_buf_t *, fds_uint32_t> AME_Get_Desc;
    typedef std::unordered_map<fds_off_t, AME_Get_Desc> AME_Buf_Map;
    /// Tracks AME buffers used for gets by their offset
    /// Also protected by the above map_lock
    AME_Buf_Map               ame_get_bufs;
    /// Tracks the total bytes received so far
    /// for a get request
    fds_uint64_t              ame_received_get_len;
    /// Tracks whether a specific length that was requested
    AME_Ctx_Get_Len           ame_req_get_range;
    /// Tracks the specific length in bytes
    fds_uint64_t              ame_req_get_len;

    /// Request etag
    ngx_md5_t                 ame_etag_ctx;
    std::string               ame_etag_result;

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
    /*
     * Contextual locking for external objects
     */
    void ame_ctx_lock();
    void ame_ctx_unlock();

    /*
     * Contextual functions for tracking
     * outstanding request state
     */
    Error ame_add_ctx_req(fds_off_t offset);
    Error ame_upd_ctx_req(fds_off_t offset,
                          AME_Ctx_Ack ack_status);
    Error ame_upd_ctx_req_locked(fds_off_t offset,
                                 AME_Ctx_Ack ack_status);
    AME_Ctx_Ack ame_check_status();
    AME_Ctx_Ack ame_check_status_locked();
    void set_ack_count(fds_uint32_t count);
    Error ame_get_unsent_offset(fds_off_t *offset,
                                fds_bool_t *last);
    Error ame_get_unsent_offset_locked(fds_off_t *offset,
                                       fds_bool_t *last);

    /*
     * Contextual functions for tracking offsets
     * into a stream
     */
    void ame_mv_off(fds_uint32_t len);
    fds_off_t ame_get_off() const;

    /*
     * Contextual functions for etag management
     */
    ngx_md5_t *ame_get_etag();

    /*
     * Contextual functions to track buffers we
     * allocated for puts
     */
    void ame_add_alloc_buf(char *buf, fds_uint32_t len);
    void ame_free_bufs();

    /*
     * Contextual functions for tracking buffers,
     * offsets, and lengths received for gets
     */
    void ame_set_request_all();
    fds_bool_t ame_set_specific(fds_uint64_t len);
    fds_uint64_t ame_get_requested_len();
    fds_uint64_t ame_get_requested_len_locked();
    void ame_add_get_buf(fds_off_t offset,
                         ame_buf_t *ame_buf);
    void ame_set_get_len(fds_off_t offset,
                         fds_uint32_t len);
    fds_uint64_t ame_get_received_len() const;
    void ame_get_get_info(fds_off_t    offset,
                          ame_buf_t    **ame_buf,
                          fds_uint32_t *len);
    void ame_get_get_info_locked(fds_off_t    offset,
                                 ame_buf_t    **ame_buf,
                                 fds_uint32_t *len);

    /*
     * Contextual functions for header management
     */
    fds_bool_t ame_get_header_sent() const;
    fds_bool_t ame_get_header_sent_locked() const;
    void ame_set_header_sent(fds_bool_t sent);
    void ame_set_header_sent_locked(fds_bool_t sent);
    void ame_set_etag_result(const std::string &etag);
    std::string ame_get_etag_result_locked();
    std::string ame_get_etag_result();
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

#endif /* INCLUDE_NGINX_DRIVER_AM_PLUGIN_H_ */
