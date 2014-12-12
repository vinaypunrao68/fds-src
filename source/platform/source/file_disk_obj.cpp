/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <libudev.h>

#include "platform_disk_obj.h"

#include "disk_print_iter.h"
#include <shared/fds-constants.h>         // TODO(donavan) these need to be evaluated for
                                          // removal from global shared space to PM only space
#include "disk_plat_module.h"
#include "disk_label.h"

#include "file_disk_obj.h"

namespace fds
{
    // -------------------------------------------------------------------------------------
    // Disk simulation with files
    // -------------------------------------------------------------------------------------
    FileDiskObj::FileDiskObj(const char *dir) : PmDiskObj(), dsk_dir(dir)
    {
        char    path[FDS_MAX_FILE_NAME];

        FdsRootDir::fds_mkdir(dir);
        strncpy(rs_name, dir, sizeof(rs_name));

        snprintf(path, FDS_MAX_FILE_NAME, "%s/sblock", dir);
        dsk_fd = open(path, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    }

    FileDiskObj::~FileDiskObj()
    {
        close(dsk_fd);
    }

    // dsk_read
    // --------
    //
    ssize_t FileDiskObj::dsk_read(void *buf, fds_uint32_t sector, int sect_cnt)
    {
        fds_assert(buf != NULL);
        return pread(dsk_fd, buf, sect_cnt * DL_SECTOR_SZ, 0);
    }

    // dsk_write
    // ---------
    //
    ssize_t FileDiskObj::dsk_write(bool sim, void *buf, fds_uint32_t sector, int sect_cnt)
    {
        ssize_t    ret;

        ret = pwrite(dsk_fd, buf, sect_cnt * DL_SECTOR_SZ, 0);
        LOGNORMAL << rs_name << ", fd" << dsk_fd << ", ret " << ret << " : write at sector " <<
        sector << ", cnt " << sect_cnt;
        return ret;
    }
}  // namespace fds
