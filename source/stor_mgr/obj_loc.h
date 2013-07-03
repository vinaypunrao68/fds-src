#include <fds_commons.h>
typedef enum {
FDS_DATA_LOC_TIER_SSD,
FDS_DATA_LOC_TIER_HD,
FDS_DATA_LOC_TIER_DRAM
} fds_data_tier_t;

#define FDS_MAX_VOL_IDS  32

typedef struct {
 fds_char_t     filename[64];
 fds_uint32_t  file_offset;
 fds_uint32_t  disk_num;
} fds_data_location_t;


typedef struct {
 fds_object_id_t     object_id;
 fds_data_tier_t     obj_tier;
 fds_data_location_t object_location;
 fds_uint32_t     vol_list[FDS_MAX_VOL_IDS]; /* Association entry: List of volumes pointing to this object */
} fds_object_location_entry_t;


fds_object_location_entry_t fds_object_loc[4096];
