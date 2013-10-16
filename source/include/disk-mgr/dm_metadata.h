#ifndef INCLUDE_DISK_MGR_DM_METADATA_H_
#define INCLUDE_DISK_MGR_DM_METADATA_H_

#include <fds_assert.h>
#include <shared/fds_types.h>

c_decls_begin

/*
 * ----------------------------------------------------------------------------
 * On disk format for object ID.  DO NOT alter the structure layout.
 * ----------------------------------------------------------------------------
 */
typedef struct meta_obj_id         meta_obj_id_t;
struct __attribute__((__packed__)) meta_obj_id
{
    fds_uint64_t         oid_hash_hi;
    fds_uint64_t         oid_hash_lo;
};

/*
 * obj_id_set_inval
 * ----------------
 * Assign the object id with invalid value.
 */
static inline void
obj_id_set_inval(meta_obj_id_t *oid)
{
    oid->oid_hash_hi = oid->oid_hash_lo = 0;
}

/*
 * obj_id_is_valid
 * ---------------
 * Return true if the given oid has valid value.
 */
static inline bool
obj_id_is_valid(meta_obj_id_t *oid)
{
    return (oid->oid_hash_hi != 0) && (oid->oid_hash_lo != 0);
}

/*
 * ----------------------------------------------------------------------------
 * On disk format for fixed size volume address, DO NOT alter the layout.
 * ----------------------------------------------------------------------------
 */
typedef struct meta_vol_adr        meta_vol_adr_t;
struct __attribute__((__packed__)) meta_vol_adr
{
    fds_uint64_t         vol_uuid;
    fds_uint64_t         vol_blk_off;
};

#define VOL_INVAL_BLK_OFF              (0xffffffffffffffff)

/*
 * vadr_set_inval
 * --------------
 * Assgin the volume address struct with invalid value.
 */
static inline void
vadr_set_inval(meta_vol_adr_t *vadr)
{
    vadr->vol_uuid    = 0;
    vadr->vol_blk_off = VOL_INVAL_BLK_OFF;
}

/*
 * vadr_is_valid
 * -------------
 * Return true if the given vol addr has valid value.
 */
static inline bool
vadr_is_valid(meta_vol_adr_t *vadr)
{
    return (vadr->vol_uuid != 0) && (vadr->vol_blk_off != VOL_INVAL_BLK_OFF);
}

/*
 * ----------------------------------------------------------------------------
 * On disk format for variable size volume address.  DO NOT alter the layout.
 * ----------------------------------------------------------------------------
 */
typedef struct meta_vol_io         meta_vol_io_t;
struct __attribute__((__packed__)) meta_vol_io
{
    meta_vol_adr_t       vol_adr[0];
    fds_uint64_t         vol_uuid;
    fds_uint64_t         vol_blk_off;
    fds_uint16_t         vol_blk_len;         /* var blk io 512 to 32M.      */
    fds_uint16_t         vol_rsvd;
};

cc_assert(vio1, fds_offset_of(meta_vol_adr_t, vol_uuid) ==
    fds_offset_of(meta_vol_io_t, vol_uuid)
);

cc_assert(vio2, fds_offset_of(meta_vol_adr_t, vol_blk_off) ==
    fds_offset_of(meta_vol_io_t, vol_blk_off)
);

/*
 * ----------------------------------------------------------------------------
 * On disk format to map an object ID to physical location.  Append new fields
 * and update the version if changed.
 * ----------------------------------------------------------------------------
 */
struct __attribute__((__packed__)) meta_obj_map_v0
{
    fds_uint8_t          obj_map_ver;         /* current version.            */
    fds_uint8_t          obj_rsvd;
    fds_uint16_t         obj_blk_len;         /* var blk: 512 to 32M.        */
    fds_uint16_t         obj_io_func;         /* f(t) = io characteristic.   */
    fds_uint16_t         obj_rd_cnt;          /* read stat.                  */
    fds_uint16_t         obj_refcnt;          /* de-dupe refcnt.             */
    fds_uint16_t         obj_stor_loc_id;     /* map to physical location    */
    fds_uint32_t         obj_stor_offset;     /* offset to the physical loc. */
    meta_obj_id_t        obj_id;              /* check sum for data.         */
};

typedef struct meta_obj_map_v0     meta_obj_map_v0_t;
typedef struct meta_obj_map_v0     meta_obj_map_t;

#define OID_MAP_CURR_VER           (0)

/*
 * obj_map_init_v0
 * ---------------
 */
static inline void
obj_map_init_v0(meta_obj_map_v0_t *map)
{
    memset(map, 0, sizeof(*map));
}

/*
 * obj_map_init
 * ------------
 */
static inline void
obj_map_init(meta_obj_map_t *map)
{
    obj_map_init_v0((meta_obj_map_v0_t *)map);
}

/*
 * obj_map_has_init_val
 * --------------------
 */
static inline bool
obj_map_has_init_val(meta_obj_map_t *map)
{
    return obj_id_is_valid(&map->obj_id);
}

c_decls_end

#endif /* INCLUDE_DISK_MGR_DM_METADATA_H_ */
