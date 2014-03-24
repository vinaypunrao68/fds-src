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
/*
 * Persistent class for storing MetaObjMap, which
 * maps an object ID to its locations in the persistent
 * layer.
 */
class ObjMetaData : public serialize::Serializable {
private:
    char *persistBuffer;
    fds_uint32_t size;
    meta_obj_map_t *obj_map; // Pointer to the meta_obj_map in the buffer
    obj_phy_loc_t *phy_loc;
    obj_assoc_entry_t *assoc_entry;
    std::vector<obj_assoc_entry_t> pods;

public:
    ObjMetaData();
    virtual ~ObjMetaData();
    void initialize(const ObjectID& objid, fds_uint32_t obj_size) {
        size = sizeof(meta_obj_map_t) + sizeof(obj_assoc_entry_t);
        persistBuffer = new char[size];
        memset(persistBuffer, 0, size);
        obj_map = (meta_obj_map_t *)persistBuffer;
        memcpy(&obj_map->obj_id.metaDigest, objid.GetId(), sizeof(obj_map->obj_id.metaDigest));
        obj_map->obj_size = obj_size;

        //Initialize the physical location array
        phy_loc = &obj_map->loc_map[0];
        phy_loc[diskio::flashTier].obj_tier = -1;
        phy_loc[diskio::diskTier].obj_tier = -1;
        phy_loc[3].obj_tier = -1;

        // Association entry setup
        assoc_entry = (obj_assoc_entry_t *)(persistBuffer + sizeof(meta_obj_map_t ));

    }
    ObjMetaData(const ObjectBuf& buf) {
        persistBuffer = new char[buf.data.length()];
        size = buf.data.length();
        memcpy(persistBuffer, buf.data.c_str(), size);
        obj_map = (meta_obj_map_t *)persistBuffer;
        phy_loc = &obj_map->loc_map[0];
        assoc_entry = (obj_assoc_entry_t *)(persistBuffer + sizeof(meta_obj_map_t ));
    }
    void unmarshall(const ObjectBuf& buf) { 
        persistBuffer = new char[buf.data.length()];
        size = buf.data.length();
        memcpy(persistBuffer, buf.data.c_str(), size);
        obj_map = (meta_obj_map_t *)persistBuffer;
        phy_loc = &obj_map->loc_map[0];
        assoc_entry = (obj_assoc_entry_t *)(persistBuffer + sizeof(meta_obj_map_t ));
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

    bool syncMetadataExists();

    meta_obj_map_t*   getObjMap() {
        return obj_map;
    }
    fds_uint32_t   getObjSize() {
        return obj_map->obj_size;
    }
    obj_phy_loc_t*   getObjPhyLoc(diskio::DataTier tier) {
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
        fds_uint32_t new_size = size + sizeof(obj_assoc_entry_t);
        char *new_buf = new char[new_size];
        memcpy(new_buf, persistBuffer, size);
        delete persistBuffer;
        persistBuffer = new_buf;
        obj_map = (meta_obj_map_t *)persistBuffer;
        obj_map->obj_num_assoc_entry++;
        phy_loc = &obj_map->loc_map[0];
        assoc_entry = (obj_assoc_entry_t *)(persistBuffer + sizeof(meta_obj_map_t ));
        return &assoc_entry[obj_map->obj_num_assoc_entry - 1];
    }

    void updateAssocEntry(obj_assoc_entry_t *assocent) { 
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

    friend std::ostream& operator<<(std::ostream& out, const ObjMetaData& objMap);
};

inline std::ostream& operator<<(std::ostream& out, const ObjMetaData& objMd)
{
    out << "Object MetaData: Version" << objMd.obj_map->obj_map_ver
            << " len : " << objMd.obj_map->obj_map_len
            << "objId : " << objMd.obj_map->obj_id.metaDigest
            << "obj_size " << objMd.obj_map->obj_size
            << "obj_ref_cnt " << objMd.obj_map->obj_refcnt
            << "num_assoc_entry " << objMd.obj_map->obj_num_assoc_entry
            << "create_time " << objMd.obj_map->obj_create_time
            << "del_time " << objMd.obj_map->obj_del_time
            << "mod_time " << objMd.obj_map->assoc_mod_time
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
