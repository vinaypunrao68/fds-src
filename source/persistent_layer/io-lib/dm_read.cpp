#include <persistentdata.h>
#include <stdio.h>
#include <unistd.h>
#include <fds_assert.h>

namespace diskio {

// \disk_io_read
// -------------
//
void
FilePersisDataIO::disk_do_read(DiskRequest *req)
{
    ssize_t                len;
    fds_uint64_t           off;
    fds::ObjectBuf *const  buf = req->req_obj_rd_buf();
    meta_obj_map_t         *map = req->req_get_vmap();

    if (obj_map_has_init_val(map)) {
        IndexRequest *idx = new IndexRequest(*req->req_get_oid(), true);
        DataIndexProxy &index = DataIndexProxy::disk_index_singleton();
        index.disk_index_get(*idx, map);
        delete idx;
    }
    fds_verify(map->obj_stor_loc_id == fi_loc);
    fds_verify(obj_map_has_init_val(map) == false);

    off = map->obj_stor_offset << DataIO::disk_io_blk_shift();
    len = pread64(fi_fd, (void *)buf->data.c_str(), buf->size, off);
    disk_read_done(req);
}

} // namespace diskio
