/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NGINX_DRIVER_S3CONNECTOR_H_
#define SOURCE_INCLUDE_NGINX_DRIVER_S3CONNECTOR_H_

#include <string>
#include <nginx-driver/am-engine.h>

namespace fds {

// S3 Get connector which does S3 specific object get semantic.
//
class S3_GetObject : public Conn_GetObject
{
  public:
    S3_GetObject(AMEngine *eng, AME_HttpReq *req);
    ~S3_GetObject();

    // returns bucket id
    virtual std::string get_bucket_id();

    // returns the object id
    virtual std::string get_object_id();

    // Format response header in S3 protocol.
    //
    virtual int ame_format_response_hdr();

  protected:
    // List of known key-values that we'll need to send the GET response.
    //
    char *resp_xx_key;
    char *resp_xx_value;
};

// S3 Put connector which does S3 specific object put semantic.
//
class S3_PutObject : public Conn_PutObject
{
  public:
    S3_PutObject(AMEngine *eng, AME_HttpReq *req);
    ~S3_PutObject();

    // returns bucket id
    virtual std::string get_bucket_id();

    // returns the object id
    virtual std::string get_object_id();

    // Format response header in S3 protocol.
    //
    virtual int ame_format_response_hdr();

  protected:
    // List of known key-values that we'll need to send the GET response.
    //
    char *resp_xx_key;
    char *resp_xx_value;
};

// S3 Delete connector which does S3 specific object delete semantic.
//
class S3_DelObject : public Conn_DelObject
{
  public:
    S3_DelObject(AMEngine *eng, AME_HttpReq *req);
    ~S3_DelObject();

    // returns bucket id
    virtual std::string get_bucket_id();

    // returns the object id
    virtual std::string get_object_id();

    // Format response header in S3 protocol.
    //
    virtual int ame_format_response_hdr();

  protected:
    // List of known key-values that we'll need to send the GET response.
    //
    char *resp_xx_key;
    char *resp_xx_value;
};

// S3 Get Bucket connector
//
class S3_GetBucket : public Conn_GetBucket
{
  public:
    S3_GetBucket(AMEngine *eng, AME_HttpReq *req);
    ~S3_GetBucket();

    virtual int ame_format_response_hdr();

    // returns bucket id
    virtual std::string get_bucket_id();

  protected:
};

// S3 Put Bucket connector
//
class S3_PutBucket : public Conn_PutBucket
{
  public:
    S3_PutBucket(AMEngine *eng, AME_HttpReq *req);
    ~S3_PutBucket();

    // returns bucket id
    virtual std::string get_bucket_id();

    // Format response header in S3 protocol.
    //
    virtual int ame_format_response_hdr();

  protected:
    // List of known key-values that we'll need to send the GET response.
    //
    char *resp_xx_key;
    char *resp_xx_value;
};

// S3 Delete Bucket connector
//
class S3_DelBucket : public Conn_DelBucket
{
  public:
    S3_DelBucket(AMEngine *eng, AME_HttpReq *req);
    ~S3_DelBucket();

    // returns bucket id
    virtual std::string get_bucket_id();

    // Format response header in S3 protocol.
    //
    virtual int ame_format_response_hdr();

  protected:
    // List of known key-values that we'll need to send the GET response.
    //
    char *resp_xx_key;
    char *resp_xx_value;
};

class AMEngine_S3 : public AMEngine
{
  public:
    explicit AMEngine_S3(char const *const name) : AMEngine(name) {}
    ~AMEngine_S3() {}

    // Object factory.
    //
    virtual Conn_GetObject *ame_getobj_hdler(AME_HttpReq *req) {
        return new S3_GetObject(this, req);
    }
    virtual Conn_PutObject *ame_putobj_hdler(AME_HttpReq *req) {
        return new S3_PutObject(this, req);
    }
    virtual Conn_DelObject *ame_delobj_hdler(AME_HttpReq *req) {
        return new S3_DelObject(this, req);
    }

    // Bucket factory.
    //
    virtual Conn_GetBucket *ame_getbucket_hdler(AME_HttpReq *req) {
        return new S3_GetBucket(this, req);
    }
    virtual Conn_PutBucket *ame_putbucket_hdler(AME_HttpReq *req) {
        return new S3_PutBucket(this, req);
    }
    virtual Conn_DelBucket *ame_delbucket_hdler(AME_HttpReq *req) {
        return new S3_DelBucket(this, req);
    }
};

extern AMEngine_S3 gl_AMEngineS3;

}  // namespace fds
#endif  // SOURCE_INCLUDE_NGINX_DRIVER_S3CONNECTOR_H_
