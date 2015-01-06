/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_PROBE_S3_PROBE_H_
#define SOURCE_INCLUDE_FDS_PROBE_S3_PROBE_H_

#include <string>
#include <fds-probe/fds_probe.h>
#include <fds-probe/js-object.h>
#include <nginx-driver/s3connector.h>
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

// Probe PUT Bucket to hookup control path to the probe.
//
class Probe_PutBucket : public S3_PutBucket
{
  public:
    Probe_PutBucket(AMEngine *eng, AME_HttpReq *req);
    ~Probe_PutBucket();

    virtual void ame_request_handler();
    virtual int  ame_request_resume();
  protected:
};

// Doing this to avoid multiple inheritance.
//
class ProbeS3 : public ProbeMod
{
  public:
    /**
     * Implement required methods.
     */
    virtual ~ProbeS3();
    ProbeS3(char const *const name,
            probe_mod_param_t *param, Module *owner);

    ProbeMod *pr_new_instance() { return NULL; }
    void pr_intercept_request(ProbeRequest *req) {}
    void pr_put(ProbeRequest *req) {}
    void pr_get(ProbeRequest *req) {}
    void pr_delete(ProbeRequest *req) {}
    void pr_verify_request(ProbeRequest *req) {}
    void pr_gen_report(std::string *out) {}

    void mod_startup() {}
    void mod_shutdown() {}
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
    inline void probe_add_adapter(ProbeMod *adapter) {
        return probe_s3->pr_add_module(adapter);
    }
    inline ProbeMod *probe_get_adapter() {
        return probe_s3->pr_get_module();
    }
    inline fds_threadpool *probe_get_thrpool() {
        return probe_s3->pr_get_thrpool();
    }
    inline JsObjManager *probe_get_obj_mgr() {
        return probe_s3->pr_get_obj_mgr();
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
        return new Probe_PutBucket(this, req);
    }
    virtual Conn_DelBucket *ame_delbucker_hdler(AME_HttpReq *req) {
        return nullptr;
    }
  private:
    ProbeS3                  *probe_s3;
};

extern ProbeS3Eng gl_probeS3Eng;

}  // namespace fds

#endif  // SOURCE_INCLUDE_FDS_PROBE_S3_PROBE_H_
