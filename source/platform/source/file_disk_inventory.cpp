/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <shared/fds-constants.h>         // TODO(donavan) these need to be evaluated for
                                          // removal from global shared space to PM only space
#include "fds_process.h"
#include "fds_module.h"
#include "file_disk_inventory.h"
#include "file_disk_obj.h"
#include "disk_label_op.h"
#include "disk_label_mgr.h"
#include "disk_print_iter.h"
#include "disk_constants.h"
#include "disk_capability_op.h"
#include "platform/disk_capabilities.h"

namespace fds
{
    FileDiskInventory::FileDiskInventory(const char *dir) : PmDiskInventory()
    {
        dsk_devdir = dir;
        FdsRootDir::fds_mkdir(dir);
    }

    FileDiskInventory::~FileDiskInventory()
    {
        dsk_discovery_done();
        dsk_curr_inv.chain_transfer(&dsk_files);
        clear_inventory();
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
        int ssd_sim_count = DISK_ALPHA_COUNT_SSD;
        int hdd_sim_count = DISK_ALPHA_COUNT_HDD;

        FdsConfigAccessor    conf(g_fdsprocess->get_conf_helper());

        try
        {
            ssd_sim_count = conf.get_abs <int> ("fds.pm.disk_sim.ssd_count");   // NOLINT
            hdd_sim_count = conf.get_abs <int> ("fds.pm.disk_sim.hdd_count");   // NOLINT

            LOGDEBUG << "Overriding default simulation disk counts:  HDD count = " << hdd_sim_count << ", SSD count = " << ssd_sim_count;
        }
        catch (fds::Exception e)
        {
            LOGDEBUG << "Failure loading disk counts:  " << e.what();
        }

        dsk_file_create("hdd-", hdd_sim_count, &dsk_files);
        dsk_file_create("ssd-", ssd_sim_count, &dsk_files);
    }

    // disk_read_label
    // --------------
    //
    bool FileDiskInventory::disk_capabilities(DiskCapabilitiesMgr *mgr)
    {
        DiskCapabilityOp    op(DISK_CAPABILITIES_READ, mgr);

        dsk_foreach(&op, &dsk_files, dsk_count);

        return true;
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
