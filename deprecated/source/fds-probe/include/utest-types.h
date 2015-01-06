/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_FDS_PROBE_INCLUDE_UTEST_TYPES_H_
#define SOURCE_FDS_PROBE_INCLUDE_UTEST_TYPES_H_

#include <string>
#include <fds-probe/js-object.h>

namespace fds {

// ----------------------------------------------------------------------------
// "Run-Setup" Unit Test Section Spec
// ----------------------------------------------------------------------------

/**
 * Unit test Thread param object, type name "Threads"
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
    virtual JsObject *js_exec_obj(JsObject *parent,
                                  JsObjTemplate *templ, JsObjOutput *out);

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
                "tp-min-thr",      &p->thr_min,
                "tp-max-thr",      &p->thr_max,
                "tp-spawn-thres",  &p->thr_spawn_thres,
                "tp-idle-seconds", &p->thr_idle_sec)) {
            delete p;
            return NULL;
        }
        return js_parse(new UT_Thread(), in, p);
    }
};

/**
 * Unit test Server param object, type name "Server"
 */
typedef struct ut_srv_param ut_srv_param_t;
struct ut_srv_param
{
    int                      srv_port;
};

class UT_Server : public JsObject
{
  public:
    virtual JsObject *js_exec_obj(JsObject *parent,
                                  JsObjTemplate *templ, JsObjOutput *out);

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
            delete p;
            return NULL;
        }
        return js_parse(new UT_Server(), in, p);
    }
};

/**
 * Unit test Load param object, type name "Load"
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
    virtual JsObject *js_exec_obj(JsObject *parent,
                                  JsObjTemplate *templ, JsObjOutput *out);

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
            delete p;
            return NULL;
        }
        return js_parse(new UT_Load(), in, p);
    }
};

/**
 * Handle "Run-Setup" object.
 */
class UT_RunSetup : public JsObject
{
  public:
    virtual JsObject *js_exec_obj(JsObject *parent,
                                  JsObjTemplate *templ, JsObjOutput *out);

    inline UT_Thread *js_get_thread(int idx) {
        JsObject *obj = js_obj_field("Threads");
        if (obj != NULL) {
            return static_cast<UT_Thread *>((*obj)[idx]);
        }
        return NULL;
    }
    inline UT_Server *js_get_server() {
        return static_cast<UT_Server *>(js_obj_field("Server"));
    }
    inline UT_Load *js_get_load() {
        return static_cast<UT_Load *>(js_obj_field("Loads"));
    }
};

class UT_RunSetupTempl : public JsObjTemplate
{
  public:
    virtual ~UT_RunSetupTempl() {}
    explicit UT_RunSetupTempl(JsObjManager *mgr) : JsObjTemplate("Run-Setup", mgr)
    {
        js_decode["Threads"] = new UT_ThreadTempl(mgr);
        js_decode["Server"]  = new UT_ServerTempl(mgr);
        js_decode["Loads"]   = new UT_LoadTempl(mgr);
    }

    virtual JsObject *js_new(json_t *in) {
        return js_parse(new UT_RunSetup(), in, NULL);
    }
};

// ----------------------------------------------------------------------------
// "Run-Input" Unit Test Section Spec
// ----------------------------------------------------------------------------

/**
 * Unit test ServerLoad obj, type name "Server-Load"
 */
class UT_ServerLoad : public JsObject
{
  public:
    virtual JsObject *js_exec_obj(JsObject *parent,
                                  JsObjTemplate *templ, JsObjOutput *out);
};

class UT_ServerLoadTempl : public JsObjTemplate
{
  public:
    virtual ~UT_ServerLoadTempl() {}
    explicit UT_ServerLoadTempl(JsObjManager *mgr)
        : JsObjTemplate("Server-Load", mgr) {}

    virtual JsObject *js_new(json_t *in) {
        return js_parse(new UT_ServerLoad(), in, NULL);
    }
};

/**
 * Unit test ClientLoad obj, type name "Client-Load"
 */
typedef struct ut_client_param ut_client_param_t;
struct ut_client_param
{
    char                    *cl_script;
    char                    *cl_script_args;
    fds_bool_t               cl_cache;
};

class UT_ClientLoad : public JsObject
{
  public:
    virtual JsObject *js_exec_obj(JsObject *parent,
                                  JsObjTemplate *templ, JsObjOutput *out);

    inline ut_client_param_t *client_load_param() {
        return static_cast<ut_client_param_t *>(js_pod_object());
    }
};

class UT_ClientLoadTempl : public JsObjTemplate
{
  public:
    virtual ~UT_ClientLoadTempl() {}
    explicit UT_ClientLoadTempl(JsObjManager *mgr)
        : JsObjTemplate("Client-Load", mgr) {}

    virtual JsObject *js_new(json_t *in)
    {
        ut_client_param_t *p = new ut_client_param_t;
        if (json_unpack(in, "{s:s, s:s, s:b}",
                "script",          &p->cl_script,
                "script-args",     &p->cl_script_args,
                "cache-output",    &p->cl_cache)) {
            delete p;
            return NULL;
        }
        return js_parse(new UT_ClientLoad(), in, p);
    }
};

/**
 * Unit test RunInput obj, type name "Run-Input"
 */
class UT_RunInput : public JsObject
{
  public:
    virtual JsObject *js_exec_obj(JsObject *parent,
                                  JsObjTemplate *templ, JsObjOutput *out);

    inline UT_ServerLoad *js_get_server_load() {
        JsObject *obj = js_obj_field("Server-Load");
        if (obj != NULL) {
            return static_cast<UT_ServerLoad *>(obj);
        }
        return NULL;
    }
    inline UT_ClientLoad *js_get_client_load() {
        JsObject *obj = js_obj_field("Client-Load");
        if (obj != NULL) {
            return static_cast<UT_ClientLoad *>(obj);
        }
        return NULL;
    }
};

class UT_RunInputTemplate : public JsObjTemplate
{
  public:
    virtual ~UT_RunInputTemplate() {}
    explicit UT_RunInputTemplate(JsObjManager *mgr)
        : JsObjTemplate("Run-Input", mgr)
    {
        js_decode["Server-Load"] = new UT_ServerLoadTempl(mgr);
        js_decode["Client-Load"] = new UT_ClientLoadTempl(mgr);
    }

    virtual JsObject *js_new(json_t *in) {
        return js_parse(new UT_RunInput(), in, NULL);
    }
};

}   // namespace fds

#endif  // SOURCE_FDS_PROBE_INCLUDE_UTEST_TYPES_H_
