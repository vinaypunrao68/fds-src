/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#include <persistentdata.h>
#include <stdio.h>
#include <unistd.h>
#include <fds_assert.h>
#include <fds_error.h>

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
    fds_uint32_t retry_cnt=0;
    size_t read_len = buf->size; 
    char *buffer = (char *)buf->data.c_str();
    while (retry_cnt++ < 3 && read_len > 0) { 
      len = pread64(fi_fd, (void *)buffer, read_len, off);
      if (len == buf->size) {
         break;
      } 
      if (len > 0 && len < buf->size ) { 
          read_len -= len;
          buffer += len;
          off += len;
      }
    }
    disk_read_done(req);
    if ( len < 0 ) {
	perror("read Error");
	err= fds::ERR_DISK_READ_FAILED;
    }
    return err; 
}

}  // namespace diskio
