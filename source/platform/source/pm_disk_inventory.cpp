/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

/*
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <mntent.h>
#include <string>
#include <libudev.h>

#include <fds_uuid.h>
#include <fds_process.h>
#include <disk_partition.h>
#include <shared/fds-constants.h>


#include "disk_inventory.h"
#include "disk_label.h"
#include "disk_op.h"

#include "disk_plat_module.h"
#include "file_disk_inventory.h"
#include "file_disk_obj.h"

// needed

*/

#include <disk-constants.h>

#include "disk_part_mgr.h"
#include "disk_common.h"

#include "platform_disk_inventory.h"   // TODO(donavan), should be renamed to pm_disk_inventory.h

#include "disk_print_iter.h"
#include "disk_op.h"
#include "disk_label_op.h"
#include "disk_label_mgr.h"

namespace fds
{
    // -------------------------------------------------------------------------------------
    // PM Disk Inventory
    // -------------------------------------------------------------------------------------
    PmDiskInventory::PmDiskInventory() : DiskInventory(), dsk_qualify_cnt(0)
    {
        dsk_partition = new DiskPartMgr();
    }

    PmDiskInventory::~PmDiskInventory()
    {
        delete dsk_partition;

        // Setup conditions as if all disks are gone after the discovery.
        dsk_prev_inv.chain_transfer(&dsk_curr_inv);
        dsk_prev_inv.chain_transfer(&dsk_discovery);
        dsk_discovery_done();
    }

    // rs_new
    // ------
    //
    Resource *PmDiskInventory::rs_new(const ResourceUUID &uuid)
    {
        return new PmDiskObj();
    }

    // dsk_get_info
    // ------------
    //
    PmDiskObj::pointer PmDiskInventory::dsk_get_info(dev_t dev_node)
    {
        return NULL;
    }

    PmDiskObj::pointer PmDiskInventory::dsk_get_info(dev_t dev_node, int partition)
    {
        return NULL;
    }

    PmDiskObj::pointer PmDiskInventory::dsk_get_info(const std::string &blk_path)
    {
        PmDiskObj::pointer    disk;

        rs_mtx.lock();
        disk = dsk_dev_map[blk_path];
        rs_mtx.unlock();

        return disk;
    }

    PmDiskObj::pointer PmDiskInventory::dsk_get_info(const std::string &blk_path,
                                                     const std::string &dev_path)
    {
        PmDiskObj::pointer    disk;

        disk = dsk_get_info(blk_path);

        if (disk != NULL)
        {
            return disk->dsk_get_info(dev_path);
        }
        return NULL;
    }

    // dsk_discovery_begin
    // -------------------
    //
    void PmDiskInventory::dsk_discovery_begin()
    {
        LOGNORMAL << "Begin disk discovery...";
        rs_mtx.lock();

        if (dsk_prev_inv.chain_empty_list() && dsk_discovery.chain_empty_list())
        {
            dsk_prev_inv.chain_transfer(&dsk_curr_inv);
            fds_assert(dsk_curr_inv.chain_empty_list());
        }
        dsk_qualify_cnt = 0;
        rs_mtx.unlock();
    }

    // dsk_discovery_add
    // -----------------
    //
    void PmDiskInventory::dsk_discovery_add(PmDiskObj::pointer disk, PmDiskObj::pointer ref)
    {
        fds_verify(disk->dsk_common != NULL);
        rs_mtx.lock();

        // Add to the map indexed by full HW device path.
        if (ref == NULL)
        {
            dsk_dev_map[disk->dsk_common->dsk_blk_path] = disk;
        }else {
            fds_assert(dsk_dev_map[disk->dsk_common->dsk_blk_path] == ref->dsk_parent);
        }
        dsk_discovery.chain_add_back(&disk->dsk_disc_link);

        if (disk->dsk_parent == disk)
        {
            DiskInventory::dsk_add_to_inventory_mtx(disk, &dsk_hdd);

            if (disk->dsk_capacity_gb() >= DISK_ALPHA_CAPACITY_GB)
            {
                dsk_qualify_cnt++;
            }else {
                LOGNORMAL << " disk rejected due to capacity under sized ..." << disk;
            }
        }else {
            DiskInventory::dsk_add_to_inventory_mtx(disk, NULL);
        }
        rs_mtx.unlock();
    }

    // dsk_discovery_update
    // --------------------
    //
    void PmDiskInventory::dsk_discovery_update(PmDiskObj::pointer disk)
    {
        fds_verify(disk->dsk_common != NULL);
        rs_mtx.lock();
        fds_assert(dsk_dev_map[disk->dsk_common->dsk_blk_path] != NULL);

        // Rechain this obj to the discovery list.
        disk->dsk_disc_link.chain_rm();
        dsk_discovery.chain_add_back(&disk->dsk_disc_link);
        rs_mtx.unlock();
    }

    // dsk_discovery_remove
    // --------------------
    //
    void PmDiskInventory::dsk_discovery_remove(PmDiskObj::pointer disk)
    {
        rs_mtx.lock();

        // Unlink the chain and unmap every indices.
        disk->dsk_disc_link.chain_rm_init();

        if (disk->dsk_parent == disk)
        {
            fds_assert(dsk_dev_map[disk->dsk_common->dsk_blk_path] != NULL);
            dsk_dev_map.erase(disk->dsk_common->dsk_blk_path);
        }
        DiskInventory::dsk_remove_out_inventory_mtx(disk);
        rs_mtx.unlock();

        // Break free from the inventory.
        rs_free_resource(disk);
    }

    // dsk_discovery_done
    // ------------------
    //
    void PmDiskInventory::dsk_discovery_done()
    {
        ChainList             rm;
        PmDiskObj::pointer    disk;

        rs_mtx.lock();
        // Anything left over in the prev inventory list are phantom devices.
        dsk_curr_inv.chain_transfer(&dsk_discovery);
        rm.chain_transfer(&dsk_prev_inv);

        fds_assert(dsk_prev_inv.chain_empty_list());
        fds_assert(dsk_discovery.chain_empty_list());
        rs_mtx.unlock();

        while (1)
        {
            disk = rm.chain_rm_front<PmDiskObj>();

            if (disk == NULL)
            {
                break;
            }
            dsk_discovery_remove(disk);
            disk = NULL;  // free the disk obj when refcnt = 0
        }
        fds_assert(rm.chain_empty_list());
        LOGNORMAL << "End disk discovery...";
    }

    // dsk_dump_all
    // ------------
    //
    void PmDiskInventory::dsk_dump_all()
    {
        DiskPrintIter    iter;
        dsk_foreach(&iter);
    }

    // dsk_need_simulation
    // -------------------
    //
    bool PmDiskInventory::dsk_need_simulation()
    {
        uint32_t    hdd_count;

        /* Remove ssd disks */
        hdd_count = dsk_count - DISK_ALPHA_COUNT_SSD;

        if ((hdd_count <= dsk_qualify_cnt) && (hdd_count >= DISK_ALPHA_COUNT_HDD_MIN))
        {
            // if (dsk_qualify_cnt >= DISK_ALPHA_COUNT_HDD_MIN) {
            return false;
        }
        LOGNORMAL << "Need to run in simulation";
        return true;
    }

    // disk_do_partition
    // -----------------
    //
    void PmDiskInventory::dsk_do_partition()
    {
        DiskOp    op(DISK_OP_CHECK_PARTITION, dsk_partition);

        // For now, go through all HDD & check for parition.
        // dsk_foreach(dsk_partition, &op, &dsk_hdd, dsk_count);
    }

    // dsk_mount_all
    // -------------
    //
    void PmDiskInventory::dsk_mount_all()
    {
    }

    // dsk_admit_all
    // -------------
    //
    void PmDiskInventory::dsk_admit_all()
    {
    }

    // disk_read_label
    // --------------
    //
    bool PmDiskInventory::disk_read_label(DiskLabelMgr *mgr, bool creat)
    {
        DiskLabelOp    op(DISK_LABEL_READ, mgr);

        dsk_foreach(&op);

        return mgr->dsk_reconcile_label(this, creat);
    }
}  // namespace fds
