/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <errno.h>
#include <linux/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <nbd-test-mod.h>
#include <util/Log.h>
#include <StorHvisorNet.h>
#include <fdsp/fds_service_types.h>
#include "net/SvcRequestPool.h"

namespace fds {

NbdSmMod                     gl_NbdSmMod;
NbdDmMod                     gl_NbdDmMod;

/**
 * nbd_vol_read
 * ------------
 */
void
NbdSmVol::nbd_vol_read(NbdBlkIO *vio)
{
}

/**
 * nbd_vol_write
 * -------------
 */
void
NbdSmVol::nbd_vol_write(NbdBlkIO *vio)
{
    char                 *buf;
    ssize_t               len;
    NbdSmVol::ptr         vol;
    fpi::PutObjectMsgPtr  put(bo::make_shared<fpi::PutObjectMsg>());

    vol = blk_vol_cast<NbdSmVol>(vio->nbd_vol);
    put->volume_id        = vol->vol_uuid;
    put->origin_timestamp = util::getTimeStampMillis();

    buf = vio->aio_buffer(&len);
    vio->nbd_cur_obj = ObjIdGen::genObjectId(buf, len);

    put->data_obj.assign(buf, len);
    put->data_obj_id.digest = std::string((const char *)vio->nbd_cur_obj.GetId(),
                                          vio->nbd_cur_obj.GetLen());

    auto dltMgr = BlockMod::blk_singleton()->blk_amc->om_client->getDltManager();
    auto putmsg = gSvcRequestPool->newQuorumSvcRequest(
            bo::make_shared<DltObjectIdEpProvider>(
                dltMgr->getDLT()->getNodes(vio->nbd_cur_obj)));

    putmsg->setPayload(FDSP_MSG_TYPEID(fpi::PutObjectMsg), put);
    putmsg->onResponseCb(RESPONSE_MSG_HANDLER(NbdSmVol::nbd_vol_write_cb, vio));
    putmsg->invoke();

    vio->aio_wait(&vol_mtx, &vol_waitq);
}

/**
 * nbd_vol_write_cb
 * ----------------
 */
void
NbdSmVol::nbd_vol_write_cb(NbdBlkIO                    *vio,
                           QuorumSvcRequest            *svcreq,
                           const Error                 &err,
                           bo::shared_ptr<std::string>  payload)
{
    vio->aio_wakeup(&vol_mtx, &vol_waitq);
}

/**
 * blk_alloc_vol
 * -------------
 */
BlkVol::ptr
NbdSmMod::blk_alloc_vol(const blk_vol_creat_t *r)
{
    if (ev_prim_loop == NULL) {
        ev_prim_loop = EV_DEFAULT;
        return new NbdSmVol(r, ev_prim_loop);
    }
    return new NbdSmVol(r, ev_loop_new(0));
}

/**
 * nbd_vol_read_cb
 * ---------------
 */
void
NbdSmVol::nbd_vol_read_cb(NbdBlkIO                    *vio,
                          FailoverSvcRequest          *svcreq,
                          const Error                 &err,
                          bo::shared_ptr<std::string>  payload)
{
}

/**
 * nbd_vol_read
 * ------------
 */
void
NbdDmVol::nbd_vol_read(NbdBlkIO *vio)
{
}

/**
 * nbd_vol_write
 * -------------
 */
void
NbdDmVol::nbd_vol_write(NbdBlkIO *vio)
{
    ssize_t             len, off;
    NbdDmVol::ptr       vol;
    std::stringstream   ss;
    fpi::FDSP_BlobObjectInfo    blob;
    fpi::UpdateCatalogOnceMsgPtr    upcat(bo::make_shared<fpi::UpdateCatalogOnceMsg>());

    vol = vio->nbd_vol;
    upcat->blob_name.assign(vol->vol_name);

    upcat->blob_version = blob_version_invalid;
    upcat->volume_id    = vol->vol_uuid;
    upcat->txId         = vol->vio_txid++;
    upcat->blob_mode    = false;

    off = vio->nbd_cur_off & ~vol->vol_blksz_mask;
    len = vio->nbd_cur_len + (vio->nbd_cur_len & vol->vol_blksz_mask);

    while (len > 0) {
        ss << off;
        blob.offset             = off;
        blob.size               = vol->vol_blksz_mask;
        blob.data_obj_id.digest = ss.str();

        if (len > vol->vol_blksz_byte) {
            len -= vol->vol_blksz_byte;
            off += vol->vol_blksz_byte;
        } else {
            len = 0;
        }
        upcat->obj_list.push_back(blob);
    }
    auto dmtMgr = BlockMod::blk_singleton()->blk_amc->om_client->getDmtManager();
    auto upcat_req = gSvcRequestPool->newQuorumSvcRequest(
                bo::make_shared<DmtVolumeIdEpProvider>(
                    dmtMgr->getCommittedNodeGroup(vol->vol_uuid)));

    upcat_req->setPayload(FDSP_MSG_TYPEID(fpi::UpdateCatalogOnceMsg), upcat);
    upcat_req->onResponseCb(RESPONSE_MSG_HANDLER(NbdBlkVol::nbd_vol_write_cb, vio));
    upcat_req->invoke();

    vio->aio_wait(&vol_mtx, &vol_waitq);
}

/**
 * nbd_vol_write_cb
 * ----------------
 */
void
NbdDmVol::nbd_vol_write_cb(NbdBlkIO                    *vio,
                           QuorumSvcRequest            *svcreq,
                           const Error                 &err,
                           bo::shared_ptr<std::string>  payload)
{
    vio->aio_wakeup(&vol_mtx, &vol_waitq);
}

/**
 * blk_alloc_vol
 * -------------
 */
BlkVol::ptr
NbdDmMod::blk_alloc_vol(const blk_vol_creat_t *r)
{
    if (ev_prim_loop == NULL) {
        ev_prim_loop = EV_DEFAULT;
        return new NbdDmVol(r, ev_prim_loop);
    }
    return new NbdDmVol(r, ev_loop_new(0));
}

/**
 * nbd_vol_read_cb
 * ------------
 */
void
NbdDmVol::nbd_vol_read_cb(NbdBlkIO                     *vio,
                          FailoverSvcRequest           *svcreq,
                          const Error                  &err,
                          bo::shared_ptr<std::string>  payload)
{
}

}   // namespace fds
