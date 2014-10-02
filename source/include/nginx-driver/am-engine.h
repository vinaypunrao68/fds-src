/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NGINX_DRIVER_AM_ENGINE_H_
#define SOURCE_INCLUDE_NGINX_DRIVER_AM_ENGINE_H_

#include <fds_module.h>
#include <fds_request.h>
#include <string>
#include <native_api.h>
#include <util/Log.h>
#include <nginx-driver/http_utils.h>
#include <json/json.h>

extern "C" {
struct ngx_buf_s;
struct ngx_chain_s;
struct ngx_http_request_s;

typedef struct ngx_buf_s          ame_buf_t;
typedef struct ngx_chain_s        ame_chain_t;
typedef struct ngx_http_request_s AME_HttpReq;
}

namespace fds {
class AME_Ctx;
class FDS_NativeAPI;
class Conn_GetObject;
class Conn_PutObject;
class Conn_DelObject;
class Conn_GetBucket;
class Conn_PutBucket;
class Conn_DelBucket;
class Conn_PutBucketParams;
class Conn_GetBucketStats;

class AMEngine : public Module
{
  public:
    explicit AMEngine(char const *const name) :
        Module(name), eng_signal(), ame_queue(1, 1000) {}
    ~AMEngine() {}

    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();
    void run_server(FDS_NativeAPI *api);

    virtual void init_server(FDS_NativeAPI *api);

    static void run_all_servers();

    // Factory methods to create objects handling required protocol.
    virtual Conn_GetObject *ame_getobj_hdler(AME_HttpReq *req) = 0;
    virtual Conn_PutObject *ame_putobj_hdler(AME_HttpReq *req) = 0;
    virtual Conn_DelObject *ame_delobj_hdler(AME_HttpReq *req) = 0;
    virtual Conn_GetBucket *ame_getbucket_hdler(AME_HttpReq *req) = 0;
    virtual Conn_PutBucket *ame_putbucket_hdler(AME_HttpReq *req) = 0;
    virtual Conn_DelBucket *ame_delbucket_hdler(AME_HttpReq *req) = 0;
    virtual Conn_PutBucketParams *ame_putbucketparams_hdler(AME_HttpReq *req);
    virtual Conn_GetBucketStats *ame_getbucketstats_hdler(AME_HttpReq *req);

    inline FDS_NativeAPI *ame_fds_hook() {
        return eng_api;
    }
    inline fdsio::RequestQueue *ame_get_queue() {
        return &ame_queue;
    }

  private:
    std::string              eng_signal;
    FDS_NativeAPI            *eng_api;
    fdsio::RequestQueue      ame_queue;
};

// ---------------------------------------------------------------------------

// Table of keys used request and response headers.
// Reference spec URL:
// http://docs.aws.amazon.com/AmazonS3/latest/API/
//   Common response: RESTCommonResponseHeaders.html
//   Bucket get     : RESTBucketGET.html
//
// Don't change the order because they're indices to the keytab table.
typedef enum
{
    // Common response keys
    RESP_CONTENT_LEN         = 0,
    RESP_CONTENT_TYPE        = 1,
    RESP_CONNECTION          = 2,
    RESP_CONNECTION_OPEN     = 3,
    RESP_CONNECTION_CLOSE    = 4,
    RESP_ETAG                = 5,
    RESP_DATE                = 6,
    RESP_SERVER              = 7,
    RESP_LOCATION            = 8,

    // RESTBucket response keys
    REST_LIST_BUCKET         = 9,
    REST_NAME                = 10,
    REST_PREFIX              = 11,
    REST_MARKER              = 12,
    REST_MAX_KEYS            = 13,
    REST_IS_TRUNCATED        = 14,
    REST_CONTENTS            = 15,
    REST_KEY                 = 16,
    REST_ETAG                = 17,
    REST_SIZE                = 18,
    REST_STORAGE_CLASS       = 19,
    REST_OWNER               = 20,
    REST_ID                  = 21,
    REST_DISPLAY_NAME        = 22,

    // Bucket Stats/Policy response keys -- our own
    RESP_QOS_PRIORITY        = 23,
    RESP_QOS_SLA             = 24,
    RESP_QOS_LIMIT           = 25,
    RESP_QOS_PERFORMANCE     = 26,
    RESP_STATS_TIME          = 27,
    RESP_STATS_VOLS          = 28,
    RESP_STATS_ID            = 29,

    AME_HDR_KEY_MAX
} ame_hdr_key_e;

typedef struct ame_keytab ame_keytab_t;
struct ame_keytab
{
    union {
        char const *const    kv_key;
        char                 *kv_key_name;
    } u;
    int                      kv_keylen;
    ame_hdr_key_e            kv_idx;
};

// Get the key and its length to fill in REST's header, indexed by its enum.
//
extern ame_keytab_t sgt_AMEKey[];

// ---------------------------------------------------------------------------
// Performance stat collection points.  These are indices to array.
//
enum
{
    STAT_NGX_GET             = 0,
    STAT_NGX_GET_FDSN        = 1,
    STAT_NGX_GET_FDSN_RET    = 2,
    STAT_NGX_GET_FDSN_CB     = 3,
    STAT_NGX_GET_RESUME      = 4,

    STAT_NGX_PUT             = 5,
    STAT_NGX_PUT_FDSN        = 6,
    STAT_NGX_PUT_FDSN_RET    = 7,
    STAT_NGX_PUT_FDSN_CB     = 8,
    STAT_NGX_PUT_RESUME      = 9,

    STAT_NGX_DEL             = 10,
    STAT_NGX_DEL_FDSN        = 11,
    STAT_NGX_DEL_FDSN_RET    = 12,
    STAT_NGX_DEL_FDSN_CB     = 13,
    STAT_NGX_DEL_RESUME      = 14,

    STAT_NGX_GET_BK          = 15,
    STAT_NGX_GET_BK_FDSN     = 16,
    STAT_NGX_GET_BK_FDSN_RET = 17,
    STAT_NGX_GET_BK_FDSN_CB  = 18,
    STAT_NGX_GET_BK_RESUME   = 19,

    STAT_NGX_PUT_BK          = 20,
    STAT_NGX_PUT_BK_FDSN     = 21,
    STAT_NGX_PUT_BK_FDSN_RET = 22,
    STAT_NGX_PUT_BK_FDSN_CB  = 23,
    STAT_NGX_PUT_BK_RESUME   = 24,

    STAT_NGX_DEL_BK          = 25,
    STAT_NGX_DEL_BK_FDSN     = 26,
    STAT_NGX_DEL_BK_FDSN_RET = 27,
    STAT_NGX_DEL_BK_FDSN_CB  = 28,
    STAT_NGX_DEL_BK_RESUME   = 29,

    STAT_NGX_DEFAULT         = 30,
    STAT_NGX_POINT_MAX       = 31
};

// ---------------------------------------------------------------------------
// Generic connector to handle request/response protocol with buffer chunks
// semantic
//
class AME_Request : public fdsio::Request
{
  public:
    // Clock time to record perf stat.
    int                      ame_stat_pt;
    fds_uint64_t             ame_clk_all;
    fds_uint64_t             ame_clk_fdsn;
    fds_uint64_t             ame_clk_fdsn_cb;

    static int ame_map_fdsn_status(FDSN_Status status);

  public:
    AME_Request(AMEngine *eng, AME_HttpReq *req);
    virtual ~AME_Request();

    // ame_add_context
    // ---------------
    // Add the context to keep track of internal states when this obj is moved
    // around the event loop.
    //
    void ame_add_context(AME_Ctx *ctx);
    inline AME_Ctx *ame_get_context() {
        return ame_ctx;
    }

    inline AMEngine *ame_get_ame() const {
        return ame;
    }

    // ame_get_reqt_hdr_val
    // --------------------
    // Return the value corresponding with the key in the request header.
    //
    char const *const ame_get_reqt_hdr_val(char const *const key);

    // ame_set_std_resp
    // ----------------
    // Common path to set the standard response status/len.
    //
    int ame_set_std_resp(int status, int len);

    // ame_set_resp_keyval
    // -------------------
    // Set key/value in the response to send to the client.
    // Key/value strings can be on the stack if the caller calls the method
    // ame_send_response_hdr() in the same context.
    //
    int ame_set_resp_keyval(char *k, int klen, char *v, int vlen);

    // ame_signal_resume
    // -----------------
    // Callback by engine driver to resume the event loop.  The event loop
    // is the loop of ame_request_handle() and ame_request_resume() until
    // the last response is done or ame_finalize_request() method is called
    // to cut short the stack with error status.
    //
    void ame_signal_resume(int status);

    void appendURIPart(const std::string &uri);

    virtual void ame_request_handler() = 0;
    virtual int  ame_request_resume() = 0;
    virtual int  ame_format_response_hdr() = 0;

  protected:
    friend class AME_Ctx;

    AMEngine                 *ame;
    AME_Ctx                  *ame_ctx;
    AME_HttpReq              *ame_req;
    struct ngx_chain_s       *post_buf_itr;
    HttpRequest              ame_http;
    int                      ame_resp_status;
    std::string              ame_etag;

    // Tell the engine if we need to finalize this request.
    fds_bool_t               ame_finalize;

    // Common request path.
    // The request handler is called through ame_request_handler().
    // Get request data:
    //    while (ame_reqt_iter_next() == AME_HAVE_DATA) {
    //        buf = ame_reqt_iter_data(&len);
    //        Valid request data is in buf, len.
    //    }
    // Get header data, key is case insensitive:
    //    host = ame_get_req_hdr_val("host");
    //    uri  = ame_get_req_hdr_val("uri");
    //
    void  ame_reqt_iter_reset();
    int   ame_reqt_iter_next();
    char *ame_reqt_iter_data(fds_uint32_t *len);

    char *ame_reqt_iter_data_next(fds_uint32_t data_len,
                                  fds_uint32_t *len);

    // Common response path.
    // The request handler then prepares response data.
    // Format key/value in response header:
    //    ame_set_resp_keyval("Content-Type", "application/xml");
    //    ...
    // Send partial/full response data:
    //    ame_send_response_hdr();
    //    ame_send_resp_data(cookie, valid_len, last_buf);
    //    If the last buf is true, this object will be freed by the AM Engine.
    //
    int   ame_send_resp_data(ame_buf_t *cookie, int len, fds_bool_t last);

    // Common send response path.
    // o The send_respone_hdr method is used to send header with data setup by
    //   other methods.  If there isn't any data expected in the header, it'll
    //   complete the response; otherwise, the engine needs the send_resp_data
    //   method above with 'last' == true to complete the response.
    // o The finalize_response method is used to send response back to client
    //   without any data.
    //
    int  ame_send_response_hdr();
    void ame_finalize_response(int status);

    // ame_finalize_request
    // --------------------
    // Use this method to cut short the processing stack and return back to
    // client with the error status.
    //
    void ame_finalize_request(int status);
};

// ---------------------------------------------------------------------------
// Connector Adapter to implement GetObject method.
//
class Conn_GetObject : public AME_Request
{
  public:
    Conn_GetObject(AMEngine *eng, AME_HttpReq *req);
    virtual ~Conn_GetObject();

    // returns bucket id
    virtual std::string get_bucket_id() = 0;

    // returns the object id
    virtual std::string get_object_id() = 0;

    /// returns the max buffer length to write
    virtual fds_uint32_t get_max_buf_len() const;

    // Connector methods to handle/resume GetObject request.
    //
    virtual void ame_request_handler();
    virtual int  ame_request_resume();

  protected:
    void *cur_get_buffer;

    /// Maximum buf size for stream chunking
    fds_uint32_t get_obj_max_buf_len;
};

// ---------------------------------------------------------------------------
// Connector Adapter to implement PutObject method.
//
class Conn_PutObject : public AME_Request
{
  public:
    Conn_PutObject(AMEngine *eng, AME_HttpReq *req);
    virtual ~Conn_PutObject();

    // returns bucket id
    virtual std::string get_bucket_id() = 0;

    // returns the object id
    virtual std::string get_object_id() = 0;

    /// returns the max buffer length to write
    virtual fds_uint32_t get_max_buf_len() const;

    // Connector method to handle/resume PutObject request.
    //
    virtual void ame_request_handler();
    virtual int  ame_request_resume();

  protected:
    /// Maximum buf size for stream chunking
    fds_uint32_t put_obj_max_buf_len;
};

// ---------------------------------------------------------------------------
// Connector Adapter to implement DelObject method.
//
class Conn_DelObject : public AME_Request
{
  public:
    Conn_DelObject(AMEngine *eng, AME_HttpReq *req);
    virtual ~Conn_DelObject();

    // returns bucket id
    virtual std::string get_bucket_id() = 0;

    // returns the object id
    virtual std::string get_object_id() = 0;

    // Connector method to handle/resume DelObject request.
    //
    virtual void ame_request_handler();
    virtual int  ame_request_resume();

  protected:
};

// Connector Adapter to implement PutObject method.
//
class Conn_PutBucket : public AME_Request
{
  public:
    Conn_PutBucket(AMEngine *eng, AME_HttpReq *req);
    virtual ~Conn_PutBucket();

    // returns bucket id
    virtual std::string get_bucket_id() = 0;

    // Connector method to handle/resume PutBucket request.
    //
    virtual void ame_request_handler();
    virtual int  ame_request_resume();

  protected:
};

// ---------------------------------------------------------------------------
// Connector Adapter to implement GetBucket method.
//
class Conn_GetBucket : public AME_Request
{
  public:
    Conn_GetBucket(AMEngine *eng, AME_HttpReq *req);
    virtual ~Conn_GetBucket();

    // returns bucket id
    virtual std::string get_bucket_id() = 0;

    virtual void ame_request_handler();
    virtual int  ame_request_resume();

    // Format response data with result from listing objects in the bucket.
    //
    void ame_fmt_resp_data(int is_trucated, const char *next_marker,
            int content_cnt, const ListBucketContents *contents,
            int comm_prefix_cnt, const char **comm_prefix);

  protected:
    void *cur_get_buffer;
};

// Connector Adapter to implement GetBucket method.
//
class Conn_DelBucket : public AME_Request
{
  public:
    Conn_DelBucket(AMEngine *eng, AME_HttpReq *req);
    virtual ~Conn_DelBucket();

    // returns bucket id
    virtual std::string get_bucket_id() = 0;

    virtual void ame_request_handler();
    virtual int  ame_request_resume();

  protected:
};

// Connector Adapter to implement PutBucketParams method.
//
class Conn_PutBucketParams : public AME_Request
{
  public:
    Conn_PutBucketParams(AMEngine *eng, AME_HttpReq *req);
    virtual ~Conn_PutBucketParams();

    // returns bucket id
    virtual std::string get_bucket_id() {
      return ame_http.getURIParts()[0];
    }

    // returns relative priority
    virtual fds_uint32_t get_relative_prio() {
      return relative_prio;
    }

    // returns min iops
    virtual double get_iops_min() {
      return iops_min;
    }

    // returns max iops
    virtual double get_iops_max() {
      return iops_max;
    }

    // Connector method to handle PutBucket request.
    //
    virtual void ame_request_handler();
    virtual int  ame_request_resume();

    // Format response header in FDS protocol.
    //
    virtual int ame_format_response_hdr();
  protected:
    fds_uint32_t relative_prio;
    double       iops_min;
    double       iops_max;
    fds_bool_t   validParams;
};

// ---------------------------------------------------------------------------
// Connector Adapter to implement GetBucketStats method.
//
class Conn_GetBucketStats : public AME_Request
{
  public:
    Conn_GetBucketStats(AMEngine *eng, AME_HttpReq *req);
    virtual ~Conn_GetBucketStats();

    /* returns bucket id
     * currently we get stats for all existing buckets,
     * so so this will return empty string for now, but
     * we may extend this to get stats for a particular bucket as well */
    virtual std::string get_bucket_id() {
      return std::string("");
    }

    virtual void ame_request_handler();
    virtual int  ame_request_resume();

    // Format response header in FDS protocol.
    //
    virtual int ame_format_response_hdr();
    void ame_fmt_resp_data(const std::string &timestamp,
            int content_count, const BucketStatsContent *contents);

  protected:
    void *cur_get_buffer;
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_NGINX_DRIVER_AM_ENGINE_H_
