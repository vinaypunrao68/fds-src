/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef INCLUDE_S3_CONNECTOR_H_
#define INCLUDE_S3_CONNECTOR_H_

#include <am-engine/am-engine.h>

namespace fds {

// S3 Get connector which does S3 specific object get semantic.
//
class S3_GetObject : public Conn_GetObject
{
  public:
    S3_GetObject(HttpRequest &req);
    ~S3_GetObject();

    // returns bucket id
    virtual std::string get_bucket_id();

    // returns the object id
    virtual std::string get_object_id();

    // Format response header in S3 protocol.
    //
    virtual ame_ret_e ame_format_response_hdr();

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
    S3_PutObject(HttpRequest &req);
    ~S3_PutObject();

    // returns bucket id
    virtual std::string get_bucket_id();

    // returns the object id
    virtual std::string get_object_id();

    // Format response header in S3 protocol.
    //
    virtual ame_ret_e ame_format_response_hdr();

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
    S3_GetBucket(HttpRequest &req);
    ~S3_GetBucket();

    virtual ame_ret_e ame_format_response_hdr();

  protected:
};

// S3 Put Bucket connector
//
class S3_PutBucket : public Conn_PutBucket
{
  public:
    S3_PutBucket(HttpRequest &req);
    ~S3_PutBucket();

    virtual ame_ret_e ame_format_response_hdr();

  protected:
};

} // namespace fds

#endif /* INCLUDE_AM_ENGINE_H_ */
