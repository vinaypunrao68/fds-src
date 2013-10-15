#include <disk-mgr/dm_io.h>

namespace diskio {

// \IndexRequest
// -------------
//
IndexRequest::IndexRequest(const fdsio::RequestQueue &queue,
                           meta_obj_id_t             &oid,
                           bool                      block)
    : Request(queue, block)
{
}

IndexRequest::IndexRequest(const fdsio::RequestQueue &queue,
                           meta_vol_adr_t            &vio,
                           bool                      block)
    : Request(queue, block)
{
}

IndexRequest::IndexRequest(const fdsio::RequestQueue &queue,
                           meta_obj_id_t             &oid,
                           meta_vol_adr_t            &vio,
                           bool                      block)
    : Request(queue, block)
{
}

// \req_abort
// ----------
//
void
IndexRequest::req_abort()
{
}

// \req_submit
// -----------
//
void
IndexRequest::req_submit()
{
}

// \req_wait_cond
// --------------
//
bool
IndexRequest::req_wait_cond()
{
}

// \req_set_wakeup_cond
// --------------------
//
void
IndexRequest::req_set_wakeup_cond()
{
}

// \DiskRequest
// ------------
//
DiskRequest::DiskRequest(const fdsio::RequestQueue &queue,
                         meta_vol_io_t             &vio,
                         meta_obj_id_t             &oid,
                         fds::ObjectBuf            &buf,
                         bool                      block)
    : IndexRequest(queue, oid, block), dat_buf(buf)
{
}

DiskRequest::DiskRequest(const fdsio::RequestQueue &queue,
                         meta_vol_io_t             &vio,
                         meta_obj_id_t             &oid,
                         meta_obj_id_t             *old_oid,
                         meta_vol_io_t             *new_vol,
                         fds::ObjectBuf            &buf,
                         bool                      block)
    : IndexRequest(queue, oid, block), dat_buf(buf)
{
}

// \req_has_remap_oid
// ------------------
//
bool
DiskRequest::req_has_remap_oid(meta_obj_id_t *old_oid, meta_vol_adr_t *new_vol)
{
    return false;
}

// \DataIOFuncParams
// -----------------
//
DataIOFuncParams::DataIOFuncParams()
{
}

DataIOFuncParams::~DataIOFuncParams()
{
}

// \data_io_func
// -------------
//
int
DataIOFuncParams::data_io_func(fds_uint32_t time_delta, int *tier)
{
    return io_all_write;
}

// \DataIndexProxy
// ---------------
//
DataIndexProxy::DataIndexProxy(int nr_queue, int max_depth)
{
}

DataIndexProxy::~DataIndexProxy()
{
}

// \disk_index_put
// ---------------
//
void
DataIndexProxy::disk_index_put(IndexRequest &req, meta_obj_map_t &value)
{
}

void
DataIndexProxy::disk_index_put(IndexRequest &req, meta_vol_adr_t &value)
{
}

void
DataIndexProxy::disk_index_put(IndexRequest &req, meta_obj_id_t &value)
{
}

// \disk_index_get
// ---------------
//
void
DataIndexProxy::disk_index_get(IndexRequest &req, meta_obj_map_t *value)
{
}

void
DataIndexProxy::disk_index_get(IndexRequest &req, meta_vol_adr_t *value)
{
}

void
DataIndexProxy::disk_index_get(IndexRequest &req, meta_obj_id_t *value)
{
}

// \disk_index_update
// ------------------
//
void
DataIndexProxy::disk_index_update(IndexRequest &req, meta_obj_map_t &map)
{
}

// \disk_index_commit
// ------------------
//
void
DataIndexProxy::disk_index_commit(IndexRequest &req)
{
}

// \disk_index_remove
// ------------------
//
void
DataIndexProxy::disk_index_remove(IndexRequest &req)
{
}

// \disk_index_inc_ref
// -------------------
//
void
DataIndexProxy::disk_index_inc_ref(IndexRequest &req)
{
}

// \disk_index_dec_ref
// -------------------
//
void
DataIndexProxy::disk_index_dec_ref(IndexRequest &req)
{
}

} // namespace diskio
