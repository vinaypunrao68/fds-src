/*
 * Copyright 2014 by Formation Data, Inc.
 */

#include "util/Log.h"
#include "disk_capability.h"
#include "disk_constants.h"

namespace fds
{

// This is defined in disk_format.py
static constexpr std::uint8_t DISK_MARKER[]  = { 'F',  'o',  'r',  'm',  'a',  't',  'i',  'o',
                                                 'n', '\0',  'D',  'a',  't',  'a', '\0',  'S',
                                                 'y',  's',  't',  'e',  'm',  's', '\0', 0xFD,
                                                '\0',  'D',  'I',  'S',  'K', '\0' };
static constexpr char DISK_TYPE_SDD = 'S';
static constexpr char DISK_TYPE_HDD = 'H';
static constexpr char INTERFACE_TYPE_SATA   = 'T';
static constexpr char INTERFACE_TYPE_SAS    = 'S';

DiskCapability::DiskCapability(PmDiskObj::pointer owner) : dc_link(this), dc_owner(owner)
{
}

void
DiskCapability::dsk_capability_read() {
    uint8_t* buf = new uint8_t[fds_disk_sector_to_byte(1)];

    if (0 < dc_owner->dsk_read(buf, 0, 1, true)) {
        if (0 == memcmp(DISK_MARKER, buf, sizeof(DISK_MARKER))) {
            LOGNORMAL << "Found FDS disk [" << dc_owner->dsk_get_mount_point()
                      << "] type: " << buf[30] << buf[31];
            initialized = true;
            if (DISK_TYPE_HDD == buf[30])
              { spinning = true; }

            if (INTERFACE_TYPE_SAS == buf[31])
              { disk_type = FDS_DISK_SAS; }
        }
    }

    delete buf;
}

}  // namespace fds
