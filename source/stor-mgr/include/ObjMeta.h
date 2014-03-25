/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_OBJ_META_STORMGR_H_
#define SOURCE_OBJ_META_STORMGR_H_


#include <unistd.h>
#include <utility>
#include <atomic>
#include <unordered_map>

#include <leveldb/db.h>

#include <fdsp/FDSP_types.h>
#include "stor_mgr_err.h"
#include <fds_volume.h>
#include <fds_types.h>
#include <odb.h>

#include <assert.h>
#include <iostream>
#include <util/Log.h>
#include "StorMgrVolumes.h"
#include "SmObjDb.h"
#include <persistent_layer/dm_service.h>
#include <persistent_layer/dm_io.h>
#include <fds_migration.h>

#include <fds_qos.h>
#include <fds_assert.h>
#include <fds_config.hpp>
#include <fds_stopwatch.h>

#include <ObjStats.h>
#include <serialize.h>

namespace fds {

#define SYNCMETADATA_MASK 0x1

struct SyncMetaData {
    SyncMetaData();
    struct header_t {
        /* Born timestamp */
        uint64_t born_ts;
        /* Modification timestamp */
        uint64_t mod_ts;
        /* Association entry count */
        uint8_t assoc_entry_cnt;
    } *header;

    /* Association entries */
    obj_assoc_entry_t* assoc_entries;
};

/*
 * Persistent class for storing MetaObjMap, which
 * maps an object ID to its locations in the persistent
 * layer.
 */
class ObjMetaData : public serialize::Serializable {
private:
    char *persistBuffer;
    fds_uint32_t size;

    uint8_t *mask;
    meta_obj_map_t *obj_map; // Pointer to the meta_obj_map in the buffer
    obj_phy_loc_t *phy_loc;

    /* NOTE: We always start with atleast one association entry.  It's
     * possible that it's not populated
     */
    obj_assoc_entry_t *assoc_entry;

    SyncMetaData sync_data;

public:
    ObjMetaData();
    virtual ~ObjMetaData();


    void initialize(const ObjectID& objid, fds_uint32_t obj_size) {
        size = sizeof(uint8_t) + sizeof(meta_obj_map_t) + sizeof(obj_assoc_entry_t);
        persistBuffer = new char[size];
        memset(persistBuffer, 0, size);

        adjustPointers_(true);

        memcpy(&obj_map->obj_id.metaDigest, objid.GetId(), sizeof(obj_map->obj_id.metaDigest));
        obj_map->obj_size = obj_size;
        obj_map->obj_map_len = size;

        //Initialize the physical location array
        phy_loc = &obj_map->loc_map[0];
        phy_loc[diskio::flashTier].obj_tier = -1;
        phy_loc[diskio::diskTier].obj_tier = -1;
        phy_loc[3].obj_tier = -1;
    }

    ObjMetaData(const ObjectBuf& buf) {
        deserializeFrom(buf);
    }

    void unmarshall(const ObjectBuf& buf) { 
        deserializeFrom(buf);
    }

    char *marshall()  { 
        return persistBuffer;
    }

    virtual uint32_t write(serialize::Serializer* serializer) const override;

    virtual uint32_t read(serialize::Deserializer* deserializer) override;

    virtual uint32_t getEstimatedSize() const override;

    uint32_t serializeTo(ObjectBuf& buf) const;

    bool deserializeFrom(const ObjectBuf& buf);

    bool deserializeFrom(const leveldb::Slice& s);

    uint64_t getModificationTs() const;

    void extractSyncData(FDSP_MigrateObjectMetadata &md) const;

    void checkAndDemoteUnsyncedData(const uint64_t& syncTs);

    bool syncDataExists();

    void applySyncData(const FDSP_MigrateObjectMetadata& data);

    void mergeNewAndUnsyncedData();

    bool objectExists();

    fds_uint32_t   getObjSize() {
        return obj_map->obj_size;
    }
    obj_phy_loc_t*   getObjPhyLoc(diskio::DataTier tier) {
        return &phy_loc[tier];
    }
   meta_obj_map_t*   getObjMap() { 
       return obj_map;
   }
   fds_uint32_t   getMapSize() {
       return obj_map->obj_map_len;
   }
   fds_uint32_t   getObjSize() { 
       return obj_map->obj_size;
   }
   obj_phy_loc_t*   getObjPhyLoc(DataTier tier) { 
       return &phy_loc[tier];
   }
    void setRefCnt(fds_uint16_t refcnt) { 
        obj_map->obj_refcnt = refcnt;
    }

    void incRefCnt() { 
        obj_map->obj_refcnt++;
    }

    void decRefCnt() { 
        obj_map->obj_refcnt--;
    }

    // Association Tbl manipulation routines
    obj_assoc_entry_t * appendAssocEntry()
    {
        /* Create new buffer */
       fds_uint32_t new_size = size + sizeof(obj_assoc_entry_t);
       char *new_buf = new char[new_size];

       /* Increment obj_num_assoc_entry upfront to allow adjustPointers_()
        * to work
        */
       DBG(uint32_t prev_assoc_cnt = obj_map->obj_num_assoc_entry);
       obj_map->obj_num_assoc_entry++;

       /* copy existing data */
       if (syncDataExists()) {
           fds_assert(sync_data.header != nullptr);
           uint32_t copy_sz =  (char*)sync_data.header - persistBuffer;
           memcpy(new_buf, persistBuffer, copy_sz);
           memcpy(new_buf+copy_sz+size(obj_assoc_entry_t),
                   persistBuffer+copy_sz,
                   size-copy_sz);
       } else {
           memcpy(new_buf, persistBuffer, size);
       }
       delete persistBuffer;
       persistBuffer = new_buf;
       size = new_size;

       /* update pointers */
       adjustPointers_();

       obj_map->obj_map_len = size;

       fds_assert(prev_assoc_cnt+1 == obj_map->obj_num_assoc_entry);

       return &assoc_entry[obj_map->obj_num_assoc_entry - 1];
    }

    void updateAssocEntry(ObjectID objId, fds_volid_t vol_id) { 
        for(int i=0; i < obj_map->obj_num_assoc_entry; i++) { 
            if (vol_id == assoc_entry[i].vol_uuid) {
                assoc_entry[i].ref_cnt++;
                obj_map->obj_refcnt++;
                return;
            }
        }
        // Did not find any match vol_id's assoc_entry, append one
        if (obj_map->obj_refcnt == 0) { 
            assoc_entry[0].ref_cnt++;
            assoc_entry[0].vol_uuid = vol_id;
            obj_map->obj_refcnt++;
            obj_map->obj_num_assoc_entry = 1;
        } else {
            obj_assoc_entry_t *new_entry = appendAssocEntry();
            new_entry->vol_uuid = vol_id;
            new_entry->ref_cnt = 1;
            obj_map->obj_refcnt++;
        }
    }

    void deleteAssocEntry(ObjectID objId, fds_volid_t vol_id) { 
        for(int i=0; i < obj_map->obj_num_assoc_entry; i++) { 
            if (vol_id == assoc_entry[i].vol_uuid) {
                assoc_entry[i].ref_cnt--;
                obj_map->obj_refcnt--;
                return;
            }
        }
        // If Volume did not put this objId then it delete is a noop
    }

    // Tiering/Physical Location update routines
    fds_bool_t   onFlashTier() { 
        if (phy_loc[diskio::flashTier].obj_tier == diskio::flashTier) {
            return true;
        }
        return false;
    }  

    void 
    updatePhysLocation(obj_phy_loc_t *in_phy_loc) { 
        memcpy(&phy_loc[(fds_uint32_t)in_phy_loc->obj_tier], in_phy_loc, sizeof(obj_phy_loc_t));
    }

    void 
    removePhyLocation(diskio::DataTier tier) {
        phy_loc[tier].obj_tier = -1;
    }

private:
    bool syncDataExists();
    void adjustPointers_(bool init = false);

    friend std::ostream& operator<<(std::ostream& out, const ObjMetaData& objMap);
};

inline std::ostream& operator<<(std::ostream& out, const ObjMetaData& objMd)
{
     out << "Object MetaData: Version" << objMd.obj_map->obj_map_ver 
             << "   len : " << objMd.obj_map->obj_map_len
             << "  objId : " << objMd.obj_map->obj_id.metaDigest
             << "  obj_size " << objMd.obj_map->obj_size
             << "  obj_ref_cnt " << objMd.obj_map->obj_refcnt
             << "  num_assoc_entry " << objMd.obj_map->obj_num_assoc_entry
             << "  create_time " << objMd.obj_map->obj_create_time
             << "  del_time " << objMd.obj_map->obj_del_time
             << "  mod_time " << objMd.obj_map->assoc_mod_time
            << std::endl;
     for (fds_uint32_t i = 0; i < MAX_PHY_LOC_MAP; i++) {
         out << "Object MetaData: "
             << " Tier (" << objMd.phy_loc[i].obj_tier
             << ") loc id (" << objMd.phy_loc[i].obj_stor_loc_id
             << ") offset (" << objMd.phy_loc[i].obj_stor_offset << "), "
            << std::endl;
    }
    return out;
}

}  // namespace fds

#endif //  SOURCE_STOR_MGR_STORMGR_H_
