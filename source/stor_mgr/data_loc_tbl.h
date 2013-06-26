typedef enum {
FDS_DATA_LOC_TIER_SSD,
FDS_DATA_LOC_TIER_HD,
FDS_DATA_LOC_TIER_DRAM
} fds_data_tier_t;


typedef struct {
 fds_data_hash_t     object_id;
 fds_data_tier_t     obj_tier;
 fds_data_location_t object_location;
} fds_object_location_entry_t;

typedef struct {
 fds_char      filename;
 fds_uint32_t  file_offset;
} fds_data_location_t;

fds_object_location_entry_t fds_object_loc[4096];
