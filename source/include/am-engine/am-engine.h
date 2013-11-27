/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef INCLUDE_AM_ENGINE_H_
#define INCLUDE_AM_ENGINE_H_

#include <fds_module.h>
#include <fds_request.h>
#include <string>
#include <am-engine/http_utils.h>
#include <native_api.h>
#include <util/Log.h>

namespace fds {
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
    virtual Conn_GetObject *ame_getobj_hdler(HttpRequest &req) = 0;
    virtual Conn_PutObject *ame_putobj_hdler(HttpRequest &req) = 0;
    virtual Conn_DelObject *ame_delobj_hdler(HttpRequest &req) = 0;
    virtual Conn_GetBucket *ame_getbucket_hdler(HttpRequest &req) = 0;
    virtual Conn_PutBucket *ame_putbucket_hdler(HttpRequest &req) = 0;
    virtual Conn_DelBucket *ame_delbucket_hdler(HttpRequest &req) = 0;

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

// Common return code used by this API.
typedef enum
{
    AME_OK           = 0,
    AME_TRY_AGAIN    = 1,
    AME_HAVE_DATA    = 2,
    AME_NO_MORE_DATA = 3,
    AME_MAX
} ame_ret_e;

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
    ngx_int_t                kv_keylen;
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
    AME_Request(AMEngine *eng, HttpRequest &req);
    ~AME_Request();

    // ame_get_reqt_hdr_val
    // --------------------
    // Return the value corresponding with the key in the request header.
    //
    char const *const ame_get_reqt_hdr_val(char const *const key);

    // ame_set_std_resp
    // ----------------
    // Common path to set the standard response status/len.
    //
    ame_ret_e ame_set_std_resp(int status, int len);

    // ame_set_resp_keyval
    // -------------------
    // Set key/value in the response to send to the client.
    // Assume key/value buffers remain valid until this obj is freed.
    //
    ame_ret_e ame_set_resp_keyval(char *k, ngx_int_t klen,
                                  char *v, ngx_int_t vlen);

    virtual ame_ret_e ame_format_response_hdr() = 0;

    fds_log* get_log() {
      return ame->get_log();
    }

  protected:
    HttpRequest              ame_req;
    AMEngine                 *ame;
    ngx_chain_t              *post_buf_itr;
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
    bool req_completed;

    void notify_request_completed(int status, const char *buf, int len) {
      resp_status = status;
      resp_buf = buf;
      resp_buf_len = len;
      req_completed = true;
      req_complete();
    }

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
    ame_ret_e ame_reqt_iter_next();
    char* ame_reqt_iter_data(int *len);
    virtual void ame_request_handler() = 0;

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
    ame_ret_e ame_send_resp_data(void *cookie, int len, fds_bool_t last);

    // ame_send_response_hdr
    // ---------------------
    // Common code path to send the response header to the client.
    ame_ret_e ame_send_response_hdr();
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
    Conn_GetObject(AMEngine *eng, HttpRequest &req);
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

    // fdsn_alloc_get_buffer
    // ---------------------
    // Allocate buffer to store data for the GET request.
    // @param ask_len (i): the length in byte that FDSN asked for.
    // @param buf_adr (o): address of the buffer.
    // @param got_len (o): actual length that the transport layer can alloc.
    // @return cookie to pass to fdsn_send_get_buffer().
    //
    virtual void *
    fdsn_alloc_get_buffer(int ask_len, char **buf_adr, int *got_len)
    {
        return ame_push_resp_data_buf(ask_len, buf_adr, got_len);
    }

    // fdsn_send_get_buffer
    // --------------------
    // Send the get buffer to the client.  If this is the last call, FDSN
    // can free its resources.
    // @param cookie (i): cookie obtained from the alloc() API.
    // @param len (i): buffer length containing valid data.
    // @param last (i): true if this is the last buffer.  Since the connector
    //    layer also has the expected length in fdsn_send_get_response(),
    //    it'll performn cross check to make sure everything adds up.
    // @return status of the send op.
    //
    virtual ame_ret_e
    fdsn_send_get_buffer(void *cookie, int len, fds_bool_t last)
    {
        return ame_send_resp_data(cookie, len, last);
    }

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
    Conn_PutObject(AMEngine *eng, HttpRequest &req);
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
    Conn_DelObject(AMEngine *eng, HttpRequest &req);
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
    Conn_PutBucket(AMEngine *eng, HttpRequest &req);
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
    Conn_GetBucket(AMEngine *eng, HttpRequest &req);
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
    Conn_DelBucket(AMEngine *eng, HttpRequest &req);
    ~Conn_DelBucket();

    // returns bucket id
    virtual std::string get_bucket_id() = 0;

    virtual void ame_request_handler();
    virtual void fdsn_send_delbucket_response(int status, int len);
  protected:
};

} // namespace fds

#endif /* INCLUDE_AM_ENGINE_H_ */
