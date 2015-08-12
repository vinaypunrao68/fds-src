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
#include "fdsp/sm_types_types.h"
#include "fdsp/FDSP_types.h"
#include "fds_volume.h"
#include "fds_types.h"

#include <persistent-layer/dm_io.h>

#include "fds_assert.h"
#include "fds_config.hpp"
#include "util/timeutils.h"
#include "serialize.h"

namespace fds {


/*
 * Persistent class for storing MetaObjMap, which
 * maps an object ID to its locations in the persistent
 * layer.
 */
class ObjMetaData : public serialize::Serializable {
  public:
    typedef boost::shared_ptr<ObjMetaData> ptr;
    typedef boost::shared_ptr<const ObjMetaData> const_ptr;

    ObjMetaData();
    explicit ObjMetaData(const ObjectBuf& buf);
    explicit ObjMetaData(const ObjMetaData::const_ptr &rhs);
    explicit ObjMetaData(const ObjMetaData &rhs);
    virtual ~ObjMetaData();

    void initialize(const ObjectID& objid, fds_uint32_t obj_size);

    bool isInitialized() const;

    void unmarshall(const ObjectBuf& buf);

    virtual uint32_t write(serialize::Serializer* serializer) const override;

    virtual uint32_t read(serialize::Deserializer* deserializer) override;

    virtual uint32_t getEstimatedSize() const override;

    uint32_t serializeTo(ObjectBuf& buf) const;

    bool deserializeFrom(const ObjectBuf& buf);

    bool deserializeFrom(const leveldb::Slice& s);

    uint64_t getModificationTs() const;

    void diffObjectMetaData(const ObjMetaData::ptr oldObjMetaData);

    void syncObjectMetaData(fpi::CtrlObjectMetaDataSync& objMetaData);

    bool isEqualSyncObjectMetaData(fpi::CtrlObjectMetaDataSync& objMetaData);

    void propagateObjectMetaData(fpi::CtrlObjectMetaDataPropagate& objMetaData,
                                 fpi::ObjectMetaDataReconcileFlags reconcileFlag);

    Error updateFromRebalanceDelta(const fpi::CtrlObjectMetaDataPropagate& objMetaData);

    void setObjCorrupted();
    fds_bool_t isObjCorrupted() const;

    void setObjReconcileRequired();
    void unsetObjReconcileRequired();
    fds_bool_t isObjReconcileRequired() const;

    void initializeDelReconcile(const ObjectID& objid, fds_volid_t volId);
    void reconcilePutObjMetaData(ObjectID objId, fds_uint32_t objSize, fds_volid_t volId);
    fds_bool_t reconcileDelObjMetaData(ObjectID objId, fds_volid_t vol_id, fds_uint64_t ts);
    void reconcileDeltaObjMetaData(const fpi::CtrlObjectMetaDataPropagate& objMetaData);
    fds_bool_t isVolAssocReconciled(fds_uint64_t& totalVolRefCnt,
                                    fds_int64_t& totalReconcileVolRefCnt);

    bool dataPhysicallyExists() const;

    fds_uint32_t   getObjSize() const;
    const obj_phy_loc_t* getObjPhyLoc(diskio::DataTier tier) const;
    meta_obj_map_t*   getObjMap();
    fds_uint64_t getCreationTime() const;

    void setRefCnt(fds_uint64_t refcnt);

    void incRefCnt();

    void decRefCnt();

    fds_uint64_t getRefCnt() const;

    void copyAssocEntry(ObjectID objId, fds_volid_t srcVolId, fds_volid_t destVolId);

    void updateAssocEntry(ObjectID objId, fds_volid_t vol_id);

    /**
     * @return true if metadata entry was changed, otherwise false
     */
    fds_bool_t deleteAssocEntry(ObjectID objId, fds_volid_t vol_id, fds_uint64_t ts);

    fds_bool_t isVolumeAssociated(fds_volid_t vol_id) const;
    std::vector<obj_assoc_entry_t>::iterator getAssociationIt(fds_volid_t volId);

    void getAssociatedVolumes(std::vector<fds_volid_t> &vols) const;

    void getVolsRefcnt(std::map<fds_volid_t, fds_uint64_t>& vol_refcnt) const;

    // Tiering/Physical Location update routines
    fds_bool_t onFlashTier() const;
    fds_bool_t onTier(diskio::DataTier tier) const;

    void updatePhysLocation(obj_phy_loc_t *in_phy_loc);
    void removePhyLocation(diskio::DataTier tier);

    bool operator==(const ObjMetaData &rhs) const;
    bool operator!= (const ObjMetaData &rhs) const
    { return !(this->operator==(rhs)); }

    std::string logString() const;

    // TODO(Anna) moved this up to be a public member so I can
    // remove SmObjDb dependency; when we replace SmObjDB with
    // ObjectMetaDb class, we will move it back to be private
    /* current object meta data */
    meta_obj_map_t obj_map;

  private:
    void mergeAssociationArrays_();

    friend std::ostream& operator<<(std::ostream& out, const ObjMetaData& objMap);

    /* Physical location entries.  Pointer to field inside obj_map */
    obj_phy_loc_t *phy_loc;

    /* Volume association entries */
    std::vector<obj_assoc_entry_t> assoc_entry;
};

inline std::ostream& operator<<(std::ostream& out, const ObjMetaData& objMd) {
    ObjectID oid(objMd.obj_map.obj_id.metaDigest);

    fds_assert(meta_obj_map_magic_value == objMd.obj_map.obj_magic);

    out << "Object MetaData: Version " << (fds_uint16_t)objMd.obj_map.obj_map_ver
        << " " << oid
        << "  obj_size " << objMd.obj_map.obj_size
        << "  obj_ref_cnt " << objMd.obj_map.obj_refcnt
        << "  num_assoc_entry " << objMd.obj_map.obj_num_assoc_entry
        << "  create_time " << objMd.obj_map.obj_create_time
        << "  del_time " << objMd.obj_map.obj_del_time
        << "  mod_time " << objMd.obj_map.assoc_mod_time
        << "  flags " << std::hex << objMd.obj_map.obj_flags << std::dec
        << "  obj_migration_dlt_version " << objMd.obj_map.obj_migration_reconcile_dlt_ver
        << "  obj_migration_reconcile_refcnt " << objMd.obj_map.obj_migration_reconcile_ref_cnt
        << std::endl;
    for (fds_uint32_t i = 0; i < MAX_PHY_LOC_MAP; i++) {
        out << "Object MetaData: "
            << " Tier (" << (fds_int16_t)objMd.phy_loc[i].obj_tier
            << ") loc id (" << objMd.phy_loc[i].obj_stor_loc_id
            << ") offset (" << objMd.phy_loc[i].obj_stor_offset
            << ") file id (" << objMd.phy_loc[i].obj_file_id << ")"
            << std::endl;
    }
    for (fds_uint32_t i = 0; i < objMd.obj_map.obj_num_assoc_entry; ++i) {
        out << "Assoc volume " << std::hex << objMd.assoc_entry[i].vol_uuid << std::dec
            << " vol_refcnt " << objMd.assoc_entry[i].ref_cnt
            << " vol_reconcile_refcnt " << objMd.assoc_entry[i].vol_migration_reconcile_ref_cnt
            << std::endl;
    }
    return out;
}

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_OBJMETA_H_
