/*
 * Copyright 2014 by Formation Data Sysems, Inc.
 */
#ifndef SOURCE_INCLUDE_SHARED_FDS_CONSTANTS_H_
#define SOURCE_INCLUDE_SHARED_FDS_CONSTANTS_H_

#include <shared/fds_types.h>

/*  This file is in the shared directory.  Keep it in C code. */
c_decls_begin

/**
 * -----------------------------------------------------------------------------------
 * Disk sector and block units.
 * -----------------------------------------------------------------------------------
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
 * -----------------------------------------------------------------------------------
 * Disk inventory constants.
 * Alpha configuration requires min of 4 HDD >= 1000GB in capacity.
 * -----------------------------------------------------------------------------------
 */
#define DISK_ALPHA_COUNT_SSD           (2)
#define DISK_ALPHA_COUNT_HDD           (12)
#define DISK_ALPHA_COUNT_HDD_MIN       (4)
#define DISK_ALPHA_CAPACITY_GB         (500)

/**
 * -----------------------------------------------------------------------------------
 * Some max constants.
 * -----------------------------------------------------------------------------------
 */
#define FDS_MAX_FILE_NAME              (256)
#define FDS_MAX_IP_STR                 (32)
#define FDS_MAX_UUID_STR               (32)
#define FDS_MAX_NODE_NAME              (12)
#define MAX_SVC_NAME_LEN               (12)
#define MAX_DOMAIN_EP_SVC              (10240)

/**
 * -----------------------------------------------------------------------------------
 * System platform constants.
 * -----------------------------------------------------------------------------------
 */
#define FDS_MAX_CPUS                   (32)
#define FDS_MAX_DISKS_PER_NODE         (1024)
#define FDS_MAX_AM_NODES               (10000)
#define MAX_DOMAIN_NODES               (1024)

#define FDS_MAX_DLT_ENTRIES            (65536)
#define FDS_MAX_DLT_DEPTH              (4)
#define FDS_MAX_DLT_BYTES              (522 * 1024)
// current default DMT size
#define FDS_MAX_DMT_BYTES              (16 * 4 * 8 + 20)

c_decls_end

#endif  /* SOURCE_INCLUDE_SHARED_FDS_CONSTANTS_H_  // NOLINT */
