/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <fds_uuid.h>
#include <disk-label.h>
#include <shared/fds-magic.h>

namespace fds {

// ------------------------------------------------------------------------------------
// Disk Label Formater
// ------------------------------------------------------------------------------------
DiskLabel::DiskLabel() : dl_label(NULL), dl_disk_uuids(NULL) {}
DiskLabel::~DiskLabel() {}

// dsk_label_init_header
// ---------------------
//
void
DiskLabel::dsk_label_init_header(dlabel_hdr_t *hdr)
{
    if (hdr == NULL) {
        hdr = dl_label;
    }
    fds_verify(hdr != NULL);
    hdr->dl_magic      = MAGIC_DSK_SUPPER_BLOCK;
    hdr->dl_sector_beg = 63;
    hdr->dl_sector_end = 100;   // just for now.
    hdr->dl_major      = DL_MAJOR;
    hdr->dl_minor      = DL_MINOR;
    hdr->dl_sect_sz    = DL_SECTOR_SZ;
    hdr->dl_total_sect = hdr->dl_sector_end - hdr->dl_sector_beg;
    hdr->dl_used_sect  = 1;
    hdr->dl_num_quorum = 0;

    hdr->dl_recovery_mode = DL_RECOVERY_NONE;
    hdr->dl_padding       = 0;
    hdr->dl_quorum_seq    = 0;
}

// dsk_label_generate
// ------------------
//
void
DiskLabel::dsk_label_generate(int dsk_cnt)
{
    size_t         size, dlen;
    fds_uint64_t  *uuid;
    dlabel_rec_t  *rec;
    dlabel_uuid_t *entry;


    fds_verify(dl_label == NULL);
    dlen = dsk_cnt * sizeof(dlabel_disk_uuid_t);
    size = sizeof(*dl_label) + sizeof(*dl_disk_uuids) + dlen;

    dl_label = reinterpret_cast<dlabel_hdr_t *>(malloc(size));
    memset(dl_label, 0, size);

    dl_disk_uuids    = reinterpret_cast<dlabel_disk_uuid_t *>(dl_label + 1);
    rec              = &dl_disk_uuids->dl_disk_rec;
    entry            = dl_disk_uuids->dl_disk_uuids;
    rec->dl_magic    = MAGIC_DSK_UUID_REC;
    rec->dl_rec_type = DL_NODE_DISKS;
    rec->dl_rec_cnt  = dsk_cnt;
    rec->dl_byte_len = dlen;

    for (int i = 0; i < dsk_cnt; i++) {
        entry->dl_idx = i;
        uuid          = reinterpret_cast<fds_uint64_t *>(entry->dl_uuid);
        *uuid         = fds_get_uuid64(get_uuid());
        entry++;
    }
    // Init and fixup the header with new fields.
    dsk_label_init_header(dl_label);
    dl_label->dl_num_quorum = dsk_cnt;
    dl_label->dl_used_sect  = FDS_ROUND_UP(size, dl_label->dl_sect_sz);

    fds_verify(dl_label->dl_used_sect < dl_label->dl_total_sect);
    dsk_label_comp_checksum(dl_label);
}

// dsk_label_comp_checksum
// -----------------------
//
void
DiskLabel::dsk_label_comp_checksum(dlabel_hdr_t *hdr)
{
    if (hdr == NULL) {
        hdr = dl_label;
    }
    fds_verify(hdr != NULL);
}

// dsk_label_read
// --------------
//
void
DiskLabel::dsk_label_read(DiskLabelMgr *mgr, PmDiskObj::pointer disk)
{
}

// dsk_label_write
// ---------------
//
void
DiskLabel::dsk_label_write(DiskLabelMgr *mgr, PmDiskObj::pointer disk)
{
}

// ------------------------------------------------------------------------------------
// Disk Label Op Router
// ------------------------------------------------------------------------------------
DiskLabelOp::DiskLabelOp(dlabel_op_e op, DiskLabelMgr *mgr)
    : dsk_op(op), dsk_mgr(mgr) {}

DiskLabelOp::~DiskLabelOp() {}

bool
DiskLabelOp::dsk_iter_fn(DiskObj::pointer curr)
{
    PmDiskObj::pointer disk;

    disk = PmDiskObj::dsk_cast_ptr(curr);
    switch (dsk_op) {
    case DISK_LABEL_READ:
        return true;

    case DISK_LABEL_WRITE:
        return true;

    default:
        break;
    }
    return false;
}

// ------------------------------------------------------------------------------------
// Disk Label Coordinator
// ------------------------------------------------------------------------------------
DiskLabelMgr::DiskLabelMgr() : dsk_mtx("label mtx") {}
DiskLabelMgr::~DiskLabelMgr() {}

bool
DiskLabelMgr::dsk_iter_fn(DiskObj::pointer curr)
{
    return false;
}

// dsk_iter_fn
// -----------
//
bool
DiskLabelMgr::dsk_iter_fn(DiskObj::pointer curr, DiskObjIter *arg)
{
    return false;
}

}  // namespace fds
