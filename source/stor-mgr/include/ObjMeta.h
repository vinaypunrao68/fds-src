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

struct SyncMetaData : public serialize::Serializable{
    SyncMetaData();
    void reset();

    /* Overrides from Serializable */
    virtual uint32_t write(serialize::Serializer* serializer) const override;
    virtual uint32_t read(serialize::Deserializer* deserializer) override;
    virtual uint32_t getEstimatedSize() const override;

    /* Born timestamp */
    uint64_t born_ts;
    /* Modification timestamp */
    uint64_t mod_ts;
    /* Association entries */
    std::vector<obj_assoc_entry_t> assoc_entries;
};

/*
 * Persistent class for storing MetaObjMap, which
 * maps an object ID to its locations in the persistent
 * layer.
 */
class ObjMetaData : public serialize::Serializable {
private:
    uint8_t mask;
    meta_obj_map_t obj_map; // Pointer to the meta_obj_map in the buffer
    obj_phy_loc_t *phy_loc;
    std::vector<obj_assoc_entry_t> assoc_entry;

    SyncMetaData sync_data;

public:
    ObjMetaData();
    virtual ~ObjMetaData();


    void initialize(const ObjectID& objid, fds_uint32_t obj_size) {
        memcpy(&obj_map.obj_id.metaDigest, objid.GetId(), sizeof(obj_map.obj_id.metaDigest));
        obj_map.obj_size = obj_size;

        //Initialize the physical location array
        phy_loc = &obj_map.loc_map[0];
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

    virtual uint32_t write(serialize::Serializer* serializer) const override;

    virtual uint32_t read(serialize::Deserializer* deserializer) override;

    virtual uint32_t getEstimatedSize() const override;

    uint32_t serializeTo(ObjectBuf& buf) const;

    bool deserializeFrom(const ObjectBuf& buf);

    bool deserializeFrom(const leveldb::Slice& s);

    uint64_t getModificationTs() const;

    void apply(const FDSP_MigrateObjectMetadata& data);

    void extractSyncData(FDSP_MigrateObjectMetadata &md) const;

    void checkAndDemoteUnsyncedData(const uint64_t& syncTs);

    bool syncDataExists() const;

    void applySyncData(const FDSP_MigrateObjectMetadata& data);

    void mergeNewAndUnsyncedData();

    bool dataPhysicallyExists();

    fds_uint32_t   getObjSize() const
    {
        return obj_map.obj_size;
    }
    obj_phy_loc_t*   getObjPhyLoc(diskio::DataTier tier) {
        return &phy_loc[tier];
    }
    meta_obj_map_t*   getObjMap() {
        return &obj_map;
    }

    void setRefCnt(fds_uint16_t refcnt) { 
        obj_map.obj_refcnt = refcnt;
    }

    void incRefCnt() { 
        obj_map.obj_refcnt++;
    }

    void decRefCnt() { 
        obj_map.obj_refcnt--;
    }

    void updateAssocEntry(ObjectID objId, fds_volid_t vol_id) { 
        fds_assert(obj_map.obj_num_assoc_entry == assoc_entry.size());
        for(int i = 0; i < obj_map.obj_num_assoc_entry; i++) {
            if (vol_id == assoc_entry[i].vol_uuid) {
                assoc_entry[i].ref_cnt++;
                obj_map.obj_refcnt++;
                return;
            }
        }
        obj_assoc_entry_t new_association;
        new_association.vol_uuid = vol_id;
        new_association.ref_cnt = 1;
        obj_map.obj_refcnt++;
        assoc_entry.push_back(new_association);
        obj_map.obj_num_assoc_entry = assoc_entry.size();

    }

    void deleteAssocEntry(ObjectID objId, fds_volid_t vol_id) { 
        fds_assert(obj_map.obj_num_assoc_entry == assoc_entry.size());
        for(int i = 0; i < obj_map.obj_num_assoc_entry; i++) {
            if (vol_id == assoc_entry[i].vol_uuid) {
                assoc_entry[i].ref_cnt--;
                obj_map.obj_refcnt--;
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
    void mergeAssociationArrays_();

    friend std::ostream& operator<<(std::ostream& out, const ObjMetaData& objMap);
};

inline std::ostream& operator<<(std::ostream& out, const ObjMetaData& objMd)
{
     out << "Object MetaData: Version" << objMd.obj_map.obj_map_ver
             << "  objId : " << objMd.obj_map.obj_id.metaDigest
             << "  obj_size " << objMd.obj_map.obj_size
             << "  obj_ref_cnt " << objMd.obj_map.obj_refcnt
             << "  num_assoc_entry " << objMd.obj_map.obj_num_assoc_entry
             << "  create_time " << objMd.obj_map.obj_create_time
             << "  del_time " << objMd.obj_map.obj_del_time
             << "  mod_time " << objMd.obj_map.assoc_mod_time
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
