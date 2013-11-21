/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef INCLUDE_AM_ENGINE_H_
#define INCLUDE_AM_ENGINE_H_

#include <fds_module.h>
#include <fds_request.h>
#include <string>
#include <am-engine/http_utils.h>

namespace fds {
class FDS_NativeAPI;
static FDS_NativeAPI *ame_fds_hook(void);

class AMEngine : public Module
{
  public:
    AMEngine(char const *const name) :
        Module(name), eng_signal(), eng_etc("etc"),
        eng_logs("logs"), eng_conf("etc/fds.conf") {}

    ~AMEngine() {}

    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();
    void run_server(FDS_NativeAPI *api);

  private:
    friend FDS_NativeAPI *ame_fds_hook(void);

    std::string              eng_signal;
    char const *const        eng_etc;
    char const *const        eng_logs;
    char const *const        eng_conf;
    FDS_NativeAPI            *eng_api;
};

extern AMEngine              gl_AMEngine;

// ---------------------------------------------------------------------------
// ame_fds_hook
// ------------
// Return the API obj to hook up with FDS API.
//
static inline FDS_NativeAPI *ame_fds_hook(void)
{
    return gl_AMEngine.eng_api;
}

// Common return code used by this API.
typedef enum
{
    AME_OK           = 0,
    AME_TRY_AGAIN    = 1,
    AME_HAVE_DATA    = 2,
    AME_NO_MORE_DATA = 3,
    AME_MAX
} ame_ret_e;

// Generic connector to handle request/response protocol with buffer chunks
// semantic
//
class AME_Request : public fdsio::Request
{
  public:
    AME_Request(HttpRequest &req);
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
    //
    ame_ret_e ame_set_resp_keyval(char const *const k, char const *const v);
    virtual ame_ret_e ame_format_response_hdr() = 0;

  protected:
    HttpRequest ame_req;
    ngx_chain_t *post_buf_itr;


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
    char const *const ame_reqt_iter_data(int *len);
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

// Connector Adapter to implement GetObject method.
//
class Conn_GetObject : public AME_Request
{
  public:
    Conn_GetObject(HttpRequest &req);
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
};

// Connector Adapter to implement PutObject method.
//
class Conn_PutObject : public AME_Request
{
  public:
    Conn_PutObject(HttpRequest &req);
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

} // namespace fds

#endif /* INCLUDE_AM_ENGINE_H_ */
