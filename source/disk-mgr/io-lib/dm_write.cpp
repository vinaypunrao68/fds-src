#include <persistentdata.h>
#include <stdio.h>
#include <unistd.h>

namespace diskio {

void
FilePersisDataIO::disk_do_write(DiskRequest *req)
{
    ssize_t         len;
    fds_uint64_t    off, shft;
    meta_obj_map_t  *map;
    fds::ObjectBuf const *const buf = req->req_obj_buf();

    fi_mutex.lock();
    off        = fi_cur_off;
    fi_cur_off = fi_cur_off + buf->size;
    fi_mutex.unlock();

    map  = req->req_get_vmap();
    shft = DataIO::disk_io_blk_shift();

    map->obj_blk_len     = buf->size >> shft;
    map->obj_stor_offset = off >> shft;
    map->obj_stor_loc_id = disk_loc_id();

    // TODO: Update both index and data in parallel.

    len = pwrite64(fi_fd, (void *)buf->data.c_str(), buf->size, off);
    if (len < 0) {
        perror("Error: ");
    }
    disk_write_done(req);
}

} // namespace diskio
