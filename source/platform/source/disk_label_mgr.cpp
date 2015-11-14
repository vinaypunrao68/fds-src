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
        bool         ret = false;
        int          valid_labels = 0;
        ChainIter    iter;
        ChainList    upgrade;

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
            if (label->dsk_label_valid(this))
            {
                dl_valid_labels++;
                GLOGDEBUG << label->dl_owner;

                if (master == NULL)
                {
                    master = label;
                }
            }else {
                chk = dl_labels.chain_iter_rm_current<DiskLabel>(&iter);
                fds_verify(chk == label);

                upgrade.chain_add_back(&label->dl_link);
            }
        }

        LOGNORMAL << "dl_total_disks = " << dl_total_disks << "   dl_valid_labels=" <<
        dl_valid_labels;

        if (dl_valid_labels > 0)
        {
            ret = (dl_valid_labels >= (dl_total_disks >> 1)) ? true : false;
        }

        if (master == NULL)
        {
            fds_verify(dl_valid_labels == 0);
            fds_verify(upgrade.chain_empty_list() == false);
            fds_verify(dl_labels.chain_empty_list() == true);

            label = upgrade.chain_peek_front<DiskLabel>();
            label->dsk_label_generate(&upgrade, dl_total_disks);

            /* We need full list to generate the label.  Remove label out of the list. */
            chk = upgrade.chain_rm_front<DiskLabel>();
            fds_verify(chk == label);
        }else {
            label = master;
        }
        dl_mtx.unlock();

        if ((master == NULL) && (creat == true))
        {
            fds_verify(label != NULL);
            label->dsk_label_write(inv, this);

            valid_labels++;                                 // local
            master = label;
        }

        if (inv->dsk_need_simulation() == true)
        {
            LOGNORMAL << "In simulation, found " << dl_valid_labels << " labels";
        }else {
            LOGNORMAL << "Scan HW inventory, found " << dl_valid_labels << " labels";
        }

        for (; 1; valid_labels++)                          // local
        {
            curr = upgrade.chain_rm_front<DiskLabel>();

            if (curr == NULL)
            {
                GLOGDEBUG << "breaking";
                break;
            }

            if (creat == true)
            {
                GLOGDEBUG << "writing label" ;
                curr->dsk_label_clone(master);
                curr->dsk_label_write(inv, this);
            }
            delete curr;
        }

        dl_valid_labels += valid_labels;                    // rhs:local

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

            if (0 == strcmp(name, "/dev/sda"))
            {
                return;
            }

            *dl_map << disk->rs_get_name() << " " << idx << " " << std::hex <<
            disk->rs_get_uuid().uuid_get_val() << std::dec << " " <<
            disk->dsk_get_mount_point().c_str()  << "\n";
        }
    }
}  // namespace fds
