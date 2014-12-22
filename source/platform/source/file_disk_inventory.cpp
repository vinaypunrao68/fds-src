/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <shared/fds-constants.h>         // TODO(donavan) these need to be evaluated for
                                          // removal from global shared space to PM only space
#include "disk_plat_module.h"
#include "file_disk_inventory.h"
#include "file_disk_obj.h"
#include "disk_label_op.h"
#include "disk_label_mgr.h"
#include "disk_print_iter.h"
#include "disk_constants.h"

namespace fds
{
    FileDiskInventory::FileDiskInventory(const char *dir) : PmDiskInventory()
    {
        dsk_devdir = dir;
        FdsRootDir::fds_mkdir(dir);
    }

    FileDiskInventory::~FileDiskInventory()
    {
        dsk_prev_inv.chain_transfer(&dsk_files);
        dsk_discovery_done();
    }

    // dsk_mount_all
    // -------------
    //
    void FileDiskInventory::dsk_mount_all()
    {
    }

    // dsk_file_create
    // ---------------
    //
    void FileDiskInventory::dsk_file_create(const char *type, int count, ChainList *list)
    {
        int            i, len, end;
        char          *cur, dir[FDS_MAX_FILE_NAME];
        FileDiskObj   *file;

        len = snprintf(dir, sizeof(dir), "%s%s", dsk_devdir, type);
        cur = dir + len;
        len = sizeof(dir) - len;

        for (i = 0; i < count; i++)
        {
            end = snprintf(cur, len, "%d", i);
            LOGNORMAL << "Making " << dir << std::endl;
            file = new FileDiskObj(dir);
            file->dsk_read_uuid();

            rs_mtx.lock();
            DiskInventory::dsk_add_to_inventory_mtx(file, list);
            rs_mtx.unlock();
        }
    }

    // dsk_admit_all
    // -------------
    //
    void FileDiskInventory::dsk_admit_all()
    {
        dsk_file_create("hdd-", DISK_ALPHA_COUNT_HDD, &dsk_files);
        dsk_file_create("ssd-", DISK_ALPHA_COUNT_SSD, &dsk_files);
    }

    // disk_read_label
    // --------------
    //
    bool FileDiskInventory::disk_read_label(DiskLabelMgr *mgr, bool creat)
    {
        DiskLabelOp    op(DISK_LABEL_READ, mgr);

        dsk_foreach(&op, &dsk_files, dsk_count);

        return mgr->dsk_reconcile_label(this, creat);
    }

    // dsk_do_partition
    // ----------------
    //
    void FileDiskInventory::dsk_do_partition()
    {
    }

    // dsk_dump_all
    // ------------
    //
    void FileDiskInventory::dsk_dump_all()
    {
        DiskPrintIter    iter;
        dsk_foreach(&iter, &dsk_files, dsk_count);
    }
}  // namespace fds
