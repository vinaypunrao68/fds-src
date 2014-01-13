/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_FDS_PROBE_INCLUDE_UTEST_TYPES_H_
#define SOURCE_FDS_PROBE_INCLUDE_UTEST_TYPES_H_

#include <fds-probe/js-object.h>

namespace fds {

/**
 * Unit Test Thread param object
 */
typedef struct ut_thr_param ut_thr_param_t;
struct ut_thr_param
{
    int                      thr_min;
    int                      thr_max;
    int                      thr_spawn_thres;
    int                      thr_idle_sec;
};

class UT_Thread : public JsObject
{
  public:
    virtual JsObject *js_exec_obj(JsObjTemplate *templ);

    inline ut_thr_param_t *thr_param() {
        return static_cast<ut_thr_param_t *>(js_pod_object());
    }
};

class UT_ThreadTempl : public JsObjTemplate
{
  public:
    virtual ~UT_ThreadTempl() {}
    explicit UT_ThreadTempl(JsObjManager *mgr)
        : JsObjTemplate("Threads", mgr) {}

    virtual JsObject *js_new(json_t *in)
    {
        ut_thr_param_t *p = new ut_thr_param_t;
        if (json_unpack(in, "{s:i, s:i, s:i, s:i}",
                "tp-min-thr",       &p->thr_min,
                "tp-max-thr",       &p->thr_max,
                "tp-spawn-thres",   &p->thr_spawn_thres,
                "tp->idle-seconds", &p->thr_idle_sec)) {
            return NULL;
        }
        return js_parse(new UT_Thread(), in, p);
    }
};

/**
 */
typedef struct ut_srv_param ut_srv_param_t;
struct ut_srv_param
{
    int                      srv_port;
};

class UT_Server : public JsObject
{
  public:
    virtual JsObject *js_exec_obj(JsObjTemplate *templ);

    inline ut_srv_param_t *srv_param() {
        return static_cast<ut_srv_param_t *>(js_pod_object());
    }
};

class UT_ServerTempl : public JsObjTemplate
{
  public:
    virtual ~UT_ServerTempl() {}
    explicit UT_ServerTempl(JsObjManager *mgr) : JsObjTemplate("Server", mgr) {}

    virtual JsObject *js_new(json_t *in)
    {
        ut_srv_param_t *p = new ut_srv_param_t;
        if (json_unpack(in, "{s:i}",
                "srv-port",     &p->srv_port)) {
            return NULL;
        }
        return js_parse(new UT_Server(), in, p);
    }
};

/**
 */
typedef struct ut_load_param ut_load_param_t;
struct ut_load_param
{
    int                      rt_loop;
    int                      rt_concurrent;
    int                      rt_duration_sec;
};

class UT_Load : public JsObject
{
  public:
    virtual JsObject *js_exec_obj(JsObjTemplate *templ);

    inline ut_load_param_t *load_param() {
        return static_cast<ut_load_param_t *>(js_pod_object());
    }
};

class UT_LoadTempl : public JsObjTemplate
{
  public:
    virtual ~UT_LoadTempl() {}
    explicit UT_LoadTempl(JsObjManager *mgr) : JsObjTemplate("Loads", mgr) {}

    virtual JsObject *js_new(json_t *in)
    {
        ut_load_param_t *p = new ut_load_param_t;
        if (json_unpack(in, "{s:i, s:i, s:i}",
                "rt-loop",         &p->rt_loop,
                "rt-concurrent",   &p->rt_concurrent,
                "rt-duration-sec", &p->rt_duration_sec)) {
            return NULL;
        }
        return js_parse(new UT_Load(), in, p);
    }
};

}   // namespace fds

#endif  // SOURCE_FDS_PROBE_INCLUDE_UTEST_TYPES_H_
