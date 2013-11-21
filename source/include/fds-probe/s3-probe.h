/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef INCLUDE_S3_PROBE_H_
#define INCLUDE_S3_PROBE_H_

#include <am-engine/s3connector.h>

namespace fds {

// Probe GET connector to hookup S3 protocol to the probe.
//
class Probe_GetObject : public S3_GetObject
{
  public:
    Probe_GetObject(HttpRequest &req);
    ~Probe_GetObject();

    virtual void ame_request_handler();
  protected:
};

// Probe PUT connector to hooup S3 protocol to the probe.
//
class Probe_PutObject : public S3_PutObject
{
  public:
    Probe_PutObject(HttpRequest &req);
    ~Probe_PutObject();

    virtual void ame_request_handler();
  protected:
};

} // namespace fds

#endif /* INCLUDE_S3_PROBE_H_ */
