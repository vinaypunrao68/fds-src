/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include "fds_process.h"
#include "disk_constants.h"
#include "disk_part_mgr.h"
#include "disk_common.h"
#include "pm_disk_inventory.h"
#include "disk_print_iter.h"
#include "disk_op.h"
#include "disk_label_op.h"
#include "disk_label_mgr.h"
#include "disk_capability_op.h"
#include "platform/disk_capabilities.h"

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
        dsk_discovery_done();
        clear_inventory();
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

        fds_assert(dsk_discovery.chain_empty_list());
        clear_inventory();

        rs_mtx.lock();
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
            if (disk->dsk_capacity_gb() >= DISK_MINIMUM_CAPACITY_GB)
            {
                dsk_qualify_cnt++;
                DiskInventory::dsk_add_to_inventory_mtx(disk, &dsk_hdd);
            }else {
                LOGNORMAL << "Undersize disk (under " << DISK_MINIMUM_CAPACITY_GB << "GB) removed from usage consideration (device = " << disk << ")";
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
        PmDiskObj::pointer    disk;

        rs_mtx.lock();
        dsk_curr_inv.chain_transfer(&dsk_discovery);

        fds_assert(dsk_discovery.chain_empty_list());
        rs_mtx.unlock();

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

        FdsConfigAccessor    conf(g_fdsprocess->get_conf_helper());

        static bool mode_logged = false;  // We spam the log everytime we call this function
                                          // use this to only log our mode once.

        try
        {
            bool use_feature_control_disk_simulation = conf.get_abs<bool>("fds.feature_toggle.pm.control_disk_simulation_mode");   // NOLINT

            if (use_feature_control_disk_simulation)
            {
                if (conf.get_abs<bool>("fds.pm.force_disk_simulation"))
                {
                    if (! mode_logged)
                    {
                        mode_logged = true;
                        LOGNORMAL << "Forcing disk simulation mode via platform.conf";
                    }

                    return true;
                }
            }
        }
        catch (fds::Exception e)
        {
            LOGDEBUG << e.what();
        }

        /* Remove ssd disks */
        hdd_count = dsk_count - DISK_ALPHA_COUNT_SSD;

        if ((hdd_count <= dsk_qualify_cnt) && (hdd_count >= DISK_ALPHA_COUNT_HDD_MIN))
        {
            return false;
        }

        if (! mode_logged)
        {
            mode_logged = true;
            LOGNORMAL << "Need to run in disk simulation mode";
        }

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

    // disk_capabilities
    // --------------
    //
    bool PmDiskInventory::disk_read_capabilities(DiskCapabilitiesMgr *mgr)
    {
        DiskCapabilityOp    op(DISK_CAPABILITIES_READ, mgr);

        // First partition contains the capabilities
        dsk_foreach(&op);

        return true;
    }

    // disk_reconcile_label
    // --------------
    //
    void PmDiskInventory::disk_reconcile_label(DiskLabelMgr *mgr, NodeUuid node_uuid)
    {
        DiskLabelOp    op(DISK_LABEL_READ, mgr);

        dsk_foreach(&op);

        mgr->dsk_reconcile_label(dsk_need_simulation(), node_uuid);
    }

    void PmDiskInventory::clear_inventory()
    {
        PmDiskObj::pointer    disk;

        while (1)
        {
            disk = dsk_curr_inv.chain_rm_front<PmDiskObj>();

            if (disk == NULL)
            {
                break;
            }
            dsk_discovery_remove(disk);
        }
    }
}  // namespace fds
