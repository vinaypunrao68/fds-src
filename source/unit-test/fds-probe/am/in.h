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

typedef struct nbd_vol_in nbd_vol_in_t;
struct nbd_vol_in
{
    fds_uint32_t		block_size;
    char *			name;
    char *			device;
    fds_uint32_t		vol_blocks;
    fds_uint32_t		uuid;
};

class NbdVolObj : public JsObject
{
  public:
    /**
     * You need to provide the implementation for this method.
     */
    virtual JsObject *
    js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out) override;

    inline nbd_vol_in_t * nbd_vol_in() {
        return static_cast<nbd_vol_in_t *>(js_pod_object());
    }
};

/**
 * Decoder for:
 * "NbdVol": { .. }
 */
class NbdVolTmpl : public JsObjTemplate
{
  public:
    explicit NbdVolTmpl(JsObjManager *mgr) : JsObjTemplate("nbd_vol", mgr) {}
    virtual JsObject *js_new(json_t *in) override
    {
        nbd_vol_in_t *p = new nbd_vol_in_t;
        if (json_unpack(in, "{"
                     "s:i "
                     "s:s "
                     "s:s "
                     "s:i "
                     "s:i "
                     "}",
                     "block_size", &p->block_size,
                     "name", &p->name,
                     "device", &p->device,
                     "vol_blocks", &p->vol_blocks,
                     "uuid", &p->uuid)) {
            delete p;
            return NULL;
        }
        return js_parse(new NbdVolObj(), in, p);
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
        js_decode["nbd_vol"] = new NbdVolTmpl(mgr);
    }
    virtual JsObject *js_new(json_t *in) {
        return js_parse(new JsObject(), in, NULL);
    }
};

}  // nsmespace fds
#endif  // _ABC__
