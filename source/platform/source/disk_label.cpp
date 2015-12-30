/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <fds_magic.h>
//#include "platform/platform.h"
#include "disk_label.h"
#include "disk_label_mgr.h"
#include <fds_module_provider.h>
#include <net/SvcMgr.h>
namespace fds
{
    // ------------------------------------------------------------------------------------
    // Disk Label Formater
    // ------------------------------------------------------------------------------------
    DiskLabel::DiskLabel(PmDiskObj::pointer owner) : dl_link(this), dl_label(NULL), dl_disk_uuids(NULL), dl_owner(owner), m_conf(g_fdsprocess->get_conf_helper()), m_use_new_superblock(false)
    {
        try
        {
            m_use_new_superblock = m_conf.get_abs<bool>("fds.feature_toggle.pm.use_new_superblock");
            LOGDEBUG << "Use new superblock location:  " << m_use_new_superblock;
        }
        catch (fds::Exception e)
        {
            LOGDEBUG << e.what();
        }
    }

    DiskLabel::~DiskLabel()
    {
        dl_owner = NULL;   // dec the refcnt

        if (dl_label != NULL)
        {
            delete [] reinterpret_cast<char *>(dl_label);
        }
    }

    // dsk_label_init_header
    // ---------------------
    //
    void DiskLabel::dsk_label_init_header(dlabel_hdr_t *hdr)
    {
        if (hdr == NULL)
        {
            hdr = dl_label;
        }

        fds_verify (hdr != NULL);
        hdr->dl_magic      = MAGIC_DSK_SUPER_BLOCK;

        // Feature toggle, the else clause is the old scenario
        if (m_use_new_superblock)
        {
            hdr->dl_sector_beg = 2;
        }
        else
        {
            hdr->dl_sector_beg = 64;
        }

        hdr->dl_sector_end = hdr->dl_sector_beg + DL_PAGE_SECT_SZ;
        hdr->dl_major      = DL_MAJOR;
        hdr->dl_minor      = DL_MINOR;
        hdr->dl_sect_sz    = DL_SECTOR_SZ;
        hdr->dl_total_sect = hdr->dl_sector_end - hdr->dl_sector_beg;

        hdr->dl_used_sect     = 1;
        hdr->dl_recovery_mode = DL_RECOVERY_NONE;
        hdr->dl_num_quorum    = 0;
        hdr->dl_my_disk_index = DL_INVAL_DISK_INDEX;
        hdr->dl_quorum_seq    = 0;

        fds_verify(dl_owner != NULL);
        // Platform::plf_get_my_node_uuid()->uuid_set_to_raw(hdr->dl_node_uuid);
        // Note: I really did not want to do this - prem
        *(reinterpret_cast<fds_uint64_t *>(hdr->dl_node_uuid)) = MODULEPROVIDER()->getSvcMgr()->getSelfSvcUuid().svc_uuid;

        dl_owner->rs_get_uuid().uuid_set_to_raw(hdr->dl_disk_uuid);
    }

    // dsk_label_valid
    // ---------------
    //
    bool DiskLabel::dsk_label_valid()
    {
        ResourceUUID    disk_uuid, node_uuid;

        // TODO(Vy): checksum stuffs.
        if ((dl_label == NULL) || (dl_label->dl_magic != MAGIC_DSK_SUPER_BLOCK))
        {
            return false;
        }
        disk_uuid.uuid_set_from_raw(dl_label->dl_disk_uuid);
        node_uuid.uuid_set_from_raw(dl_label->dl_node_uuid);

        if ((dl_disk_uuids->dl_disk_rec.dl_magic == MAGIC_DSK_UUID_REC) &&
            (disk_uuid.uuid_get_val() != 0) && (node_uuid.uuid_get_val() != 0))
        {
            return true;
        }
        return false;
    }

    // dsk_label_init_uuids
    // --------------------
    //
    void DiskLabel::dsk_label_init_uuids(int dsk_cnt)
    {
        dlabel_rec_t   *rec;

        rec              = &dl_disk_uuids->dl_disk_rec;
        rec->dl_magic    = MAGIC_DSK_UUID_REC;
        rec->dl_rec_type = DL_NODE_DISKS;
        rec->dl_rec_cnt  = dsk_cnt;
        rec->dl_byte_len = sizeof(*rec) + (dsk_cnt * sizeof(dlabel_uuid_t));
    }

    // dsk_label_my_uuid
    // -----------------
    //
    ResourceUUID DiskLabel::dsk_label_my_uuid()
    {
        fds_assert(dl_label != NULL);
        return ResourceUUID(dl_label->dl_disk_uuid);
    }

    // dsk_label_save_my_uuid
    // ----------------------
    //
    void DiskLabel::dsk_label_save_my_uuid(const ResourceUUID &uuid)
    {
        fds_assert(dl_label != NULL);
        uuid.uuid_set_to_raw(dl_label->dl_disk_uuid);
    }

    // dsk_label_comp_checksum
    // -----------------------
    //
    void DiskLabel::dsk_label_comp_checksum(dlabel_hdr_t *hdr)
    {
        if (hdr == NULL)
        {
            hdr = dl_label;
        }
        fds_verify(hdr != NULL);
    }

    // dsk_label_sect_sz
    // -----------------
    //
    int DiskLabel::dsk_label_sect_sz()
    {
        int    sect_used, sect_total;

        fds_verify(dl_label != NULL);

        sect_used  = dl_label->dl_used_sect;
        sect_total = dl_label->dl_sector_end - dl_label->dl_sector_beg;

        fds_verify(sect_total == dl_label->dl_total_sect);
        fds_verify(sect_used <= dl_label->dl_total_sect);
        fds_verify(sect_used <= sect_total);
        return sect_used;
    }

    // dsk_label_generate
    // ------------------
    //
    void DiskLabel::dsk_label_generate(ChainList *labels, int dsk_cnt)
    {
        int       cnt;
        size_t    size;

        size = DL_PAGE_SZ;

        if (dl_label == NULL)
        {
            dl_label      = reinterpret_cast<dlabel_hdr_t *>(new char [size]);
            dl_disk_uuids = reinterpret_cast<dlabel_disk_uuid_t *>(dl_label + 1);
        }else {
            fds_verify(dl_disk_uuids == reinterpret_cast<dlabel_disk_uuid_t *>(dl_label + 1));
        }
        fds_verify(static_cast<int>(size) <= DL_PAGE_SZ);
        memset(dl_label, 0, size);

        // Init and fixup the header with new fields.
        dsk_label_init_header(dl_label);

        cnt = dsk_fill_disk_uuids(labels);
        fds_verify(cnt == dsk_cnt);

        dsk_label_fixup_header();
        dsk_label_comp_checksum(dl_label);
        fds_verify(dl_label->dl_used_sect <= dl_label->dl_total_sect);
    }

    // dsk_label_clone
    // ---------------
    // Clone the label from the master.  Fix up its own header with uuid and compute all
    // checksums.
    //
    void DiskLabel::dsk_label_clone(DiskLabel *master)
    {
        dlabel_hdr_t   *src;

        fds_verify(ResourceUUID(dl_label->dl_disk_uuid) == dl_owner->rs_get_uuid());

        // Copy from the master label.
        src = master->dl_label;
        memcpy(dl_label, src, src->dl_used_sect * src->dl_sect_sz);

        // Restore back my disk uuid and fix up the index.
        dl_owner->rs_get_uuid().uuid_set_to_raw(dl_label->dl_disk_uuid);
        dsk_label_fixup_header();
    }

    // dsk_label_fixup_header
    // ----------------------
    //
    void DiskLabel::dsk_label_fixup_header()
    {
        int              i, cnt;
        ResourceUUID     uuid, cmp;
        dlabel_uuid_t   *rec;

        rec = dl_disk_uuids->dl_disk_uuids;
        cnt = dl_disk_uuids->dl_disk_rec.dl_rec_cnt;

        dl_label->dl_num_quorum = cnt;
        dl_label->dl_used_sect  = FDS_ROUND_UP(DL_PAGE_SZ, dl_label->dl_sect_sz);
        uuid.uuid_set_from_raw(dl_label->dl_disk_uuid);

        for (i = 0; i < cnt; i++, rec++)
        {
            cmp.uuid_set_from_raw(rec->dl_uuid);

            if (cmp == uuid)
            {
                dl_label->dl_my_disk_index = i;
                return;
            }
        }
        fds_panic("Corrupted super block");
    }

    // dsk_fill_disk_uuids
    // -------------------
    //
    int DiskLabel::dsk_fill_disk_uuids(ChainList *labels)
    {
        int              dsk_cnt;
        ChainIter        iter;
        DiskLabel       *curr;
        dlabel_uuid_t   *entry;

        dsk_cnt = 0;
        entry   = dl_disk_uuids->dl_disk_uuids;

        chain_foreach(labels, iter) {
            curr = labels->chain_iter_current<DiskLabel>(iter);

            entry->dl_idx = dsk_cnt;
            memcpy(entry->dl_uuid, curr->dl_label->dl_disk_uuid, DL_UUID_BYTE_LEN);

            // Do consistency check with the disk object.
            fds_verify(ResourceUUID(entry->dl_uuid) == curr->dl_owner->rs_get_uuid());
            dsk_cnt++;
            entry++;
        }
        dsk_label_init_uuids(dsk_cnt);
        return dsk_cnt;
    }

    // dsk_label_read
    // --------------
    //
    void DiskLabel::dsk_label_read()
    {
        void   *buf;

        fds_assert(dl_label == NULL);
        buf = static_cast<void *>(new char [DL_PAGE_SZ]);

        if (dl_owner->dsk_read(buf, DL_SECTOR_BEGIN, DL_PAGE_SECT_SZ, m_use_new_superblock) <= 0)
        {
            memset(buf, 0, DL_PAGE_SZ);
        }
        dl_label      = reinterpret_cast<dlabel_hdr_t *>(buf);
        dl_disk_uuids = reinterpret_cast<dlabel_disk_uuid_t *>(dl_label + 1);

        if (dsk_label_valid() == false)
        {
            memset(buf, 0, DL_PAGE_SZ);
        }
    }

    // dsk_label_write
    // ---------------
    //
    bool DiskLabel::dsk_label_write(PmDiskInventory::pointer inv)
    {
        int    sect_sz;
        ssize_t ret;

        sect_sz = dsk_label_sect_sz();
        fds_verify(dl_label != NULL);
        fds_verify(sect_sz <= DL_PAGE_SECT_SZ);

        ret = dl_owner->dsk_write(inv->dsk_need_simulation(), reinterpret_cast<void *>(dl_label), DL_SECTOR_BEGIN, sect_sz, m_use_new_superblock);
        if (inv->dsk_need_simulation())
        { // if we are in simulation mode dsk_write is not going to write a label, so ignore its return value
            return true;
        }
        return ret;
    }
}  // namespace fds
