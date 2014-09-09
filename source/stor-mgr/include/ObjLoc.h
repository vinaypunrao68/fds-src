/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_OBJLOC_H_
#define SOURCE_STOR_MGR_INCLUDE_OBJLOC_H_

#include "fds_types.h"

typedef enum {
    FDS_DATA_LOC_TIER_SSD,
    FDS_DATA_LOC_TIER_HD,
    FDS_DATA_LOC_TIER_NVRAM,
    FDS_DATA_LOC_TIER_DRAM
} fds_data_tier_t;

#define FDS_MAX_VOL_IDS  32

struct FDS_DataLocEntry {
    fds_char_t      shard_file_name[64];
    fds_long_t      shard_file_offset;  // This is the present offset at which append should start
    fds_uint32_t    disk_num;
};

class FDS_VolumeIndexEntry {
    fds::fds_volid_t    volId;
    double              volOffset;
};

struct FDSObjLocEntryType {
    fds::ObjectID               object_id;
    fds_uint32_t                obj_state;        // 0: deleted | needs GC,      >0: num volume refs
    fds_uint32_t                obj_cache_state;  // 0: not in the object cache, >0: in DRAM cache
    fds_data_tier_t             obj_tier;
    FDS_DataLocEntry            object_location;
    FDS_VolumeIndexEntry        vollist[FDS_MAX_VOL_IDS];  // Volumes pointing to this object
};

#endif  // SOURCE_STOR_MGR_INCLUDE_OBJLOC_H_
