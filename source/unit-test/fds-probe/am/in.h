/*
 * Copyright 2014 by Formations Data System, Inc.
 *
 * *** This code is auto generate, DO NOT EDIT! ***
 */
#include <shared/fds_types.h>
#include <fds_assert.h>
#include <jansson.h>
#include <fds-probe/fds_probe.h>

#ifndef _ABC_
#define _ABC_

namespace fds {

typedef struct sanjay_in sanjay_in_t;
struct sanjay_in
{
    fds_uint32_t		time;
    char *			date;
    fds_uint32_t		favorite;
};

class SanjayObj : public JsObject
{
  public:
    /**
     * You need to provide the implementation for this method.
     */
    virtual JsObject *
    js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out) override;

    inline sanjay_in_t * sanjay_in() {
        return static_cast<sanjay_in_t *>(js_pod_object());
    }
};

/**
 * Decoder for:
 * "Sanjay": { .. }
 */
class SanjayTmpl : public JsObjTemplate
{
  public:
    explicit SanjayTmpl(JsObjManager *mgr) : JsObjTemplate("sanjay", mgr) {}
    virtual JsObject *js_new(json_t *in) override
    {
        sanjay_in_t *p = new sanjay_in_t;
        if (json_unpack(in, "{"
                     "s:i "
                     "s:s "
                     "s:i "
                     "}",
                     "time", &p->time,
                     "date", &p->date,
                     "favorite", &p->favorite)) {
            delete p;
            return NULL;
        }
        return js_parse(new SanjayObj(), in, p);
    }
};

typedef struct bao_in bao_in_t;
struct bao_in
{
    char *			like;
    fds_uint32_t		v1;
    fds_uint32_t		v2;
    char *			v4;
};

class BaoObj : public JsObject
{
  public:
    /**
     * You need to provide the implementation for this method.
     */
    virtual JsObject *
    js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out) override;

    inline bao_in_t * bao_in() {
        return static_cast<bao_in_t *>(js_pod_object());
    }
};

/**
 * Decoder for:
 * "Bao": { .. }
 */
class BaoTmpl : public JsObjTemplate
{
  public:
    explicit BaoTmpl(JsObjManager *mgr) : JsObjTemplate("bao", mgr) {}
    virtual JsObject *js_new(json_t *in) override
    {
        bao_in_t *p = new bao_in_t;
        if (json_unpack(in, "{"
                     "s:s "
                     "s:i "
                     "s:i "
                     "s:s "
                     "}",
                     "like", &p->like,
                     "v1", &p->v1,
                     "v2", &p->v2,
                     "v4", &p->v4)) {
            delete p;
            return NULL;
        }
        return js_parse(new BaoObj(), in, p);
    }
};

typedef struct vy_in vy_in_t;
struct vy_in
{
    fds_uint32_t		v2;
    char *			v1;
    char *			v3;
};

class VyObj : public JsObject
{
  public:
    /**
     * You need to provide the implementation for this method.
     */
    virtual JsObject *
    js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out) override;

    inline vy_in_t * vy_in() {
        return static_cast<vy_in_t *>(js_pod_object());
    }
};

/**
 * Decoder for:
 * "Vy": { .. }
 */
class VyTmpl : public JsObjTemplate
{
  public:
    explicit VyTmpl(JsObjManager *mgr) : JsObjTemplate("vy", mgr) {}
    virtual JsObject *js_new(json_t *in) override
    {
        vy_in_t *p = new vy_in_t;
        if (json_unpack(in, "{"
                     "s:i "
                     "s:s "
                     "s:s "
                     "}",
                     "V2", &p->v2,
                     "v1", &p->v1,
                     "V3", &p->v3)) {
            delete p;
            return NULL;
        }
        return js_parse(new VyObj(), in, p);
    }
};

/**
 * Decoder for:
 * "Foo": { .. }
 */
class FooTmpl : public JsObjTemplate
{
  public:
    explicit FooTmpl(JsObjManager *mgr)
        : JsObjTemplate("foo", mgr)
    {
        js_decode["sanjay"] = new SanjayTmpl(mgr);
        js_decode["bao"] = new BaoTmpl(mgr);
        js_decode["vy"] = new VyTmpl(mgr);
    }
    virtual JsObject *js_new(json_t *in) {
        return js_parse(new JsObject(), in, NULL);
    }
};

/**
 * Decoder for:
 * "RunSetup": { .. }
 */
class RunSetupTmpl : public JsObjTemplate
{
  public:
    explicit RunSetupTmpl(JsObjManager *mgr)
        : JsObjTemplate("Run-Setup", mgr)
    {
        js_decode["foo"] = new FooTmpl(mgr);
    }
    virtual JsObject *js_new(json_t *in) {
        return js_parse(new JsObject(), in, NULL);
    }
};

}  // nsmespace fds
#endif  // _ABC__
