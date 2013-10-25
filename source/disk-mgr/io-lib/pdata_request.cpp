#include <fds_assert.h>
#include <disk-mgr/dm_io.h>

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
    vadr_set_inval(idx_vio.vol_adr);
}

IndexRequest::IndexRequest(meta_vol_io_t const &vio,
                           bool                block)
    : Request(block), idx_peer(nullptr)
{
    idx_vio = vio;
    obj_id_set_inval(&idx_oid);
    obj_map_init(&idx_vmap);
}

IndexRequest::IndexRequest(meta_obj_id_t const &oid,
                           meta_vol_io_t const &vio,
                           bool                block)
    : Request(block), idx_peer(nullptr)
{
    idx_oid = oid;
    idx_vio = vio;
    obj_map_init(&idx_vmap);
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
    fdsio::Request::req_wait_cond();
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
    : IndexRequest(oid, vio, block), dat_buf(buf)
{
    obj_id_set_inval(&dat_old_oid);
    vadr_set_inval(dat_new_vol.vol_adr);
}

DiskRequest::DiskRequest(meta_vol_io_t       &vio,
                         meta_obj_id_t       &oid,
                         meta_obj_id_t       *old_oid,
                         meta_vol_io_t       *new_vol,
                         fds::ObjectBuf      *buf,
                         bool                block)
    : IndexRequest(oid, block), dat_buf(buf)
{
    if (old_oid != nullptr) {
        dat_old_oid = *old_oid;
    } else {
        obj_id_set_inval(&dat_old_oid);
    }
    if (new_vol != nullptr) {
        dat_new_vol = *new_vol;
    } else {
        vadr_set_inval(dat_new_vol.vol_adr);
    }
}

DiskRequest::~DiskRequest()
{
}

// \DiskRequest::req_has_remap_oid
// -------------------------------
//
bool
DiskRequest::req_has_remap_oid(meta_obj_id_t *old_oid, meta_vol_io_t *new_vol)
{
    fds_assert(old_oid != nullptr);
    if (new_vol != nullptr) {
        *new_vol = dat_new_vol;
    }
    if (obj_id_is_valid(&dat_old_oid)) {
        *old_oid = dat_old_oid;
        return true;
    }
    return false;
}

// \DataIOFuncParams
// -----------------
//
DataIOFuncParams::DataIOFuncParams()
    : io_func(0), io_tier(io_tier_slow)
{
}

DataIOFuncParams::~DataIOFuncParams()
{
}

// \DataIOFuncParams::data_io_func
// -------------------------------
//
int
DataIOFuncParams::data_io_func(fds_uint32_t time_delta, int *tier)
{
    *tier = io_tier;
    return io_func;
}

} // namespace diskio
