/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_UNIT_TEST_FDS_PROBE_PLATFORM_FDS_PROBE_QUEUE_H_
#define SOURCE_UNIT_TEST_FDS_PROBE_PLATFORM_FDS_PROBE_QUEUE_H_

/*
 * Header file template for probe adapter.  Replace Thrpool with your probe name
 */

#include <fds-probe/fds_probe.h>
#include <platform/fds-shm-queue.h>
#include <string>


namespace fds {

class shm_queue_ProbeMod : public ProbeMod
{
  public:
    shm_queue_ProbeMod(char const *const name, probe_mod_param_t *param, Module *owner)
        : ProbeMod(name, param, owner) {}
    virtual ~shm_queue_ProbeMod() {
        if (q != nullptr) {
            delete q;
        }
    }

    ProbeMod *pr_new_instance();
    void pr_intercept_request(ProbeRequest *req);
    void pr_put(ProbeRequest *req);
    void pr_get(ProbeRequest *req);
    void pr_delete(ProbeRequest *req);
    void pr_verify_request(ProbeRequest *req);
    void pr_gen_report(std::string *out);

    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();

  protected:
    fds::FdsShmQueue<int> *q;
};

// XX Probe Adapter.
//
extern shm_queue_ProbeMod           gl_shm_queue_ProbeMod;

/*
 * JSON stuffs
 */

struct queue_info_t {
    char *name;
    size_t size;
};

class QueueInfo : public JsObject
{
  public:
    virtual JsObject *
            js_exec_obj(JsObject *parent, JsObjTemplate *templ, JsObjOutput *out);

    inline queue_info_t  *queue_info() {
        return static_cast<queue_info_t *>(js_pod_object());
    }
};

/**
 * Decoder for Queue-Info
 */
class QueueInfoTempl : public JsObjTemplate
{
  public:
    virtual ~QueueInfoTempl() {}
    explicit QueueInfoTempl(JsObjManager *mgr)
            : JsObjTemplate("Queue-Info", mgr) {}

    virtual JsObject *js_new(json_t *in)
    {
        queue_info_t *p = new queue_info_t;
        if (json_unpack(in, "{s:s s:i}",
                        "queue-name", &p->name,
                        "queue-size", &p->size)) {
            delete p;
            return NULL;
        }
        return js_parse(new QueueInfo(), in, p);
    }
};


/**
 * Decoder for Queue-Setup
 */
class QueueSetupTempl : public JsObjTemplate
{
  public:
    virtual ~QueueSetupTempl() {}
    explicit QueueSetupTempl(JsObjManager *mgr)
            : JsObjTemplate("Queue-Setup", mgr)
    {
        js_decode["Queue-Info"] = new QueueInfoTempl(mgr);
    }
    virtual JsObject *js_new(json_t *in) {
        return js_parse(new JsObject(), in, NULL);
    }
};

//---------------------------------------------------------------------------------------

struct queue_pop_payload_t {
    char *name;
};

/**
 * QueuePop object
 */
class QueuePop : public JsObject
{
  public:
    virtual JsObject *js_exec_obj(JsObject *p, JsObjTemplate *templ, JsObjOutput *out);
    inline queue_pop_payload_t *queue_pop_payload() {
        return static_cast<queue_pop_payload_t *>(js_pod_object());
    }
};

/**
 * Decoder for Queue-Push
 */
class QueuePopTempl : public JsObjTemplate
{
  public:
    explicit QueuePopTempl(JsObjManager *mgr)
            : JsObjTemplate("Queue-Pop", mgr) {}

    virtual JsObject *js_new(json_t *in)
    {
        queue_pop_payload_t *payload = new queue_pop_payload_t;
        if (json_unpack(in, "{s:s}",
                        "queue-name", &payload->name)) {
            delete payload;
            return NULL;
        }
        return js_parse(new QueuePop(), in, payload);
    }
};

/**
 * Queue push payload information
 */
struct queue_push_payload_t {
    char *name;
    int value;
};

/**
 * QueuePush object
 */
class QueuePush : public JsObject
{
  public:
    virtual JsObject *js_exec_obj(JsObject *p, JsObjTemplate *templ, JsObjOutput *out);
    inline queue_push_payload_t *queue_push_payload() {
        return static_cast<queue_push_payload_t *>(js_pod_object());
    }
};

/**
 * Decoder for Queue-Push
 */
class QueuePushTempl : public JsObjTemplate
{
  public:
    explicit QueuePushTempl(JsObjManager *mgr)
            : JsObjTemplate("Queue-Push", mgr) {}

    virtual JsObject *js_new(json_t *in)
    {
        queue_push_payload_t *payload = new queue_push_payload_t;
        if (json_unpack(in, "{s:s s:i}",
                        "queue-name", &payload->name,
                        "value", &payload->value)) {
            delete payload;
            return NULL;
        }
        return js_parse(new QueuePush(), in, payload);
    }
};

/**
 * Decoder for Queue-Workload
 */
class QueueWorkloadTempl : public JsObjTemplate
{
  public:
    explicit QueueWorkloadTempl(JsObjManager *mgr)
            : JsObjTemplate("Queue-Workload", mgr)
    {
        js_decode["Queue-Push"] = new QueuePushTempl(mgr);
        js_decode["Queue-Pop"] = new QueuePopTempl(mgr);
    }
    virtual JsObject *js_new(json_t *in) {
        return js_parse(new JsObject(), in, NULL);
    }
};

}  // namespace fds
#endif  // SOURCE_UNIT_TEST_FDS_PROBE_PLATFORM_FDS_PROBE_QUEUE_H_
