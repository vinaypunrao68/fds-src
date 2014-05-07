/*
 * Copyright 2014 by Formation Data Sysems, Inc.
 */
#ifndef SOURCE_INCLUDE_SHARED_FDS_CONSTANTS_H_
#define SOURCE_INCLUDE_SHARED_FDS_CONSTANTS_H_

#include <shared/fds_types.h>

/*  This file is in the shared directory.  Keep it in C code. */
c_decls_begin

/**
 * Disk sector and block units.
 */
#define DISK_SECTOR_SHFT               (9)
#define DISK_SECTOR_BYTE               (1 << DISK_SECTOR_SHFT)
#define DISK_BLOCK_SHFT                (12)         /* 4K block */
#define DISk_BLOCK_BYTE                (1 << DISK_BLOCK_SHFT)

/**
 * Convert disk sector to bytes.
 */
static inline fds_uint32_t
fds_disk_sector_to_byte(fds_uint32_t sector)
{
    return sector << DISK_SECTOR_SHFT;
}

/**
 * Convert disk block to bytes.
 */
static inline fds_uint32_t
fds_disk_block_to_byte(fds_blk_t blk)
{
    return blk << DISK_BLOCK_SHFT;
}

/**
 * Disk inventory constants.
 * Alpha configuration requires min of 4 HDD >= 1000GB in capacity.
 */
#define DISK_ALPHA_COUNT_SSD           (2)
#define DISK_ALPHA_COUNT_HDD           (12)
#define DISK_ALPHA_COUNT_HDD_MIN       (4)
#define DISK_ALPHA_CAPACITY_GB         (1000)

/* Some max constants. */
#define FDS_MAX_FILE_NAME              (256)
#define MAX_SVC_NAME_LEN               (12)
#define MAX_DOMAIN_EP_SVC              (10000)

c_decls_end

#endif  /* SOURCE_INCLUDE_SHARED_FDS_CONSTANTS_H_  // NOLINT */
