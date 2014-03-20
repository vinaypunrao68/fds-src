/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#include <persistentdata.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <fds_err.h>

namespace diskio {

FilePersisDataIO::FilePersisDataIO(char const *const file, int loc)
    : fi_path(file), fi_loc(loc), fi_mutex("file mutex")
{
    fi_fd = open(file, O_RDWR | O_CREAT);
    if (fi_fd < 0) {
        printf("Can't open file %s\n", file);
        perror("Reason: ");
        exit(1);
    }
    fi_cur_off = lseek64(fi_fd, 0, SEEK_END);
    if (fi_cur_off < 0) {
        perror("lseek64 fails: ");
        exit(1);
    }
    // Convert to block-size unit.
    fi_cur_off = fi_cur_off >> DataIO::disk_io_blk_shift();
}

FilePersisDataIO::~FilePersisDataIO()
{
    close(fi_fd);
}

fds::Error
diskio::FilePersisDataIO::disk_do_write(DiskRequest *req)
{
    ssize_t         len;
    fds_blk_t       off_blk, blk, shft;
    meta_obj_map_t  *map;
    fds::ObjectBuf const *const buf = req->req_obj_buf();

    blk = DataIO::disk_io_round_up_blk(buf->size);

    fi_mutex.lock();
    off_blk    = fi_cur_off;
    fi_cur_off = fi_cur_off + blk;
    fi_mutex.unlock();

    map  = req->req_get_vmap();
    shft = DataIO::disk_io_blk_shift();

    map->obj_blk_len     = blk;
    obj_phy_loc_t *idx_phy_loc = req->req_get_phy_loc();
    idx_phy_loc->obj_stor_offset = off_blk;
    idx_phy_loc->obj_stor_loc_id = disk_loc_id();
    idx_phy_loc->obj_tier        = static_cast<fds_uint8_t>(req->getTier());
    map->obj_size        = buf->size;

    len = pwrite64(fi_fd, static_cast<const void *>(buf->data.c_str()),
                   buf->size, off_blk << shft);
    if (len < 0) {
        perror("Error: ");
        return fds::ERR_DISK_WRITE_FAILED;
    }
    disk_write_done(req);
    return fds::ERR_OK;
}

}  // namespace diskio
