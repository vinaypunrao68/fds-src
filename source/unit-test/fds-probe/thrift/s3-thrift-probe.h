/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_UNIT_TEST_FDS_PROBE_THRIFT_S3_THRIFT_PROBE_H_
#define SOURCE_UNIT_TEST_FDS_PROBE_THRIFT_S3_THRIFT_PROBE_H_

/*
 * Header file Template for probe adapter.  Replace XX with your name space.
 */
#include <string>
#include <utest-types.h>
#include <fds-probe/fds_probe.h>

namespace fds {

class Thrift_ProbeMod : public ProbeMod
{
  public:
    Thrift_ProbeMod(char const *const name,
                    probe_mod_param_t *param, Module *owner)
        : ProbeMod(name, param, owner) {}
    virtual ~Thrift_ProbeMod() {}

    void pr_intercept_request(ProbeRequest *req);
    void pr_put(ProbeRequest *req);
    void pr_get(ProbeRequest *req);
    void pr_delete(ProbeRequest *req);
    void pr_verify_request(ProbeRequest *req);
    void pr_gen_report(std::string *out);

    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();

    Thrift_ProbeMod *pr_new_instance();
};

// S3Thrift Probe Adapter.
//
extern Thrift_ProbeMod           gl_thriftProbeMod;

/*
 * ------------------------------------------------------------------------------------
 * Export API to web
 * ------------------------------------------------------------------------------------
 */

/**
 * Decoder for:
 * "ProbeFoo": {
 *     "p1": 12,
 *     "p2": 32,
 *     "p3": "This is a test",
 *     "func": "hello"
 * }
 */
typedef struct am_foo_arg am_foo_arg_t;
struct am_foo_arg
{
    fds_uint32_t    am_p1, am_p2;
    const char     *am_p3;
    const char     *am_func;
};

class ProbeAmFoo : public JsObject
{
  public:
    virtual JsObject *js_exec_obj(JsObject *p, JsObjTemplate *tmpl, JsObjOutput *out);
    inline am_foo_arg_t *am_probe_foo() {
        return static_cast<am_foo_arg_t *>(js_pod_object());
    }
};

class ProbeAmFooTmpl : public JsObjTemplate
{
  public:
    virtual ~ProbeAmFooTmpl() {}
    explicit ProbeAmFooTmpl(JsObjManager *mgr) : JsObjTemplate("ProbeFoo", mgr) {}

    virtual JsObject *js_new(json_t *in)
    {
        am_foo_arg_t *p = new am_foo_arg;
        if (json_unpack(in, "{s:i s:i s:s s:s}",
                        "p1", &p->am_p1,
                        "p2", &p->am_p2,
                        "p3", &p->am_p3,
                        "func", &p->am_func)) {
            delete p;
            return NULL;
        }
        return js_parse(new ProbeAmFoo(), in, p);
    }
};

/**
 * Decoder for:
 * "ProbeGetMsgReqt": {
 *     "msg_len": 123,
 *     "func": "bye"
 * }
 */
typedef struct am_getmsg_reqt am_getmsg_reqt_t;
struct am_getmsg_reqt
{
    fds_uint32_t   msg_len;
    const char    *am_func;
};

class ProbeAmGetReqt : public JsObject
{
  public:
    virtual JsObject *js_exec_obj(JsObject *p, JsObjTemplate *tmpl, JsObjOutput *out);
    inline am_getmsg_reqt_t *am_getmsg_reqt() {
        return static_cast<am_getmsg_reqt_t *>(js_pod_object());
    }
};

class ProbeAmGetReqtTmpl : public JsObjTemplate
{
  public:
    virtual ~ProbeAmGetReqtTmpl() {}
    explicit ProbeAmGetReqtTmpl(JsObjManager *mgr)
        : JsObjTemplate("ProbeGetMsgReqt", mgr) {}

    virtual JsObject *js_new(json_t *in)
    {
        am_getmsg_reqt_t *p = new am_getmsg_reqt_t;
        if (json_unpack(in, "{s:i s:s}",
                        "msg_len", &p->msg_len,
                        "func", &p->am_func)) {
            delete p;
            return NULL;
        }
        return js_parse(new ProbeAmGetReqt(), in, p);
    }
};

/**
 * Decoder for:
 * "ProbeBar": {
 *     "p1": 43332,
 *     "p2": 453423,
 *     "p3": 3422,
 *     "p4": 4655,
 *     "p5": 56565,
 *     "func": "poke"
 * }
 */
typedef struct am_bar_arg am_bar_arg_t;
struct am_bar_arg
{
    fds_uint32_t    am_p1, am_p2, am_p3, am_p4, am_p5;
    const char     *am_func;
};

class ProbeAmBar : public JsObject
{
  public:
    virtual JsObject *js_exec_obj(JsObject *p, JsObjTemplate *tmpl, JsObjOutput *out);
    inline am_bar_arg_t *am_probe_bar() {
        return static_cast<am_bar_arg_t *>(js_pod_object());
    }
};

class ProbeAmBarTmpl : public JsObjTemplate
{
  public:
    virtual ~ProbeAmBarTmpl() {}
    explicit ProbeAmBarTmpl(JsObjManager *mgr) : JsObjTemplate("ProbeBar", mgr) {}

    virtual JsObject *js_new(json_t *in)
    {
        am_bar_arg_t *p = new am_bar_arg_t;
        if (json_unpack(in, "{s:i s:i s:i s:i s:i s:s}",
                        "p1", &p->am_p1,
                        "p2", &p->am_p2,
                        "p3", &p->am_p3,
                        "p4", &p->am_p4,
                        "p5", &p->am_p5,
                        "func", &p->am_func)) {
            delete p;
            return NULL;
        }
        return js_parse(new ProbeAmBar(), in, p);
    }
};

/**
 * Decoder for:
 * "oProbePutMsg": {
 *     "func": "probe_put",
 *     "msg_len": 123,
 *     "msg_data": "This is data from a put request through probe interface"
 * }
 */
typedef struct am_putmsg_arg am_putmsg_arg_t;
struct am_putmsg_arg
{
    fds_uint32_t    msg_len;
    const char     *msg_data;
    const char     *am_func;
};

class ProbeAmPutMsg : public JsObject
{
  public:
    virtual JsObject *js_exec_obj(JsObject *p, JsObjTemplate *tmpl, JsObjOutput *out);
    inline am_putmsg_arg_t *am_probe_putmsg() {
        return static_cast<am_putmsg_arg_t *>(js_pod_object());
    }
};

class ProbeAmPutTmpl : public JsObjTemplate
{
  public:
    virtual ~ProbeAmPutTmpl() {}
    explicit ProbeAmPutTmpl(JsObjManager *mgr) : JsObjTemplate("ProbePutMsg", mgr) {}

    virtual JsObject *js_new(json_t *in)
    {
        am_putmsg_arg_t *p = new am_putmsg_arg_t;
        if (json_unpack(in, "{s:s s:i s:s}",
                        "msg_data", &p->msg_data,
                        "msg_len", &p->msg_len,
                        "func", &p->am_func)) {
            delete p;
            return NULL;
        }
        return js_parse(new ProbeAmPutMsg(), in, p);
    }
};

/**
 * Decoder for:
 * AM {
 *     "ProbeFoo" {}
 *     "ProbeGetMsgReqt" {}
 *     "ProbeBar" {}
 *     "ProbePutMsg" {}
 * }
 */
class ProbeAmApiTempl : public JsObjTemplate
{
  public:
    virtual ~ProbeAmApiTempl() {}
    explicit ProbeAmApiTempl(JsObjManager *mgr) : JsObjTemplate("AM", mgr)
    {
        js_decode["ProbeFoo"]        = new ProbeAmFooTmpl(mgr);
        js_decode["ProbeGetMsgReqt"] = new ProbeAmGetReqtTmpl(mgr);
        js_decode["ProbeBar"]        = new ProbeAmBarTmpl(mgr);
        js_decode["ProbePutMsg"]     = new ProbeAmPutTmpl(mgr);
    }
    virtual JsObject *js_new(json_t *in) {
        return js_parse(new JsObject(), in, NULL);
    }
};

}  // namespace fds

#endif  // SOURCE_UNIT_TEST_FDS_PROBE_THRIFT_S3_THRIFT_PROBE_H_
