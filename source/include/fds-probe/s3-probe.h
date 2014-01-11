/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_PROBE_S3_PROBE_H_
#define SOURCE_INCLUDE_FDS_PROBE_S3_PROBE_H_

#include <list>
#include <fds-probe/fds_probe.h>
#include <am-engine/s3connector.h>
#include <concurrency/ThreadPool.h>

namespace fds {

// Probe GET connector to hookup S3 protocol to the probe.
//
class Probe_GetObject : public S3_GetObject
{
  public:
    Probe_GetObject(AMEngine *eng, AME_HttpReq *req);
    ~Probe_GetObject();

    virtual void ame_request_handler();
    virtual int  ame_request_resume();
  protected:
    ProbeIORequest           *preq;
};

// Probe PUT connector to hooup S3 protocol to the probe.
//
class Probe_PutObject : public S3_PutObject
{
  public:
    Probe_PutObject(AMEngine *eng, AME_HttpReq *req);
    ~Probe_PutObject();

    virtual void ame_request_handler();
    virtual int  ame_request_resume();
  protected:
    ProbeIORequest           *preq;
};

// S3 Probe Hookup with FDS Adapters
//
class ProbeS3Eng : public AMEngine_S3
{
  public:
    explicit ProbeS3Eng(char const *const name);
    ~ProbeS3Eng();

    // Hookup this S3 engine probe to the back-end adapter.
    //
    void probe_add_adapter(ProbeMod *adapter);
    ProbeMod *probe_get_adapter();

    inline fds_threadpool *probe_get_thrpool() {
        return pr_thrpool;
    }

    // Object factory
    //
    virtual Conn_GetObject *ame_getobj_hdler(AME_HttpReq *req) {
        return new Probe_GetObject(this, req);
    }
    virtual Conn_PutObject *ame_putobj_hdler(AME_HttpReq *req) {
        return new Probe_PutObject(this, req);
    }
    virtual Conn_DelObject *ame_delobj_hdler(AME_HttpReq *req) {
        return nullptr;
    }

    // Bucket factory.
    //
    virtual Conn_GetBucket *ame_getbucket_hdler(AME_HttpReq *req) {
        return nullptr;
    }
    virtual Conn_PutBucket *ame_putbucket_hdler(AME_HttpReq *req) {
        return nullptr;
    }
    virtual Conn_DelBucket *ame_delbucker_hdler(AME_HttpReq *req) {
        return nullptr;
    }
  private:
    std::list<ProbeMod *>     pr_adapter;
    fds_mutex                 pr_mtx;
    fds_threadpool           *pr_thrpool;
};

extern ProbeS3Eng gl_probeS3Eng;

}  // namespace fds

#endif  // SOURCE_INCLUDE_FDS_PROBE_S3_PROBE_H_
