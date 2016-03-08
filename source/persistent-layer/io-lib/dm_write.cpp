/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#include <persistent-layer/persistentdata.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <fds_error.h>
#include <fiu-local.h>
#include <fiu-control.h>

namespace diskio {

FilePersisDataIO::FilePersisDataIO(char const *const file,
                                   fds_uint16_t id, int loc)
        : fi_path(file),
          fi_id(id),
          fi_loc(loc),
          fi_mutex("file mutex"),
          fi_del_objs(0),
          fi_del_blks(0)
{
    fi_fd = open(file, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
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
    fi_cur_off = DataIO::disk_io_round_up_blk(fi_cur_off);
}

FilePersisDataIO::~FilePersisDataIO()
{
    if (fi_fd > 0) {
        close(fi_fd);
    }
}

fds::Error
FilePersisDataIO::delete_file()
{
    fds::Error err(fds::ERR_OK);
    int fd = fi_fd;
    fi_mutex.lock();
    fi_fd = -1;
    fi_mutex.unlock();

    close(fd);
    int ret = unlink(fi_path.c_str());
    if (ret < 0) {
        printf("Can't unlink file %s\n", fi_path.c_str());
        perror("Reason: ");
        return fds::Error(fds::ERR_DISK_FILE_UNLINK_FAILED);
    }
    return err;
}

fds::Error
diskio::FilePersisDataIO::disk_do_write(DiskRequest *req)
{
    fds::Error err(fds::ERR_OK);
    ssize_t         len = 0;
    fds_blk_t       off_blk, blk, shft;
    meta_obj_map_t  *map;
    fds::ObjectBuf const *const buf = req->req_obj_buf();

    blk = DataIO::disk_io_round_up_blk(buf->getSize());

    fi_mutex.lock();
    if (fi_fd < 0) {
        return fds::ERR_FILE_DOES_NOT_EXIST;
    }
    off_blk    = fi_cur_off;
    fi_cur_off = fi_cur_off + blk;
    fi_mutex.unlock();

    map  = req->req_get_vmap();
    shft = DataIO::disk_io_blk_shift();

    map->obj_blk_len     = blk;
    obj_phy_loc_t *idx_phy_loc = req->req_get_phy_loc();
    idx_phy_loc->obj_stor_offset = off_blk;
    idx_phy_loc->obj_stor_loc_id = disk_loc_id();
    idx_phy_loc->obj_file_id = file_id();
    idx_phy_loc->obj_tier        = static_cast<fds_uint8_t>(req->getTier());
    map->obj_size        = buf->getSize();

    fds_uint32_t retry_cnt =0;
    off_blk <<= shft;
    while (retry_cnt++ < 3 && len != buf->getSize()) {
        len = pwrite64(fi_fd, static_cast<const void *>((buf->data)->c_str()),
                       buf->getSize(), off_blk);
        fiu_do_on("sm.persist.writefail", len = -1; );
        if (len < 0) {
            // perror("Error: ");
            err = fds::ERR_DISK_WRITE_FAILED;
            break;
        }
        if (len == buf->getSize()) {
            break;
        }
    }
    if (len != buf->getSize()) {
        // perror("Error: ");
        err = fds::ERR_DISK_WRITE_FAILED;
    }
    disk_write_done(req);
    return err;
}

void
FilePersisDataIO::disk_do_delete(fds_uint32_t obj_size)
{
    fds_blk_t blk = DataIO::disk_io_round_up_blk(obj_size);
    fi_del_blks += blk;
    ++fi_del_objs;
}

fds_uint64_t
FilePersisDataIO::get_total_bytes() const
{
    fds_blk_t shft = DataIO::disk_io_blk_shift();
    fds_uint64_t bytes = fi_cur_off << shft;
    return bytes;
}

fds_uint64_t
FilePersisDataIO::get_deleted_bytes() const
{
    fds_blk_t shft = DataIO::disk_io_blk_shift();
    fds_uint64_t bytes = fi_del_blks << shft;
    return bytes;
}

}  // namespace diskio
