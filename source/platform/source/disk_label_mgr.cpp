/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include "platform/platform_consts.h"

#include "fds_process.h"
#include "disk_label.h"
#include "disk_label_mgr.h"

namespace fds
{
    extern FdsProcess *g_fdsprocess;

    // ------------------------------------------------------------------------------------
    // Clear and reset
    // ------------------------------------------------------------------------------------
    void DiskLabelMgr::clear()
    {
        dl_mtx.lock();
        dl_master = NULL;

        if (dl_map != NULL)
        {
            dl_map->close();
            delete dl_map;
            dl_map = NULL;
        }

        while (1)
        {
            DiskLabel   *curr = dl_labels.chain_rm_front<DiskLabel>();

            if (curr != NULL)
            {
                delete curr;
                continue;
            }
            break;
        }

        dl_total_disks = 0;
        dl_valid_labels = 0;

        dl_mtx.unlock();
    }


    // ------------------------------------------------------------------------------------
    // Disk Label Coordinator
    // ------------------------------------------------------------------------------------
    DiskLabelMgr::~DiskLabelMgr()
    {
        dl_master = NULL;

        if (dl_map != NULL)
        {
            dl_map->close();
            delete dl_map;
        }
        while (1)
        {
            DiskLabel   *curr = dl_labels.chain_rm_front<DiskLabel>();

            if (curr != NULL)
            {
                delete curr;
                continue;
            }
            break;
        }
    }

    DiskLabelMgr::DiskLabelMgr() : dl_master(NULL), dl_mtx("label mtx"), dl_map(NULL)
    {
    }

    // dsk_read_label
    // --------------
    // TODO(Vy): must guard this call so that we don't do the iteration twice, or alloc and
    // free this object instead of keeping it inside the module..
    //
    void DiskLabelMgr::dsk_read_label(PmDiskObj::pointer disk)
    {
        DiskLabel   *label = disk->dsk_xfer_label();

        if (label == NULL)
        {
            disk->dsk_read_uuid();
            label = disk->dsk_xfer_label();
            fds_assert(label != NULL);
        }

        fds_verify(label->dl_owner == disk);

        dl_mtx.lock();
        dl_labels.chain_add_back(&label->dl_link);
        dl_mtx.unlock();
    }

    // dsk_master_label_mtx
    // --------------------
    //
    DiskLabel *DiskLabelMgr::dsk_master_label_mtx()
    {
        if (dl_master != NULL)
        {
            return dl_master;
        }
        return dl_labels.chain_peek_front<DiskLabel>();
    }

    // dsk_reconcile_label
    // -------------------
    // TODO(Vy): redo this code.
    //
    bool DiskLabelMgr::dsk_reconcile_label(PmDiskInventory::pointer inv, bool creat)
    {
        bool         ret = false, need_to_relabel = false;
        int          valid_labels = 0;
        ChainIter    iter;

        DiskLabel   *label, *master, *curr, *chk;

        // If we dont' have a dl_map and create is true, open the diskmap truncating
        // any disk-map already present
#ifdef DEBUG
        bool fDumpDiskMap = g_fdsprocess->get_fds_config()->get<fds_bool_t>("fds.pm.dump_diskmap",false);
#endif
        if ((dl_map == NULL) && (creat == true))
        {
            const FdsRootDir   *dir = g_fdsprocess->proc_fdsroot();
            FdsRootDir::fds_mkdir(dir->dir_dev().c_str());

#ifdef DEBUG
            if (fDumpDiskMap)
            {
                dl_map = (std::ofstream*)(&std::cout);
            }
            else
            {
#endif

                dl_map = new std::ofstream(dir->dir_dev() + DISK_MAP_FILE, std::ofstream::out | std::ofstream::trunc);

#ifdef DEBUG
            }
#endif

        }

        dl_total_disks  = 0;
        dl_valid_labels = 0;
        master          = NULL;

        // Count the disks and disks with labels
        dl_mtx.lock();
        chain_foreach(&dl_labels, iter)
        {
            dl_total_disks++;
            label = dl_labels.chain_iter_current<DiskLabel>(iter);

            // Simple, no quorum scheme for now.
            if (label->dsk_label_valid())
            {
                dl_valid_labels++;
                GLOGDEBUG << label->dl_owner;
            }else {
                need_to_relabel = true;
                GLOGDEBUG << "Found invalid label on disk " << label->dl_owner;
            }
        }

        LOGNORMAL << "dl_total_disks = " << dl_total_disks << "   dl_valid_labels=" <<
        dl_valid_labels;

        if (dl_valid_labels > 0)
        {
            ret = (dl_valid_labels >= (dl_total_disks >> 1)) ? true : false;
        }

        if (inv->dsk_need_simulation() == true)
        {
            LOGNORMAL << "In simulation, found " << dl_valid_labels << " labels";
        }else {
            LOGNORMAL << "Scan HW inventory, found " << dl_valid_labels << " labels";
        }

        if (need_to_relabel && creat)
        {   // if there are some unlabeled disks, invalidate and re-label everything
            LOGNORMAL << "Found unlabeled disk(s), re-labeling all disks";
            dl_valid_labels = 0;
        }

        // Now iterate over all of the disks, relabel if needed and write to disk-map
        if (creat)
        {
            chain_foreach(&dl_labels, iter)
            {
                label = dl_labels.chain_iter_current<DiskLabel>(iter);
                bool is_good_disk = true;
                if (need_to_relabel)
                {
                    if (master == NULL)
                    {
                        label->dsk_label_generate(&dl_labels, dl_total_disks);
                        master = label;
                    }
                    else
                    {
                        label->dsk_label_clone(master);
                    }
                    is_good_disk = label->dsk_label_write(inv);
                    if (is_good_disk)
                    {
                        valid_labels++;
                    }
                    else
                    {
                        LOGWARN << "Failed writing label to disk "
                             << label->dl_owner << ". Skipping disk.";
                    }
                }
                if (is_good_disk)
                {
                    dsk_rec_label_map(label->dl_owner, label->dl_label->dl_my_disk_index);
                }
            }
        }

        dl_valid_labels += valid_labels;                    // rhs:local
        dl_mtx.unlock();
#if 0

        /* It's the bug here, master is still chained to the list. */
        if (master != NULL)
        {
            /* this is bug because master is still chained to the list. */
            delete master;
        }
#endif

        // End of the function -- if dl_map and create, close what was opened previously
        if ((dl_map != NULL) && (creat == true) && (dl_map != &std::cout)) {
            // This isn't thread-safe but we won't need dl_map in post-alpha.
            dl_map->flush();
            dl_map->close();
            delete dl_map;
            dl_map = NULL;

            LOGNORMAL << "Wrote total " << dl_valid_labels << " labels";
        }

        return ret;
    }

    // dsk_rec_label_map
    // -----------------
    //
    void DiskLabelMgr::dsk_rec_label_map(PmDiskObj::pointer disk, int idx)
    {
        if ((dl_map != NULL) && !disk->dsk_get_mount_point().empty())
        {
            char const *const    name = disk->rs_get_name();

            if (0 == strcmp(name, "/dev/sda")) //TODO: identify OS devices
            {
                return;
            }

            *dl_map << disk->rs_get_name() << " " << idx << " " << std::hex <<
            disk->rs_get_uuid().uuid_get_val() << std::dec << " " <<
            disk->dsk_get_mount_point().c_str()  << "\n";
        }
    }
}  // namespace fds
