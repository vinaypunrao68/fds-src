/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_OBJMETA_H_
#define SOURCE_STOR_MGR_INCLUDE_OBJMETA_H_

extern "C" {
#include <assert.h>
#include <unistd.h>
}

#include <atomic>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "leveldb/db.h"
#include "fdsp/FDSP_types.h"
#include "fds_volume.h"
#include "fds_types.h"

#include "util/Log.h"
#include "persistent_layer/dm_service.h"
#include "persistent_layer/dm_io.h"

#include "fds_assert.h"
#include "fds_config.hpp"
#include "util/timeutils.h"
#include "serialize.h"

namespace fds {

#define SYNCMETADATA_MASK 0x1

struct SyncMetaData : public serialize::Serializable {
    SyncMetaData();
    void reset();

    /* Overrides from Serializable */
    virtual uint32_t write(serialize::Serializer* serializer) const override;
    virtual uint32_t read(serialize::Deserializer* deserializer) override;
    virtual uint32_t getEstimatedSize() const override;

    bool operator== (const SyncMetaData &rhs) const;

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
    public:
     ObjMetaData();
     virtual ~ObjMetaData();

     typedef boost::shared_ptr<ObjMetaData> ptr;
     typedef boost::shared_ptr<const ObjMetaData> const_ptr;

     void initialize(const ObjectID& objid, fds_uint32_t obj_size);

     bool isInitialized() const;

     explicit ObjMetaData(const ObjectBuf& buf);

     void unmarshall(const ObjectBuf& buf);

     virtual uint32_t write(serialize::Serializer* serializer) const override;

     virtual uint32_t read(serialize::Deserializer* deserializer) override;

     virtual uint32_t getEstimatedSize() const override;

     uint32_t serializeTo(ObjectBuf& buf) const;

     bool deserializeFrom(const ObjectBuf& buf);

     bool deserializeFrom(const leveldb::Slice& s);

     uint64_t getModificationTs() const;

     void apply(const fpi::FDSP_MigrateObjectMetadata& data);

     void extractSyncData(fpi::FDSP_MigrateObjectMetadata& md) const;

     void checkAndDemoteUnsyncedData(const uint64_t& syncTs);

     void setSyncMask();

     bool syncDataExists() const;

     void applySyncData(const fpi::FDSP_MigrateObjectMetadata& data);

     void mergeNewAndUnsyncedData();

     bool dataPhysicallyExists();

     fds_uint32_t   getObjSize() const;
     obj_phy_loc_t*   getObjPhyLoc(diskio::DataTier tier);
     meta_obj_map_t*   getObjMap();

     void setRefCnt(fds_uint16_t refcnt);

     void incRefCnt();

     void decRefCnt();

     fds_uint16_t getRefCnt() const;

     void updateAssocEntry(ObjectID objId, fds_volid_t vol_id);

     void deleteAssocEntry(ObjectID objId, fds_volid_t vol_id, fds_uint64_t ts);

     fds_bool_t isVolumeAssociated(fds_volid_t vol_id);

     void getVolsRefcnt(std::map<fds_volid_t, fds_uint32_t>& vol_refcnt);

     // Tiering/Physical Location update routines
     fds_bool_t onFlashTier() const;
     fds_bool_t onTier(diskio::DataTier tier) const;

     void updatePhysLocation(obj_phy_loc_t *in_phy_loc);
     void removePhyLocation(diskio::DataTier tier);

     bool operator==(const ObjMetaData &rhs) const;

     std::string logString() const;

     // TODO(Anna) moved this up to be a public member so I can
     // remove SmObjDb dependency; when we replace SmObjDB with
     // ObjectMetaDb class, we will move it back to be private
     /* current object meta data */
     meta_obj_map_t obj_map;

    private:
     void mergeAssociationArrays_();

     friend std::ostream& operator<<(std::ostream& out, const ObjMetaData& objMap);

     /* Data mask to indicate which metadata members are valid.
      * When sync is in progress mask will be set to indicate sync_data
      * is valid.
      */
     uint8_t mask;

     /* Physical location entries.  Pointer to field inside obj_map */
     obj_phy_loc_t *phy_loc;

     /* Volume association entries */
     std::vector<obj_assoc_entry_t> assoc_entry;

     /* Sync related metadata.  Valid only when mask is set appropriately */
     SyncMetaData sync_data;
};

inline std::ostream& operator<<(std::ostream& out, const ObjMetaData& objMd) {
    ObjectID oid(objMd.obj_map.obj_id.metaDigest);
    out << "Object MetaData: Version " << (fds_uint16_t)objMd.obj_map.obj_map_ver
        << " " << oid
        << "  obj_size " << objMd.obj_map.obj_size
        << "  obj_ref_cnt " << objMd.obj_map.obj_refcnt
        << "  num_assoc_entry " << objMd.obj_map.obj_num_assoc_entry
        << "  create_time " << objMd.obj_map.obj_create_time
        << "  del_time " << objMd.obj_map.obj_del_time
        << "  mod_time " << objMd.obj_map.assoc_mod_time
        << std::endl;
    for (fds_uint32_t i = 0; i < MAX_PHY_LOC_MAP; i++) {
        out << "Object MetaData: "
            << " Tier (" << (fds_int16_t)objMd.phy_loc[i].obj_tier
            << ") loc id (" << objMd.phy_loc[i].obj_stor_loc_id
            << ") offset (" << objMd.phy_loc[i].obj_stor_offset
            << ") file id (" << objMd.phy_loc[i].obj_file_id << ")"
            << std::endl;
    }
    return out;
}

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_OBJMETA_H_
