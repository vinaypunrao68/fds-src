/*
 * Copyright 2014 by Formation Data Sysems, Inc.
 */
#ifndef SOURCE_INCLUDE_SHARED_FDS_CONSTANTS_H_
#define SOURCE_INCLUDE_SHARED_FDS_CONSTANTS_H_

#include <shared/fds_types.h>

/*  This file is in the shared directory.  Keep it in C code. */
c_decls_begin

/* Disk sector and block units. */
#define DISK_SECTOR_SHFT               (9)
#define DISK_SECTOR_BYTE               (1 << DISK_SECTOR_SHFT)
#define DISK_BLOCK_SHFT                (12)         /* 4K block */
#define DISk_BLOCK_BYTE                (1 << DISK_BLOCK_SHFT)

static inline fds_uint32_t
fds_disk_sector_to_byte(fds_uint32_t sector)
{
    return sector << DISK_SECTOR_SHFT;
}

static inline fds_uint32_t
fds_disk_block_to_byte(fds_blk_t blk)
{
    return blk << DISK_BLOCK_SHFT;
}

/* Disk inventory constants. */
#define DISK_ALPHA_COUNT_SSD           (2)
#define DISK_ALPHA_COUNT_HDD           (12)

/* Some max constants. */
#define FDS_MAX_FILE_NAME              (256)

c_decls_end

#endif  /* SOURCE_INCLUDE_SHARED_FDS_CONSTANTS_H_  // NOLINT */
