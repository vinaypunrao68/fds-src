/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#include <persistentdata.h>
#include <stdio.h>
#include <unistd.h>
#include <fds_assert.h>
#include <fds_err.h>

namespace diskio {

// \disk_io_read
// -------------
//
fds::Error
diskio::FilePersisDataIO::disk_do_read(DiskRequest *req)
{
    ssize_t         len;
    fds_uint64_t    off;
    fds::ObjectBuf *buf = req->req_obj_rd_buf();
    meta_obj_map_t *map = req->req_get_vmap();
    obj_phy_loc_t *phyloc = req->req_get_phy_loc();
    fds::Error  err(fds::ERR_OK);

    fds_verify(phyloc->obj_stor_loc_id == fi_loc);
    fds_verify(obj_map_has_init_val(map) == false);
    fds_verify(phyloc->obj_file_id == fi_id);
    fds_verify(fi_fd >= 0);

    off = phyloc->obj_stor_offset << DataIO::disk_io_blk_shift();
    len = pread64(fi_fd, (void *)buf->data.c_str(), buf->size, off);
    disk_read_done(req);
    if ( len < 0 ) {
	perror("read Error");
	err= fds::ERR_DISK_READ_FAILED;
    }
    return err; 
}

}  // namespace diskio
