/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef INCLUDE_AM_ENGINE_H_
#define INCLUDE_AM_ENGINE_H_

#include <fds_module.h>
#include <fds_request.h>
#include <string>

struct ngx_http_request_s;
namespace fds {

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
    void run_server();

  private:
    std::string              eng_signal;
    char const *const        eng_etc;
    char const *const        eng_logs;
    char const *const        eng_conf;
};

extern AMEngine              gl_AMEngine;

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
    AME_Request(struct ngx_http_request_s *req);
    ~AME_Request();

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
    char const *const ame_get_reqt_hdr_val(char const *const key);
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
    ame_ret_e ame_set_resp_keyval(char const *const k, char const *const v);
    void *ame_push_resp_data_buf(int *buf_len, char **buf_adr);
    void  ame_send_resp_data(void *buf_cookie, int val_len, int last_buf);
    virtual ame_ret_e ame_send_response_hdr() = 0;

  protected:
    struct ngx_http_request_s  *ame_req;
};

// FDSN Adapter to implement GetObject Method.
//
class FDSN_GetObject : public AME_Request
{
  public:
    FDSN_GetObject(struct ngx_http_request_s *req);
    ~FDSN_GetObject();

    // Method to interface with FDSN to handle GetObject request.
    //
    virtual void ame_request_handler();

  protected:
};

} // namespace fds

#endif /* INCLUDE_AM_ENGINE_H_ */
