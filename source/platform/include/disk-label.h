/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_PLATFORM_INCLUDE_DISK_LABEL_H_
#define SOURCE_PLATFORM_INCLUDE_DISK_LABEL_H_

#include <vector>
#include <disk.h>
#include <fds_assert.h>

namespace fds {
const int DL_UUID_BYTE_LEN             = 30;

/**
 * On-disk format.  Keep this structure size at 128-byte.
 */
typedef struct __attribute__((__packed__))
{
    fds_uint32_t             dl_chksum;
    fds_uint32_t             dl_magic;
    fds_uint16_t             dl_sector_size;
    fds_uint16_t             dl_uuid_cnt;
    fds_uint16_t             dl_uuid_sector_beg;
    fds_uint16_t             dl_uuid_sector_end;

    /* 16-byte offset */
    fds_uint8_t              dl_disk_uuid[DL_UUID_BYTE_LEN];
    fds_uint8_t              dl_node_uuid[DL_UUID_BYTE_LEN];
    fds_uint8_t              dl_rsvd0[20];

    /* 96-byte offset */
    fds_uint8_t              dl_rsvd1[32];
} disk_label_hdr_t;

/**
 * On-disk format.  Keep this structure size at 32-byte.
 */
typedef struct __attribute__((__packed__))
{
    fds_uint16_t             dl_disk_idx;
    char                     dl_disk_uuid[DL_UUID_BYTE_LEN];
} disk_label_uuid_t;

cc_assert(DL0, sizeof(disk_label_hdr_t) == 128);
cc_assert(DL1, sizeof(disk_label_uuid_t) == 32);

/**
 * In-core format to compact disk label.
 */
typedef struct
{
    fds_uint16_t             dl_disk_idx;
    ResourceUUID             dl_disk_uuid;
} disk_uuid_rec_t;

typedef std::vector<disk_uuid_rec_t>   DiskUuidArray;

class DiskLabel
{
  public:
    explicit DiskLabel(DiskObj::pointer disk);
    virtual ~DiskLabel();

  protected:
    disk_label_hdr_t         dl_hdr;
    DiskUuidArray            dl_uuids;
};

}  // namespace fds
#endif  // SOURCE_PLATFORM_INCLUDE_DISK_LABEL_H_
