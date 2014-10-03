/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <fds_assert.h>
#include <persistent-layer/dm_io.h>

namespace diskio {

// \IndexRequest::IndexRequest
// ---------------------------
//
IndexRequest::IndexRequest(meta_obj_id_t const &oid,
                           bool                block)
    : Request(block), idx_peer(nullptr)
{
    idx_oid = oid;
    obj_map_init(&idx_vmap);
    memset(&idx_phy_loc, 0, sizeof(idx_phy_loc));
}

IndexRequest::IndexRequest(meta_vol_io_t const &vio,
                           bool                block)
    : Request(block), idx_peer(nullptr)
{
    idx_vio = vio;
    obj_id_set_inval(&idx_oid);
    obj_map_init(&idx_vmap);
    memset(&idx_phy_loc, 0, sizeof(idx_phy_loc));
}

IndexRequest::IndexRequest(meta_obj_id_t const &oid,
                           meta_vol_io_t const &vio,
                           bool                block)
    : Request(block), idx_peer(nullptr)
{
    idx_oid = oid;
    idx_vio = vio;
    memset(&idx_vmap, 0, sizeof(idx_vmap));
    memset(&idx_phy_loc, 0, sizeof(idx_phy_loc));
}

// \IndexRequest::req_abort
// ------------------------
//
void
IndexRequest::req_abort()
{
}

// \IndexRequest::req_submit
// -------------------------
//
void
IndexRequest::req_submit()
{
}

// \IndexRequest::req_wait_cond
// ----------------------------
//
bool
IndexRequest::req_wait_cond()
{
    return fdsio::Request::req_wait_cond();
}

// \IndexRequest::req_set_wakeup_cond
// ----------------------------------
//
void
IndexRequest::req_set_wakeup_cond()
{
    fdsio::Request::req_set_wakeup_cond();
}

// \DiskRequest::DiskRequest
// -------------------------
//
DiskRequest::DiskRequest(meta_vol_io_t       &vio,
                         meta_obj_id_t       &oid,
                         fds::ObjectBuf      *buf,
                         bool                block)
    : IndexRequest(oid, vio, block), dat_buf(buf),
      datTier(diskTier)
{
}

DiskRequest::DiskRequest(meta_vol_io_t       &vio,
                         meta_obj_id_t       &oid,
                         fds::ObjectBuf      *buf,
                         bool                block,
                         DataTier            tier)
    : DiskRequest(vio, oid, buf, block)
{
    datTier = tier;
}

DiskRequest::~DiskRequest()
{
}

}  // namespace diskio
