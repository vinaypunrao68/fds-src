/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_DISK_CONSTANTS_H_
#define SOURCE_PLATFORM_INCLUDE_DISK_CONSTANTS_H_

#include <stdint.h>
#include <cstddef>

/**
 * -----------------------------------------------------------------------------------
 * Disk sector and block units.
 * -----------------------------------------------------------------------------------
 */
const uint32_t DISK_SECTOR_SHIFT = 9;
// unsed  const uint32_t DISK_SECTOR_BYTE = 1 << DISK_SECTOR_SHIFT;

const uint32_t DISK_BLOCK_SHIFT = 12;
// unsed  const uint32_t DISK_BLOCK_BYTE = 1 << DISK_BLOCK_SHIFT;

/**
 * Convert disk sector to bytes.
 */
static inline std::size_t fds_disk_sector_to_byte(std::size_t sector)
{
    return sector << DISK_SECTOR_SHIFT;
}

/* Unused, fix return type if implemented
 * Convert disk block to bytes.
   static inline fds_uint32_t fds_disk_block_to_byte(fds_blk_t blk)
   {
    return blk << DISK_BLOCK_SHFT;
   }
 */

/**
 * -----------------------------------------------------------------------------------
 * Disk inventory constants.
 * -----------------------------------------------------------------------------------
 */
const uint32_t DISK_ALPHA_COUNT_SSD     = 1;
const uint32_t DISK_ALPHA_COUNT_HDD     = 12;
const uint32_t DISK_ALPHA_COUNT_HDD_MIN = 1;

constexpr uint32_t DISK_MINIMUM_CAPACITY_GB   = 10;

#endif  // SOURCE_PLATFORM_INCLUDE_DISK_CONSTANTS_H_
