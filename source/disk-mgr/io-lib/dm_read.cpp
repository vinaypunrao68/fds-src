#include <persistentdata.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <fds_assert.h>

namespace diskio {

FilePersisDataIO::FilePersisDataIO(char const *const file, int loc)
    : fi_path(file), fi_loc(loc), fi_mutex("file mutex")
{
    fi_fd = open(file, O_RDWR);
    fds_verify(fi_fd > 0);

    fi_cur_off = lseek64(fi_fd, 0, SEEK_END);
}

FilePersisDataIO::~FilePersisDataIO()
{
    close(fi_fd);
}

// \disk_io_read
// -------------
//
void
FilePersisDataIO::disk_do_read(DiskRequest *req)
{
    ssize_t                     len;
    fds_uint64_t                off;
    fds::ObjectBuf *const       buf = req->req_obj_rd_buf();
    meta_obj_map_t const *const map = req->req_get_vmap();

    fds_verify(map->obj_stor_loc_id == fi_loc);
    fds_verify(obj_map_has_init_val(map) == false);

    // off = map->obj_stor_offset << DataIO::disk_io_blk_shift();
    off = map->obj_stor_offset;
    len = pread64(fi_fd, (void *)buf->data.c_str(), buf->size, off);
    disk_read_done(req);
}

} // namespace diskio
