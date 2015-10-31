/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_PERSISTENT_LAYER_DM_METADATA_H_
#define SOURCE_INCLUDE_PERSISTENT_LAYER_DM_METADATA_H_

#include <fds_assert.h>
#include <shared/fds_types.h>
#include <fds_types.h>

#include "EclipseWorkarounds.h"

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

/*
 *  Following bitmasks are use to indicate the status of a object.
 *  NOTE:  Needs to be 32bit, since obj_flags is defined as uint32_t.
 */
/* object is corrupted. detected and set by scavenger */
#define OBJ_FLAG_CORRUPTED          0x0001
/* object metadata require reconcile. metadata may exist, but
 * the user data object may not exist.  this flag is set during the
 * SM token migration where forwarded IO is a metadata operation without
 * the metadata and data not yet migrated to the destination SM.
 */
#define OBJ_FLAG_RECONCILE_REQUIRED  0x0002


// magic value for meta_obj_map.  mainly used to assert that data address is correct.
const fds_uint32_t meta_obj_map_magic_value = 0xdeadbeef;

// Current version of meta object map version.
// This represents the ondisk version of levelDB's <key,value>.
const fds_uint32_t meta_obj_map_version = 0U;

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
    fds_uint32_t         obj_map_ver;         /* current version.            */
    fds_uint32_t         obj_map_len;         /* UNUSED?????                 */
    meta_obj_id_t        obj_id;              /* check sum for data.         */
    fds_uint32_t         obj_flags;           /* flags / status */
    fds_uint8_t          compress_type;       /* Obj Compression type */
    fds_uint32_t         compress_len;        /* If compressed the obj compress length */
    fds_uint16_t         obj_blk_len;         /* var blk: 512 to 32M.        */
    fds_uint32_t         obj_size;            /* var, size in bytes */
    fds_uint64_t         obj_refcnt;          /* de-dupe refcnt.             */

    /* DLT version when the metadata was created (or modified) during the SM token
     * migration.
     * TODO(Sean):  Currently, When GC runs, if migration_dlt_ver <= currentDLT() AND
     *              have RECONCILE_REQUIRED flag still set, then there is an issue.
     *              It requires GC to run quorum reconciliation to pull a "valid"
     *              copy of the metadata and object from other nodes and resolve this
     *              issue.
     */
    fds_uint64_t         obj_migration_reconcile_dlt_ver;
    /* ref_cnt reconciliation during the token migration + active IO.
     * Note: this is a signed int64_t.  During the reconciliation, reconcile_ref_cnt
     *       can be negative, which needs to be reconciled -- RECONCILE_REQUIRED flag
     *       should be set, if reconcile_ref_cnt < 0.
     */
    fds_int64_t          obj_migration_reconcile_ref_cnt;

    /* Number association entries in the arr.
     * This should always match assoc_entry_map.size().
     */
    fds_uint32_t         obj_num_assoc_entry;

    fds_uint64_t         obj_create_time;     /* creation time.         */
    fds_uint64_t         obj_del_time;        /* deletion time.         */
    fds_uint64_t         obj_access_time;     /* last access time.      */
    fds_uint64_t         assoc_mod_time;      /* Modification time.         */
    fds_uint64_t         expire_time;         /* Object Expiration time */

    /* Object transition time to a archive tier */
    fds_uint8_t          delete_count;
    obj_phy_loc_t        loc_map[MAX_PHY_LOC_MAP];

    /* add padding to the data structure */
    char                 meta_obj_padding[31];
};

struct __attribute__((__packed__)) obj_assoc_entry_v0 {
    fds_int64_t         vol_uuid;   /* volume unique identifier */
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
