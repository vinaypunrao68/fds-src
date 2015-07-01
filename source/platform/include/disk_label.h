/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_DISK_LABEL_H_
#define SOURCE_PLATFORM_INCLUDE_DISK_LABEL_H_

#include <vector>

#include "fds_config.hpp"
#include "fds_process.h"

#include "platform_disk_obj.h"
#include "pm_disk_inventory.h"

namespace fds
{
    extern FdsProcess *g_fdsprocess;

    const int    DL_UUID_BYTE_LEN             = 30;
    const int    DL_MAJOR                     = 1;
    const int    DL_MINOR                     = 0;
    const int    DL_SECTOR_SZ                 = 512;
    const int    DL_SECTOR_BEGIN              = 64;
    const int    DL_PAGE_SZ                   = (8 << 10);
    const int    DL_PAGE_SECT_SZ              = (DL_PAGE_SZ / DL_SECTOR_SZ);
    const int    DL_INVAL_DISK_INDEX          = 0xffff;

    /**
     * On-disk format.  Keep this structure size at 128-byte.
     */
    typedef struct __attribute__((__packed__))
    {
        fds_uint32_t    dl_chksum;
        fds_uint32_t    dl_magic;
        fds_uint32_t    dl_sector_beg;
        fds_uint32_t    dl_sector_end;

        /* 16-byte offset. */
        fds_uint16_t    dl_major;
        fds_uint16_t    dl_minor;
        fds_uint16_t    dl_sect_sz;          /**< sector unit size in byte.     */
        fds_uint16_t    dl_total_sect;       /**< total lenght in sector size.  */
        fds_uint16_t    dl_used_sect;        /**< number sectors consumed.      */
        fds_uint16_t    dl_recovery_mode;    /**< value is dlabel_recovery_e.   */
        fds_uint16_t    dl_num_quorum;
        fds_uint16_t    dl_my_disk_index;

        /*  32-byte offset. */
        fds_uint32_t    dl_quorum_seq;
        fds_uint8_t     dl_disk_uuid[DL_UUID_BYTE_LEN];
        fds_uint8_t     dl_node_uuid[DL_UUID_BYTE_LEN];

        /* 96-byte offset */
        fds_uint8_t     dl_rsvd1[32];
    } dlabel_hdr_t;

    /**
     * On-disk enum to specify label types.  DO NOT change the order.  Once a number
     * is committed, it can't be reused//changed.
     */
    typedef enum
    {
        DL_NULL_TYPE           = 0x0,
        DL_NODE_DISKS          = 0x1,
        DL_TYPE_MAX
    } dlabel_type_e;

    /**
     * On-disk enum to specify super block update recovery type.
     */
    typedef enum
    {
        DL_RECOVERY_NONE        = 0,
        DL_RECOVERY_FORWARD     = 1,          /**< apply the record with highest seq num. */
        DL_RECOVERY_BACKWARD    = 2,          /**< apply the record with lowest seq num.  */
        DL_RECOVERY_QUORUM      = 3,          /**< valid record must meet quorum.         */
        DL_RECOVERY_MAX
    } dlabel_recovery_e;

    /**
     * On-disk format.  Keep this structure size at 32-byte.
     */
    typedef struct __attribute__((__packed__))
    {
        fds_uint16_t    dl_idx;
        fds_uint8_t     dl_uuid[DL_UUID_BYTE_LEN];
    } dlabel_uuid_t;

    /**
     * On-disk format.  Keep this structure size at 16-byte.
     */
    typedef struct __attribute__((__packed__))
    {
        fds_uint32_t    dl_chksum;
        fds_uint32_t    dl_magic;
        fds_uint16_t    dl_rec_type;          /**< value is dlabel_type_e.      */
        fds_uint16_t    dl_rec_cnt;           /**< number of entries.           */
        fds_uint16_t    dl_byte_len;          /**< size of this record in byte. */
        fds_uint16_t    dl_rsvd;
    } dlabel_rec_t;

    typedef struct __attribute__((__packed__))
    {
        dlabel_rec_t     dl_disk_rec;
        dlabel_uuid_t    dl_disk_uuids[0];
    } dlabel_disk_uuid_t;

    cc_assert(DL0, sizeof(dlabel_hdr_t) == 128);
    cc_assert(DL1, sizeof(dlabel_uuid_t) == 32);
    cc_assert(DL2, sizeof(dlabel_rec_t) == 16);

    /**
     * In-core format to compact disk label.
     */
    typedef struct
    {
        fds_uint16_t dl_disk_idx;
        ResourceUUID dl_disk_uuid;
    } dmem_disk_uuid_t;

    class DiskLabelMgr;
    typedef std::vector<dmem_disk_uuid_t>   DiskUuidArray;

    /**
     * -------------------------------------------------------------------------------------
     * Base disk label obj to perform read/write label to a disk.
     * -------------------------------------------------------------------------------------
     */

    class DiskLabel
    {
        protected:
            friend class DiskLabelMgr;

            ChainLink             dl_link;
            dlabel_hdr_t         *dl_label;
            dlabel_disk_uuid_t   *dl_disk_uuids;
            FdsConfigAccessor     m_conf;
            bool                  m_use_new_superblock;
            PmDiskObj::pointer    dl_owner;

            void dsk_label_fixup_header();

        public:
            explicit DiskLabel(PmDiskObj::pointer disk);
            virtual ~DiskLabel();

            int  dsk_label_sect_sz();
            int  dsk_fill_disk_uuids(ChainList *labels);
            bool dsk_label_valid(DiskLabelMgr *mgr = NULL);
            void dsk_label_init_header(dlabel_hdr_t *hdr);
            void dsk_label_init_uuids(int dsk_cnt);
            void dsk_label_comp_checksum(dlabel_hdr_t *hdr);
            void dsk_label_save_my_uuid(const ResourceUUID &uuid);
            ResourceUUID dsk_label_my_uuid();

            inline const dlabel_hdr_t       *dsk_label_hdr()
            {
                return dl_label;
            }

            inline const dlabel_disk_uuid_t *dsk_label_uuids()
            {
                return dl_disk_uuids;
            }

            virtual void dsk_label_generate(ChainList *labels, int dsk_cnt);
            virtual void dsk_label_clone(DiskLabel *master);
            virtual void dsk_label_read();
            virtual void dsk_label_write(PmDiskInventory::pointer inv, DiskLabelMgr *mgr);
    };
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_DISK_LABEL_H_
