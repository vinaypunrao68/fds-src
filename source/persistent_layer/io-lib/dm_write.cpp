#include <persistentdata.h>
#include <stdio.h>
#include <unistd.h>

namespace diskio {

void
FilePersisDataIO::disk_do_write(DiskRequest *req)
{
    ssize_t         len;
    fds_blk_t       off_blk, blk, shft;
    meta_obj_map_t  *map;
    IndexRequest    *idx;
    fds::ObjectBuf const *const buf = req->req_obj_buf();

    blk = DataIO::disk_io_round_up_blk(buf->size);

    fi_mutex.lock();
    off_blk    = fi_cur_off;
    fi_cur_off = fi_cur_off + blk;
    fi_mutex.unlock();

    map  = req->req_get_vmap();
    shft = DataIO::disk_io_blk_shift();

    map->obj_blk_len     = blk;
    map->obj_stor_offset = off_blk;
    map->obj_stor_loc_id = disk_loc_id();
    map->obj_tier        = static_cast<fds_uint8_t>(req->getTier());
    map->obj_size        = buf->size;

    idx = new IndexRequest(*req->req_get_oid(), true);
    idx->req_set_peer(req);
    req->req_set_peer(idx);

    len = pwrite64(fi_fd, (void *)buf->data.c_str(),
                   buf->size, off_blk << shft);
    if (len < 0) {
        perror("Error: ");
    }
    DataIndexProxy &index = DataIndexProxy::disk_index_singleton();
    index.disk_index_put(*idx, map);
    delete idx;
    disk_write_done(req);
}

} // namespace diskio
