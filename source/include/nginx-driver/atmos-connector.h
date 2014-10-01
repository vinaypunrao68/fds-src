/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NGINX_DRIVER_ATMOS_CONNECTOR_H_
#define SOURCE_INCLUDE_NGINX_DRIVER_ATMOS_CONNECTOR_H_

#include <string>
#include <nginx-driver/am-engine.h>

namespace fds {

// Atmos Get connector which does Atmos specific object get semantic.
//
class Atmos_GetObject : public Conn_GetObject
{
  public:
    Atmos_GetObject(AMEngine *eng, AME_HttpReq *req);
    ~Atmos_GetObject();

    // returns bucket id
    virtual std::string get_bucket_id();

    // returns the object id
    virtual std::string get_object_id();

    // Format response header in Atmos protocol.
    //
    virtual int ame_format_response_hdr();

  protected:
    // List of known key-values that we'll need to send the GET response.
    //
    char *resp_xx_key;
    char *resp_xx_value;
};

// Atmos Put connector which does Atmos specific object put semantic.
//
class Atmos_PutObject : public Conn_PutObject
{
  public:
    Atmos_PutObject(AMEngine *eng, AME_HttpReq *req);
    ~Atmos_PutObject();

    // returns bucket id
    virtual std::string get_bucket_id();

    // returns the object id
    virtual std::string get_object_id();

    // Format response header in Atmos protocol.
    //
    virtual int ame_format_response_hdr();

    virtual void ame_request_handler();

  protected:
    // List of known key-values that we'll need to send the GET response.
    //
    char *resp_xx_key;
    char *resp_xx_value;
};

// Atmos Delete connector which does Atmos specific object delete semantic.
//
class Atmos_DelObject : public Conn_DelObject
{
  public:
    Atmos_DelObject(AMEngine *eng, AME_HttpReq *req);
    ~Atmos_DelObject();

    // returns bucket id
    virtual std::string get_bucket_id();

    // returns the object id
    virtual std::string get_object_id();

    // Format response header in Atmos protocol.
    //
    virtual int ame_format_response_hdr();

  protected:
    // List of known key-values that we'll need to send the GET response.
    //
    char *resp_xx_key;
    char *resp_xx_value;
};

// Atmos Get Bucket connector
//
class Atmos_GetBucket : public Conn_GetBucket
{
  public:
    Atmos_GetBucket(AMEngine *eng, AME_HttpReq *req);
    ~Atmos_GetBucket();

    virtual int ame_format_response_hdr();

    // returns bucket id
    virtual std::string get_bucket_id();

  protected:
};

// Atmos Put Bucket connector
//
class Atmos_PutBucket : public Conn_PutBucket
{
  public:
    Atmos_PutBucket(AMEngine *eng, AME_HttpReq *req);
    ~Atmos_PutBucket();

    // returns bucket id
    virtual std::string get_bucket_id();

    // Format response header in Atmos protocol.
    //
    virtual int ame_format_response_hdr();

  protected:
    // List of known key-values that we'll need to send the GET response.
    //
    char *resp_xx_key;
    char *resp_xx_value;
};

// Atmos Delete Bucket connector
//
class Atmos_DelBucket : public Conn_DelBucket
{
  public:
    Atmos_DelBucket(AMEngine *eng, AME_HttpReq *req);
    ~Atmos_DelBucket();

    // returns bucket id
    virtual std::string get_bucket_id();

    // Format response header in Atmos protocol.
    //
    virtual int ame_format_response_hdr();

  protected:
    // List of known key-values that we'll need to send the GET response.
    //
    char *resp_xx_key;
    char *resp_xx_value;
};

class AMEngine_Atmos : public AMEngine
{
  public:
    explicit AMEngine_Atmos(char const *const name) : AMEngine(name) {}
    ~AMEngine_Atmos() {}

    // Object factory.
    //
    virtual Conn_GetObject *ame_getobj_hdler(AME_HttpReq *req) {
        return new Atmos_GetObject(this, req);
    }
    virtual Conn_PutObject *ame_putobj_hdler(AME_HttpReq *req) {
        return new Atmos_PutObject(this, req);
    }
    virtual Conn_DelObject *ame_delobj_hdler(AME_HttpReq *req) {
        return new Atmos_DelObject(this, req);
    }

    // Bucket factory.
    //
    virtual Conn_GetBucket *ame_getbucket_hdler(AME_HttpReq *req) {
        return new Atmos_GetBucket(this, req);
    }
    virtual Conn_PutBucket *ame_putbucket_hdler(AME_HttpReq *req) {
        return new Atmos_PutBucket(this, req);
    }
    virtual Conn_DelBucket *ame_delbucket_hdler(AME_HttpReq *req) {
        return new Atmos_DelBucket(this, req);
    }
};

extern AMEngine_Atmos gl_AMEngineAtmos;

}  // namespace fds
#endif  // SOURCE_INCLUDE_NGINX_DRIVER_ATMOS_CONNECTOR_H_
