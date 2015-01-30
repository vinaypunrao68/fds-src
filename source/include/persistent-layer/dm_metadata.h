/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_PERSISTENT_LAYER_DM_METADATA_H_
#define SOURCE_INCLUDE_PERSISTENT_LAYER_DM_METADATA_H_

#include <fds_assert.h>
#include <shared/fds_types.h>
#include <fds_types.h>

/*
 * ----------------------------------------------------------------------------
 * On disk format for object ID.  DO NOT alter the structure layout.
 * ----------------------------------------------------------------------------
 */
typedef struct meta_obj_id         meta_obj_id_t;
struct __attribute__((__packed__)) meta_obj_id
{
    // looks like  we have Obj Structure scattered  across. sizing the array is a issue??.
    alignas(8) uint8_t metaDigest[20];
};

/*
 * obj_id_set_inval
 * ----------------
 * Assign the object id with invalid value.
 */
static inline void
obj_id_set_inval(meta_obj_id_t *oid)
{
    memset(oid->metaDigest, 0 , sizeof(oid));
}

/* TODO(sean)
 * The size of the buffer is hard-coded.  This was there before. But there should be a common
 * define that is used by all buffer that's sized as ObjectId.
 */
static uint8_t InvalidObjId[20];  // global is zero-filled.

/*
 * obj_id_is_valid
 * ---------------
 * Return true if the given oid has valid value.
 */
static inline bool
obj_id_is_valid(meta_obj_id_t const *const oid)
{
    // size of the invalid object and metaDigest should match.
    static_assert((sizeof(InvalidObjId) == sizeof(oid->metaDigest)),
                  "size of ObjId doesn't match oid");
    return (!(memcmp((const char *)oid->metaDigest, InvalidObjId, sizeof(oid)) == 0 ));
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
vadr_is_valid(meta_vol_adr_t const *const vadr)
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
    fds_int64_t         vol_uuid;
    fds_uint16_t         vol_rsvd;
};

cc_assert(vio1, fds_offset_of(meta_vol_adr_t, vol_uuid) ==
    fds_offset_of(meta_vol_io_t, vol_uuid)
);

/*
 * ----------------------------------------------------------------------------
 * On disk format to map an object ID to physical location.  Append new fields
 * and update the version if changed.
 *    meta_obj_map_v0
 *    obj_phys_loc1
 *    obj_phys_loc2
 *    obj_phys_loc3
 *    assoc_entry1
 *    assoc_entry2
 *    assoc_entry3
 * ----------------------------------------------------------------------------
 */
#define MAX_PHY_LOC_MAP 3
#define MAX_ASSOC_ENTRY 64
#define SYNCMETADATA_MASK    0x1
#define OBJ_FLAG_CORRUPTED   0x40
#define META_OBJ_MAP_MAGIC_VALUE    0xdeadbeef // magic value for meta_obj_map
struct __attribute__((__packed__)) obj_phy_loc_v0 {
    fds_int8_t           obj_tier;            /* tier location               */
    fds_uint16_t         obj_stor_loc_id;     /* physical location in tier   */
    fds_uint16_t         obj_file_id;         /* FileId of the tokenFile */
    fds_blk_t            obj_stor_offset;     /* offset to the physical loc. */
};
typedef struct meta_obj_map_v0     meta_obj_map_t;
typedef struct obj_phy_loc_v0     obj_phy_loc_t;
typedef struct obj_assoc_entry_v0     obj_assoc_entry_t;
typedef obj_phy_loc_t     ObjPhyLoc;

struct __attribute__((__packed__)) meta_obj_map_v0
{
    fds_uint32_t         obj_magic;           /* magic value for object */
    fds_uint8_t          obj_map_ver;         /* current version.            */
    fds_uint32_t         obj_map_len;         /* UNUSED?????                 */
    fds_uint8_t          obj_rsvd;            /* NO NEED FOR THIS????        */
    fds_uint8_t          compress_type;       /* Obj Compression type */
    fds_uint32_t         compress_len;        /* If compressed the obj compress length */
    meta_obj_id_t        obj_id;              /* check sum for data.         */
    fds_uint16_t         obj_blk_len;         /* var blk: 512 to 32M.        */
    fds_uint32_t         obj_size;            /* var, size in bytes */
    fds_uint64_t         obj_refcnt;          /* de-dupe refcnt.             */

    /* version of sm token migration for reconcile_ref_cnt.  This version
     * determines whether the reconcile_ref_cnt is current or stale
     */
    fds_uint64_t         migration_ver;
    /* ref_cnt reconciliation during the token migration + active IO */
    fds_int64_t          migration_reconcile_ref_cnt;

    fds_uint16_t         obj_num_assoc_entry; /* Number association entries in the arr.*/
    fds_uint64_t         obj_create_time;     /* creation time.         */
    fds_uint64_t         obj_del_time;        /* deletion time.         */
    fds_uint64_t         assoc_mod_time;      /* Modification time.         */
    fds_uint64_t         expire_time;         /* Object Expiration time */
    fds_uint8_t          obj_flags;            /* flags / status */

    /* Object transition time to a archive tier */
    fds_uint64_t         transition_time;
    obj_phy_loc_t        loc_map[MAX_PHY_LOC_MAP];

    /* add padding to the data structure */
    char                 meta_obj_padding[32];
};

struct __attribute__((__packed__)) obj_assoc_entry_v0 {
    fds::fds_volid_t    vol_uuid;   /* volume unique identifier */
    fds_uint64_t        ref_cnt;    /* ref_cnt association to this volume */
    /* ref_cnt reconciliation during the token migration.
     * see meta_obj_map_t's migration_ver to determine
     * if the version is current or not.
     */
    fds_int64_t         vol_migration_reconcile_ref_cnt;
};

/*
 * The below is an attempt to make the persistent layer
 * organization a bit more c++-y/object-oriented.
 * We can evaluate if it's good/stupid/whatever...
 */

#define OID_MAP_CURR_VER           (0)

/*
 * obj_map_init_v0
 * ---------------
 */
static inline void
obj_map_init_v0(meta_obj_map_v0 *map)
{
    memset(map, 0, sizeof(*map));
    map->loc_map[0].obj_tier = -1;
    map->loc_map[1].obj_tier = -1;
    map->loc_map[2].obj_tier = -1;
}

/*
 * obj_map_init
 * ------------
 */
static inline void
obj_map_init(meta_obj_map_t *map)
{
    obj_map_init_v0(reinterpret_cast<meta_obj_map_v0 *>(map));
}

/*
 * obj_map_has_init_val
 * --------------------
 */
static inline bool
obj_map_has_init_val(meta_obj_map_t const *const map)
{
    return obj_id_is_valid(&map->obj_id);
}

#endif  // SOURCE_INCLUDE_PERSISTENT_LAYER_DM_METADATA_H_
