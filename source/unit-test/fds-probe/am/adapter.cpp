/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <in.h>
#include <am-nbd.h>

namespace fds {

JsObject *
NbdVolObj::js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out)
{
    int              rt;
    blk_vol_creat_t  r;
    nbd_vol_in_t    *in  = nbd_vol_in();
    NbdBlockMod     *blk = NbdBlockMod::nbd_singleton();

    r.v_name = in->name;
    r.v_dev  = in->device;
    r.v_uuid = in->uuid;
    r.v_blksz     = in->block_size;
    r.v_vol_blksz = in->vol_blocks;
    r.v_test_vol_flag = in->test_vol_flag;
    rt = blk->blk_attach_vol(&r);
    return this;
}

}  // namespace fds
