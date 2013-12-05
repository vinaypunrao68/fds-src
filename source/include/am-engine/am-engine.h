/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef INCLUDE_AM_ENGINE_H_
#define INCLUDE_AM_ENGINE_H_

#include <fds_module.h>
#include <fds_request.h>
#include <string>
#include <native_api.h>
#include <util/Log.h>
#include <am-engine/http_utils.h>

extern "C" {
struct ngx_chain_s;
struct ngx_http_request_s;
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

class AMEngine : public Module
{
  public:
    AMEngine(char const *const name) :
        Module(name), eng_signal(), eng_etc("etc"),
        eng_logs("logs"), eng_conf("etc/fds.conf"),
        queue(1, 1000) {
      ame_log = new fds_log("ame", "logs");
    }

    ~AMEngine() {}

    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();
    void run_server(FDS_NativeAPI *api);
    fdsio::RequestQueue* get_queue() {
      return &queue;
    }

    // Factory methods to create objects handling required protocol.
    virtual Conn_GetObject *ame_getobj_hdler(AME_HttpReq *req) = 0;
    virtual Conn_PutObject *ame_putobj_hdler(AME_HttpReq *req) = 0;
    virtual Conn_DelObject *ame_delobj_hdler(AME_HttpReq *req) = 0;
    virtual Conn_GetBucket *ame_getbucket_hdler(AME_HttpReq *req) = 0;
    virtual Conn_PutBucket *ame_putbucket_hdler(AME_HttpReq *req) = 0;
    virtual Conn_DelBucket *ame_delbucket_hdler(AME_HttpReq *req) = 0;

    FDS_NativeAPI *ame_fds_hook() {
        return eng_api;
    }

    fds_log* get_log() {
      return ame_log;
    }

  private:
    std::string              eng_signal;
    char const *const        eng_etc;
    char const *const        eng_logs;
    char const *const        eng_conf;
    FDS_NativeAPI            *eng_api;
    fdsio::RequestQueue queue;
    fds_log *ame_log;
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
    RESP_CONNECTION          = 1,
    RESP_CONNECTION_OPEN     = 2,
    RESP_CONNECTION_CLOSE    = 3,
    RESP_ETAG                = 4,
    RESP_DATE                = 5,
    RESP_SERVER              = 6,

    // RESTBucket response keys
    REST_LIST_BUCKET         = 7,
    REST_NAME                = 8,
    REST_PREFIX              = 9,
    REST_MARKER              = 10,
    REST_MAX_KEYS            = 11,
    REST_IS_TRUNCATED        = 12,
    REST_CONTENTS            = 13,
    REST_KEY                 = 14,
    REST_ETAG                = 15,
    REST_SIZE                = 16,
    REST_STORAGE_CLASS       = 17,
    REST_OWNER               = 18,
    REST_ID                  = 19,
    REST_DISPLAY_NAME        = 20,
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
// Generic connector to handle request/response protocol with buffer chunks
// semantic
//
class AME_Request : public fdsio::Request
{
  public:
    static int map_fdsn_status(FDSN_Status status);

  public:
    AME_Request(AMEngine *eng, AME_HttpReq *req);
    ~AME_Request();

    // ame_add_context
    // ---------------
    // Add context to track the state of this request and synchronize with the
    // event loop in the plugin module.
    //
    void ame_add_context(AME_Ctx *ctx) { ame_ctx = ctx; }

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
    // Key/value strings can be on the stack if the caller calls
    // ame_send_response_hdr() in the same context.
    //
    int ame_set_resp_keyval(char *k, int klen, char *v, int vlen);
    virtual int  ame_format_response_hdr() = 0;
    virtual void ame_request_handler() = 0;

    fds_log* get_log() {
      return ame->get_log();
    }

  protected:
    friend class AME_Ctx;

    AMEngine                 *ame;
    AME_Ctx                  *ame_ctx;
    AME_HttpReq              *ame_req;
    struct ngx_chain_s       *post_buf_itr;
    HttpRequest              ame_http;
    int                      ame_resp_status;
    std::string              etag;

    // Tell the engine if we need to finalize this request.
    fds_bool_t               ame_finalize;
    /*
     * todo: once we switch to async model with handling requests, these members
     * shouldn't be neeeded
     */
    int resp_status;
    const char *resp_buf;
    int resp_buf_len;

    void notify_request_completed(int status, const char *buf, int len) {
      resp_status = status;
      resp_buf = buf;
      resp_buf_len = len;
      req_complete();
    }

    // Async callback to transfer the control to the ame_ctx context maintained
    // by this request object.  The caller must setup the correct handler and
    // internal state in ame_ctx object so that when this method is called,
    // it'll get back the control at the correct context.
    //
    void ame_request_async_callback(int status);

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
    void ame_reqt_iter_reset();
    int ame_reqt_iter_next();
    char* ame_reqt_iter_data(int *len);

    // Common response path.
    // The request handler then prepares response data.
    // Format key/value in response header:
    //    ame_set_resp_keyval("Content-Type", "application/xml");
    //    ...
    // Get buffer to send response data:
    //    cookie = ame_push_resp_data_buf(&len, &buf_adr);
    //    fill data to buf_adr, len.
    //    ...
    // Send partial/full response data:
    //    ame_send_response_hdr();
    //    ame_send_resp_data(cookie, valid_len, last_buf);
    //    If the last buf is true, this object will be freed by the AM Engine.
    //
    void *ame_push_resp_data_buf(int ask, char **buf, int *got_len);
    int ame_send_resp_data(void *cookie, int len, fds_bool_t last);

    // ame_send_response_hdr
    // ---------------------
    // Common code path to send the response header to the client.
    int ame_send_response_hdr();
};

// ---------------------------------------------------------------------------
// Connector Adapter to implement GetObject method.
//
class Conn_GetObject : public AME_Request
{
public:
  static FDSN_Status cb(void *req, fds_uint64_t bufsize,
      const char *buf, void *cb,
      FDSN_Status status, ErrorDetails *errdetails);

  public:
    Conn_GetObject(AMEngine *eng, AME_HttpReq *req);
    ~Conn_GetObject();

    // returns bucket id
    virtual std::string get_bucket_id() = 0;

    // returns the object id
    virtual std::string get_object_id() = 0;

    // Connector method to handle GetObject request.
    //
    virtual void ame_request_handler();

    // ame_send_get_response
    // ---------------------
    // Primitive API to allow FDSN to send simple response to the client.
    //
    virtual void
    fdsn_send_get_response(int status, int get_len);

  protected:
    void *cur_get_buffer;
};

// ---------------------------------------------------------------------------
// Connector Adapter to implement PutObject method.
//
class Conn_PutObject : public AME_Request
{
public:
  static int cb(void *reqContext, fds_uint64_t bufferSize, char *buffer,
      void *callbackData, FDSN_Status status, ErrorDetails* errDetails);

  public:
    Conn_PutObject(AMEngine *eng, AME_HttpReq *req);
    ~Conn_PutObject();

    // returns bucket id
    virtual std::string get_bucket_id() = 0;

    // returns the object id
    virtual std::string get_object_id() = 0;

    // Connector method to handle PutObject request.
    //
    virtual void ame_request_handler();

    // Common code to send response back to the client.  Connector specific
    // will provide more detail on the response.
    //
    virtual void fdsn_send_put_response(int status, int put_len);

  protected:
};

// ---------------------------------------------------------------------------
// Connector Adapter to implement DelObject method.
//
class Conn_DelObject : public AME_Request
{
public:
  static void cb(FDSN_Status status,
      const ErrorDetails *errorDetails,
      void *callbackData);

  public:
    Conn_DelObject(AMEngine *eng, AME_HttpReq *req);
    ~Conn_DelObject();

    // returns bucket id
    virtual std::string get_bucket_id() = 0;

    // returns the object id
    virtual std::string get_object_id() = 0;

    // Connector method to handle DelObject request.
    //
    virtual void ame_request_handler();

    // Common code to send response back to the client.  Connector specific
    // will provide more detail on the response.
    //
    virtual void fdsn_send_del_response(int status, int len);

  protected:
};

// Connector Adapter to implement PutObject method.
//
class Conn_PutBucket : public AME_Request
{
  public:
    // put bucket callback from FDS Api
    static void
    fdsn_cb(FDSN_Status status,
            const ErrorDetails *errorDetails, void *callbackData);

  public:
    Conn_PutBucket(AMEngine *eng, AME_HttpReq *req);
    ~Conn_PutBucket();

    // returns bucket id
    virtual std::string get_bucket_id() = 0;

    // Connector method to handle PutBucket request.
    //
    virtual void ame_request_handler();

    // Common code to send response back to the client.  Connector specific
    // will provide more detail on the response.
    //
    virtual void fdsn_send_put_response(int status, int put_len);
  protected:
};

// ---------------------------------------------------------------------------
// Connector Adapter to implement GetBucket method.
//
class Conn_GetBucket : public AME_Request
{
  public:
    // Get Bucket callback from FDS API.
    static void
    fdsn_getbucket(int isTruncated, const char *nextMaker,
                   int contentsCount, const ListBucketContents *contents,
                   int commPrefixCount, const char **commPrefixes,
                   void *cbarg, FDSN_Status status);

  public:
    Conn_GetBucket(AMEngine *eng, AME_HttpReq *req);
    ~Conn_GetBucket();

    // returns bucket id
    virtual std::string get_bucket_id() = 0;

    virtual void ame_request_handler();
    virtual void fdsn_send_getbucket_response(int status, int len);
  protected:
    void *cur_get_buffer;
};

// Connector Adapter to implement GetBucket method.
//
class Conn_DelBucket : public AME_Request
{
public:
  // delete bucket callback from FDS Api
  static void cb(FDSN_Status status,
                 const ErrorDetails *errorDetails, void *callbackData);

  public:
    Conn_DelBucket(AMEngine *eng, AME_HttpReq *req);
    ~Conn_DelBucket();

    // returns bucket id
    virtual std::string get_bucket_id() = 0;

    virtual void ame_request_handler();
    virtual void fdsn_send_delbucket_response(int status, int len);
  protected:
};

} // namespace fds

#endif /* INCLUDE_AM_ENGINE_H_ */
