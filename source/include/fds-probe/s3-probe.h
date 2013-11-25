/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef INCLUDE_S3_PROBE_H_
#define INCLUDE_S3_PROBE_H_

#include <fds-probe/fds_probe.h>
#include <am-engine/s3connector.h>

namespace fds {

// Probe GET connector to hookup S3 protocol to the probe.
//
class Probe_GetObject : public S3_GetObject
{
  public:
    Probe_GetObject(AMEngine *eng, HttpRequest &req);
    ~Probe_GetObject();

    virtual void ame_request_handler();
  protected:
};

// Probe PUT connector to hooup S3 protocol to the probe.
//
class Probe_PutObject : public S3_PutObject
{
  public:
    Probe_PutObject(AMEngine *eng, HttpRequest &req);
    ~Probe_PutObject();

    virtual void ame_request_handler();
  protected:
};

// S3 Probe Hookup with FDS Adapters
//
class ProbeS3Eng : public AMEngine_S3
{
  public:
    ProbeS3Eng(char const *const name) : AMEngine_S3(name) {}
    ~ProbeS3Eng() {}

    // Hookup this S3 engine probe to the back-end adapter.
    //
    void probe_connect_adapter(ProbeMod *adapter) {
        pr_adapter = adapter;
    }
    ProbeMod *probe_get_adapter() {
        return pr_adapter;
    }

    // Object factory
    //
    virtual Conn_GetObject *ame_getobj_hdler(HttpRequest &req) {
        return new Probe_GetObject(this, req);
    }
    virtual Conn_PutObject *ame_putobj_hdler(HttpRequest &req) {
        return new Probe_PutObject(this, req);
    }
    virtual Conn_DelObject *ame_delobj_hdler(HttpRequest &req) {
        return nullptr;
    }

    // Bucket factory
    //
    virtual Conn_GetBucket *ame_getbucket_hdler(HttpRequest &req) {
        return nullptr;
    }
    virtual Conn_PutBucket *ame_putbucket_hdler(HttpRequest &req) {
        return nullptr;
    }
    virtual Conn_DelBucket *ame_delbucker_hdler(HttpRequest &req) {
        return nullptr;
    }
  private:
    ProbeMod                 *pr_adapter;
};

extern ProbeS3Eng gl_probeS3Eng;

} // namespace fds

#endif /* INCLUDE_S3_PROBE_H_ */
