/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <algorithm>
#include <sstream>
#include <map>
#include <string>
#include <vector>

#include <fdsp_utils.h>
#include <ObjMeta.h>

namespace fds {

SyncMetaData::SyncMetaData()
{
    reset();
}

/**
 * Resets sync metadata
 */
void SyncMetaData::reset()
{
    born_ts = 0;
    mod_ts = 0;
    assoc_entries.clear();
}

/**
 * Serialization routine
 * @param serializer
 * @return
 */
uint32_t SyncMetaData::write(serialize::Serializer* serializer) const
{
    uint32_t sz = 0;
    sz += serializer->writeI64((int64_t&)born_ts);
    sz += serializer->writeI64((int64_t&)mod_ts);
    sz += serializer->writeVector(assoc_entries);
    return sz;
}

/**
 * Deserialization routine
 * @param deserializer
 * @return
 */
uint32_t SyncMetaData::read(serialize::Deserializer* deserializer)
{
    uint32_t sz = 0;
    sz += deserializer->readI64((int64_t&)born_ts);
    sz += deserializer->readI64((int64_t&)mod_ts);
    sz += deserializer->readVector(assoc_entries);
    return sz;
}

/**
 * Return the the size of serialization buffer
 * @return
 */
uint32_t SyncMetaData::getEstimatedSize() const
{
    uint32_t sz = 0;
    sz += sizeof(born_ts) +
            sizeof(mod_ts) +
            serialize::getEstimatedSize(assoc_entries);
    return sz;
}

bool SyncMetaData::operator==(const SyncMetaData &rhs) const
{
    if (born_ts == rhs.born_ts && mod_ts == rhs.mod_ts &&
        assoc_entries.size() == rhs.assoc_entries.size()) {
        return (memcmp(assoc_entries.data(),
                rhs.assoc_entries.data(),
                sizeof(obj_assoc_entry_t) * assoc_entries.size()) == 0);
    }
    return false;
}

SyncMetaData&
SyncMetaData::operator=(const SyncMetaData &rhs) {
    born_ts = rhs.born_ts;
    mod_ts  = rhs.mod_ts;
    assoc_entries = rhs.assoc_entries;
    return *this;
}

ObjMetaData::ObjMetaData()
{
    memset(&obj_map, 0, sizeof(obj_map));
    obj_map.obj_magic = META_OBJ_MAP_MAGIC_VALUE;

    phy_loc = &obj_map.loc_map[0];
    phy_loc[diskio::flashTier].obj_tier = -1;
    phy_loc[diskio::diskTier].obj_tier = -1;
    phy_loc[3].obj_tier = -1;
}

ObjMetaData::ObjMetaData(const ObjectBuf& buf)
        : ObjMetaData() {
    fds_verify(deserializeFrom(buf) == true);
    obj_map.obj_magic = META_OBJ_MAP_MAGIC_VALUE;
}

ObjMetaData::ObjMetaData(const ObjMetaData::const_ptr &rhs) {
    memcpy(&obj_map, &(rhs->obj_map), sizeof(obj_map));
    obj_map.obj_magic = META_OBJ_MAP_MAGIC_VALUE;
    phy_loc = &obj_map.loc_map[0];
    assoc_entry = rhs->assoc_entry;
    sync_data   = rhs->sync_data;
}

ObjMetaData::~ObjMetaData()
{
}

/**
 *
 * @param objid
 * @param obj_size
 */
void ObjMetaData::initialize(const ObjectID& objid, fds_uint32_t obj_size) {
    memcpy(&obj_map.obj_id.metaDigest, objid.GetId(), sizeof(obj_map.obj_id.metaDigest));
    obj_map.obj_magic = META_OBJ_MAP_MAGIC_VALUE;
    obj_map.obj_size = obj_size;

    // Initialize the physical location array
    phy_loc = &obj_map.loc_map[0];
    phy_loc[diskio::flashTier].obj_tier = -1;
    phy_loc[diskio::diskTier].obj_tier = -1;
    phy_loc[3].obj_tier = -1;
}

/**
 *
 * @return
 */
bool ObjMetaData::isInitialized() const
{
    for (uint32_t i = 0; i < sizeof(obj_map.obj_id.metaDigest); ++i) {
        if (obj_map.obj_id.metaDigest[i] != 0) {
            return true;
        }
    }
    return false;
}
/**
 *
 * @param buf
 */
void ObjMetaData::unmarshall(const ObjectBuf& buf) {
    deserializeFrom(buf);
}
/**
 * Serialization routine
 * @param serializer
 * @return
 */
uint32_t ObjMetaData::write(serialize::Serializer* serializer) const
{
    uint32_t sz = 0;
    sz += serializer->writeBuffer(reinterpret_cast<int8_t*>(
            const_cast<meta_obj_map_t*>(&obj_map)),
            sizeof(obj_map));

    fds_verify(obj_map.obj_num_assoc_entry == assoc_entry.size());
    sz += serializer->writeVector(assoc_entry);

    if (syncDataExists()) {
        sz += sync_data.write(serializer);
    }
    fds_assert(sz == getEstimatedSize());
    return sz;
}

/**
 * Deserialization routine
 * @param deserializer
 * @return
 */
uint32_t ObjMetaData::read(serialize::Deserializer* deserializer)
{
    uint32_t sz = 0;

    uint32_t nread = deserializer->\
            readBuffer(reinterpret_cast<int8_t*>(&obj_map), sizeof(obj_map));
    fds_assert(nread == sizeof(obj_map));
    sz += nread;

    phy_loc = &obj_map.loc_map[0];

    sz += deserializer->readVector(assoc_entry);
    fds_assert(assoc_entry.size() == obj_map.obj_num_assoc_entry);

    if (syncDataExists()) {
        sz += sync_data.read(deserializer);
    }

    fds_assert(sz == getEstimatedSize());
    return sz;
}

/**
 * Serialziation buffer size is returned here.
 * @return
 */
uint32_t ObjMetaData::getEstimatedSize() const
{
    uint32_t sz = sizeof(obj_map) +
            serialize::getEstimatedSize(assoc_entry);

    if (syncDataExists()) {
        sz += sync_data.getEstimatedSize();
    }

    return sz;
}

/**
 *
 * @param buf
 * @return
 */
uint32_t ObjMetaData::serializeTo(ObjectBuf& buf) const
{
    Error ret = getSerialized(*(buf.data));
    fds_assert(ret.ok());
    return buf.getSize();
}

/**
 *
 * @param buf
 * @return
 */
bool ObjMetaData::deserializeFrom(const ObjectBuf& buf)
{
    Error ret = loadSerialized(*(buf.data));
    fds_assert(ret.ok());
    return ret.ok();
}

/**
 *
 * @param s
 * @return
 */
bool ObjMetaData::deserializeFrom(const leveldb::Slice& s)
{
    // TODO(Rao): Avoid the extra copy from s.ToString().  Have
    // leveldb expose underneath string
    Error ret = loadSerialized(s.ToString());
    fds_assert(ret.ok());
    return ret.ok();
}

/**
 *
 * @return
 */
uint64_t ObjMetaData::getModificationTs() const
{
    return obj_map.assoc_mod_time;
}

/**
 *
 * @return
 */
fds_uint32_t   ObjMetaData::getObjSize() const
{
    return obj_map.obj_size;
}

/**
 *
 * @param tier
 * @return
 */
const obj_phy_loc_t*
ObjMetaData::getObjPhyLoc(diskio::DataTier tier) const {
    return &phy_loc[tier];
}

/**
 * @brief
 *
 * @return
 */
fds_uint64_t ObjMetaData::getCreationTime() const
{
    return obj_map.obj_create_time;
}

/**
 *
 * @return
 */
meta_obj_map_t*   ObjMetaData::getObjMap() {
    return &obj_map;
}

/**
 *
 * @param refcnt
 */
void ObjMetaData::setRefCnt(fds_uint64_t refcnt) {
    obj_map.obj_refcnt = refcnt;
}

/**
 *
 * @return refcnt
 */
fds_uint64_t ObjMetaData::getRefCnt() const{
    return obj_map.obj_refcnt;
}


/**
 *
 */
void ObjMetaData::incRefCnt() {
    obj_map.obj_refcnt++;
}

/**
 *
 */
void ObjMetaData::decRefCnt() {
    obj_map.obj_refcnt--;
}

/**
 * Copy association from source volume to destination volume.
 * This is mostly used by snapshots/ clones.
 * @param objId
 * @param srcVolId
 * @param destVolId
 */
void ObjMetaData::copyAssocEntry(ObjectID objId, fds_volid_t srcVolId, fds_volid_t destVolId) {
    fds_assert(obj_map.obj_num_assoc_entry == assoc_entry.size());

    for (int i = 0; i < obj_map.obj_num_assoc_entry; ++i) {
        if (destVolId == assoc_entry[i].vol_uuid) {
            GLOGWARN << "Entry already exists!";
            return;
        }
    }

    int pos = 0;
    for (; pos < obj_map.obj_num_assoc_entry; ++pos) {
        if (srcVolId == assoc_entry[pos].vol_uuid) {
            break;
        }
    }
    if (obj_map.obj_num_assoc_entry >= pos) {
        GLOGWARN << "Source volume not found!";
        return;
    }

    obj_assoc_entry_t new_association;
    new_association.vol_uuid = destVolId;
    new_association.ref_cnt = assoc_entry[pos].ref_cnt;
    obj_map.obj_refcnt += assoc_entry[pos].ref_cnt;
    assoc_entry.push_back(new_association);
    obj_map.obj_num_assoc_entry = assoc_entry.size();
}

/**
 * Updates volume association entry
 * @param objId
 * @param vol_id
 */
void ObjMetaData::updateAssocEntry(ObjectID objId, fds_volid_t vol_id) {
    fds_assert(obj_map.obj_num_assoc_entry == assoc_entry.size());
    for (int i = 0; i < obj_map.obj_num_assoc_entry; ++i) {
        if (vol_id == assoc_entry[i].vol_uuid) {
            assoc_entry[i].ref_cnt++;
            obj_map.obj_refcnt++;
            return;
        }
    }
    obj_assoc_entry_t new_association;
    new_association.vol_uuid = vol_id;
    new_association.ref_cnt = 1L;
    new_association.vol_migration_reconcile_ref_cnt = 0L;
    obj_map.obj_refcnt++;
    assoc_entry.push_back(new_association);
    obj_map.obj_num_assoc_entry = assoc_entry.size();
}

/**
 * Delete volumes association
 * @param objId
 * @param vol_id
 * @return true if meta entry was changed
 */
fds_bool_t ObjMetaData::deleteAssocEntry(ObjectID objId, fds_volid_t vol_id, fds_uint64_t ts) {
    fds_assert(obj_map.obj_num_assoc_entry == assoc_entry.size());
    std::vector<obj_assoc_entry_t>::iterator it;
    for (it = assoc_entry.begin(); it != assoc_entry.end(); ++it) {
        if (vol_id == (*it).vol_uuid) break;
    }
    // If Volume did not put this objId then it delete is a noop
    if (it == assoc_entry.end()) return false;

    // found association, decrement ref counts
    fds_verify((*it).ref_cnt > 0L);
    (*it).ref_cnt--;
    if ((*it).ref_cnt == 0L) {
        assoc_entry.erase(it);
    }
    obj_map.obj_num_assoc_entry = assoc_entry.size();
    obj_map.obj_refcnt--;
    if (obj_map.obj_refcnt == 0L) {
        obj_map.obj_del_time = ts;
    }
    return true;
}

void
ObjMetaData::getVolsRefcnt(std::map<fds_volid_t,
                           fds_uint32_t>& vol_refcnt) const {
    vol_refcnt.clear();
    fds_assert(obj_map.obj_num_assoc_entry == assoc_entry.size());
    for (int i = 0; i < obj_map.obj_num_assoc_entry; ++i) {
        if (assoc_entry[i].ref_cnt > 0L) {
            if (vol_refcnt.count(assoc_entry[i].vol_uuid) == 0) {
                vol_refcnt[assoc_entry[i].vol_uuid] = 0;
            }
            vol_refcnt[assoc_entry[i].vol_uuid] += assoc_entry[i].ref_cnt;
        }
    }
}

/**
 * Checks if an association entry exists for volume
 * @param vol_id
 * @return
 */
fds_bool_t ObjMetaData::isVolumeAssociated(fds_volid_t vol_id) const
{
    for (int i = 0; i < obj_map.obj_num_assoc_entry; ++i) {
        if (vol_id == assoc_entry[i].vol_uuid) {
            return true;
        }
    }
    return false;
}

/**
 * Returns index of an association entry for the given volume
 * If volume is not associated, returns assoc_entry.end()
 */
std::vector<obj_assoc_entry_t>::iterator
ObjMetaData::getAssociationIt(fds_volid_t volId) {
    std::vector<obj_assoc_entry_t>::iterator it;
    for (it = assoc_entry.begin(); it != assoc_entry.end(); ++it) {
        if (volId == (*it).vol_uuid) break;
    }
    return it;
}

/*
* @brief copies associated volume information into vols
*
* @param vols
*/
void ObjMetaData::getAssociatedVolumes(std::vector<fds_volid_t> &vols) const
{
    vols.clear();
    for (int i = 0; i < obj_map.obj_num_assoc_entry; ++i) {
        vols.push_back(assoc_entry[i].vol_uuid);
    }
}

/**
 *
 * @return
 */
fds_bool_t ObjMetaData::onFlashTier() const {
    if (phy_loc[diskio::flashTier].obj_tier == diskio::flashTier) {
        return true;
    }
    return false;
}

/**
 *
 * @return true if data is on given tier
 */
fds_bool_t ObjMetaData::onTier(diskio::DataTier tier) const {
    return (phy_loc[tier].obj_tier == tier);
}


/**
 *
 * @param in_phy_loc
 */
void ObjMetaData::updatePhysLocation(obj_phy_loc_t *in_phy_loc) {
    memcpy(&phy_loc[(fds_uint32_t)in_phy_loc->obj_tier], in_phy_loc, sizeof(obj_phy_loc_t));
}

/**
 *
 * @param tier
 */
void ObjMetaData::removePhyLocation(diskio::DataTier tier) {
    phy_loc[tier].obj_tier = -1;
}
/**
 * Applies incoming data to metadata.
 * @param data
 */
void ObjMetaData::apply(const fpi::FDSP_MigrateObjectMetadata& data)
{
    fds_assert(!syncDataExists());

    /* Of sync metadata Object size and object id don't require a merge.
     * They can directly be applied to meta data. NOTE: If obj_map has
     * these fields set, incoming sync entry must match with existing.
     */
    if (obj_map.obj_size == 0) {
        obj_map.obj_size = data.obj_len;
        memcpy(obj_map.obj_id.metaDigest, data.object_id.digest.data(),
                sizeof(obj_map.obj_id.metaDigest));
    } else {
        fds_assert(obj_map.obj_size == static_cast<uint32_t>(data.obj_len));
        fds_assert(memcmp(obj_map.obj_id.metaDigest, data.object_id.digest.data(),
                sizeof(obj_map.obj_id.metaDigest)) == 0);
    }
    /* Creation time */
    obj_map.obj_create_time = data.born_ts;

    /* Modification time */
    obj_map.assoc_mod_time = data.modification_ts;

    /* association entries */
    assoc_entry.clear();
    obj_map.obj_refcnt = 0;
    for (auto itr : data.associations) {
        obj_assoc_entry_t e;
        e.ref_cnt = itr.ref_cnt;
        e.vol_uuid = itr.vol_id.uuid;
        assoc_entry.push_back(e);

        obj_map.obj_refcnt += itr.ref_cnt;
    }
    obj_map.obj_num_assoc_entry = assoc_entry.size();
}
/**
 * Extracts sync entry data.  Metadata that is node specific is skipped
 * @param md
 */
void ObjMetaData::extractSyncData(fpi::FDSP_MigrateObjectMetadata &md) const
{
    /* Object id */
    fds::assign(md.object_id, obj_map.obj_id);

    /* Object len */
    md.obj_len = static_cast<int32_t>(getObjSize());

    /* Born timestamp */
    md.born_ts = obj_map.obj_create_time;

    /* Modification timestamp */
    md.modification_ts = getModificationTs();

    if (obj_map.obj_refcnt == 0)  {
        return;
    }
    /* Association entries */
    fds_assert(obj_map.obj_num_assoc_entry == assoc_entry.size());
    for (uint32_t i = 0; i < obj_map.obj_num_assoc_entry; ++i) {
        if (assoc_entry[i].ref_cnt > 0L) {
            fds_assert(assoc_entry[i].vol_uuid != 0);
            fpi::FDSP_ObjectVolumeAssociation a;
            a.vol_id.uuid = assoc_entry[i].vol_uuid;
            a.ref_cnt = assoc_entry[i].ref_cnt;
            md.associations.push_back(a);
        }
    }
}

struct AssocEntryLess {
    bool operator() (const obj_assoc_entry_t &assocEntry1,
                     const obj_assoc_entry_t &assocEntry2)
    {
        return assocEntry1.vol_uuid < assocEntry2.vol_uuid;
    }
};

/**
 * This function currently calculates difference between two object's metadata ref_cnts:
 * 1) object reference count
 * 2) per volume association reference count.
 *
 * The metadata now contains the difference between this and old object meta data.
 *
 * In the future, we may need additional fields to diff two object metadata.
 */
void
ObjMetaData::diffObjectMetaData(const ObjMetaData::ptr oldObjMetaData)
{
    LOGMIGRATE << "OLD Object MetaData: " << oldObjMetaData->logString();
    LOGMIGRATE << "NEW Object MetaData: " << logString();

    fds_assert(memcmp(obj_map.obj_id.metaDigest, oldObjMetaData->obj_map.obj_id.metaDigest,
               sizeof(obj_map.obj_id.metaDigest)) == 0);

    /* calculate the refcnt change */
    obj_map.obj_refcnt = (uint64_t)((int64_t)obj_map.obj_refcnt -
                                    (int64_t)oldObjMetaData->obj_map.obj_refcnt);

    fds_assert(obj_map.obj_num_assoc_entry == assoc_entry.size());
    fds_assert(oldObjMetaData->obj_map.obj_num_assoc_entry == oldObjMetaData->assoc_entry.size());

    /* Sort both the old metadata and new metadata volume association, so
     * it's easier to determine the changes between two sets - O(nlogn) + O(n).
     */
    std::sort(assoc_entry.begin(), assoc_entry.end(), AssocEntryLess());
    std::sort(oldObjMetaData->assoc_entry.begin(),
              oldObjMetaData->assoc_entry.end(),
              AssocEntryLess());

    auto newIter = assoc_entry.begin();
    auto oldIter = oldObjMetaData->assoc_entry.begin();

    /* Following conditions are handled:
     * 1) volume association exists in both new and old set.
     *       - update it with diff.
     * 2) volume assoction exists in old but not in new.
     *       - update it with *negative* value of old entry.
     * 3) volume association exists in new, but not in old
     *       - do nothing.
     */
    while (oldIter != oldObjMetaData->assoc_entry.end()) {
        if (oldIter->vol_uuid == newIter->vol_uuid) {
            /* This is a case where volume association appears on both obj metadata.
             * Get the *signed* value and update it with the diff.
             */
            newIter->ref_cnt = (uint64_t)((int64_t)newIter->ref_cnt -
                                          (int64_t)oldIter->ref_cnt);

            ++oldIter;

            if (newIter != assoc_entry.end()) {
                ++newIter;
            }
        } else if (oldIter->vol_uuid < newIter->vol_uuid) {
            /* This is a case where volume association appears on the old list but
             * not on the new list.  This means that the volume association has
             * disappeared.  Diff is *negative* of the volume ref_cnt.  Add the
             * entry to the new list, so when propagated to the destination SM, it
             * will appropriately reflect that volume association has disappeared.
             */
            oldIter->ref_cnt = (uint64_t)((int64_t)-(oldIter->ref_cnt));
            assoc_entry.push_back(*oldIter);

            ++oldIter;
        } else {
            /* It's in the new list, but not in the old list.  no need to do anything
             * here.
             */
            if (newIter != assoc_entry.end()) {
                ++newIter;
            }
        }
    }

    /* Just update the assoc entry number.  this is just to avoid headache
     * later with all the assert in the existing code base.
     */
    obj_map.obj_num_assoc_entry = assoc_entry.size();

    LOGMIGRATE << "DIFF of OLD/NEW: " << logString();
}


void
ObjMetaData::propagateObjectMetaData(fpi::CtrlObjectMetaDataPropagate &objMetaData,
                                     bool reconcileMetaDataOnly)
{
    fds::assign(objMetaData.objectID, obj_map.obj_id);

    /* Even ObjectMetaDataReconcileOny flag is set, still copy over
     * the entire ObjectMetaData.
     */
    objMetaData.isObjectMetaDataReconcile = reconcileMetaDataOnly;
    objMetaData.objectRefCnt = getRefCnt();
    objMetaData.objectCompressType = obj_map.compress_type;
    objMetaData.objectCompressLen = obj_map.compress_len;
    objMetaData.objectBlkLen = obj_map.obj_blk_len;
    objMetaData.objectSize = getObjSize();
    objMetaData.objectFlags = obj_map.obj_flags;
    objMetaData.objectExpireTime = obj_map.expire_time;

    fds_verify(obj_map.obj_num_assoc_entry == assoc_entry.size());
    for (uint32_t i = 0; i < obj_map.obj_num_assoc_entry; ++i) {
        if (assoc_entry[i].ref_cnt > 0L) {
            fds_verify(assoc_entry[i].vol_uuid != 0);
            fpi::MetaDataVolumeAssoc volAssoc;
            volAssoc.volumeAssoc = assoc_entry[i].vol_uuid;
            volAssoc.volumeRefCnt = assoc_entry[i].ref_cnt;
            objMetaData.objectVolumeAssoc.push_back(volAssoc);
        }
    }
}

Error
ObjMetaData::updateFromRebalanceDelta(const fpi::CtrlObjectMetaDataPropagate& objMetaData)
{
    Error err(ERR_OK);

    if (objMetaData.isObjectMetaDataReconcile) {
        // objMetaData contain changes to the metadata since object
        // was migrated to this SM

        // these fields must not change at least in current implementation
        // may not be true in the future...
        if ((obj_map.compress_type != objMetaData.objectCompressType) ||
            (obj_map.compress_len != (fds_uint32_t)objMetaData.objectCompressLen) ||
            (obj_map.obj_blk_len != objMetaData.objectBlkLen) ||
            (obj_map.obj_size != (fds_uint32_t)objMetaData.objectSize) ||
            (obj_map.expire_time != (fds_uint64_t)objMetaData.objectExpireTime)) {
            return ERR_SM_TOK_MIGRATION_METADATA_MISMATCH;
        }

        // if object is corrupted on source, set corrupted here too.
        // should not trust that SM with the object..
        obj_map.obj_flags = objMetaData.objectFlags;

        // reconcile refcnt
        fds_int64_t newRefcnt = obj_map.obj_refcnt + objMetaData.objectRefCnt;
        if (newRefcnt < 0) {
            LOGERROR << "Cannot reconcile refcnt: existing refcnt "
                     << obj_map.obj_refcnt << ", diff from destination SM "
                     << objMetaData.objectRefCnt;
            return ERR_SM_TOK_MIGRATION_METADATA_MISMATCH;
        }
        obj_map.obj_refcnt = newRefcnt;

        // reconcile volume association
        std::vector<obj_assoc_entry_t>::iterator it;
        for (auto volAssoc : objMetaData.objectVolumeAssoc) {
            it = getAssociationIt(volAssoc.volumeAssoc);
            if (it != assoc_entry.end()) {
                // found volume association, reconcile
                newRefcnt = it->ref_cnt + volAssoc.volumeRefCnt;
                if (newRefcnt >= 0) {
                    it->ref_cnt = newRefcnt;
                    if (newRefcnt == 0) {
                        assoc_entry.erase(it);
                        obj_map.obj_num_assoc_entry = assoc_entry.size();
                    }
                } else {
                    err = ERR_SM_TOK_MIGRATION_METADATA_MISMATCH;
                }
            } else {
                // this is a new association..
                if (volAssoc.volumeRefCnt >= 0) {
                    obj_assoc_entry_t new_association;
                    new_association.vol_uuid = volAssoc.volumeAssoc;
                    new_association.ref_cnt = volAssoc.volumeRefCnt;
                    assoc_entry.push_back(new_association);
                    obj_map.obj_num_assoc_entry = assoc_entry.size();
                } else {
                    err = ERR_SM_TOK_MIGRATION_METADATA_MISMATCH;
                }
            }

            if (!err.ok()) {
                LOGERROR << "Cannot reconcile refcnt for volume "
                         << std::hex << volAssoc.volumeAssoc << std::dec
                         << " : existing refcnt "
                         << obj_map.obj_refcnt << ", diff from destination SM "
                         << volAssoc.volumeRefCnt;
                return err;
            }
        }
    } else {
        // over-write metadata
        if (objMetaData.objectRefCnt < 0) {
            LOGERROR << "Object refcnt must be > 0 if isObjectMetaDataReconcile is false "
                     << " refcnt = " << objMetaData.objectRefCnt;
            return ERR_INVALID_ARG;
        }
        setRefCnt(objMetaData.objectRefCnt);

        obj_map.compress_type = objMetaData.objectCompressType;
        obj_map.compress_len = objMetaData.objectCompressLen;
        obj_map.obj_blk_len = objMetaData.objectBlkLen;
        obj_map.obj_size = objMetaData.objectSize;
        obj_map.expire_time = objMetaData.objectExpireTime;

        // TODO(Anna) do not over-write if data corrupted flag set
        // unless we got the data from source SM and can recover...
        obj_map.obj_flags = objMetaData.objectFlags;

        // over-write volume association
        assoc_entry.clear();
        for (auto volAssoc : objMetaData.objectVolumeAssoc) {
            obj_assoc_entry_t new_association;
            new_association.vol_uuid = volAssoc.volumeAssoc;
            if (volAssoc.volumeRefCnt < 0) {
                LOGERROR << "Object vol assoc refcnt must be > 0 if isObjectMetaDataReconcile "
                         << "is false refcnt = " << objMetaData.objectRefCnt;
                return ERR_INVALID_ARG;
            }
            new_association.ref_cnt = volAssoc.volumeRefCnt;
            assoc_entry.push_back(new_association);
            obj_map.obj_num_assoc_entry = assoc_entry.size();
        }
    }

    return ERR_OK;
}

/**
 * While sync is in progress, existin metadata prior to sync point needs
 * to demoted as the sync data.  We do this process here.  While demoting
 * any replicable data is demoted to sync data.
 * @param syncTs
 */
void ObjMetaData::checkAndDemoteUnsyncedData(const uint64_t& syncTs)
{
    if (!syncDataExists() &&
            obj_map.assoc_mod_time != 0 &&
            obj_map.assoc_mod_time < syncTs) {
        /* sync_data will inherit fields from current metadata.  We will do
         * this one field at a time.
         * NOTE: For each field here, there should be an association resolve in
         * mergeNewAndUnsyncedData
         */
        obj_map.obj_flags |= SYNCMETADATA_MASK;

        /* Creation time */
        sync_data.born_ts = obj_map.obj_create_time;
        /* Modification time */
        sync_data.mod_ts = obj_map.assoc_mod_time;
        /* association entries */
        sync_data.assoc_entries = assoc_entry;

        /* Clear existing modification time.  NOTE: We keep the creation
         * time.  However, during merge we'll use creation time from replica
         */
        obj_map.assoc_mod_time = 0;
        /* Clear association information from existing metadata */
        obj_map.obj_refcnt = 0;
        obj_map.obj_num_assoc_entry = 0;
        assoc_entry.clear();
    }
}

/**
 * Applies data to sync metadata.  Normal metadata is unaffected.
 * @param data
 */
void ObjMetaData::applySyncData(const fpi::FDSP_MigrateObjectMetadata& data)
{
    if (!syncDataExists()) {
        obj_map.obj_flags |= SYNCMETADATA_MASK;
        LOGWARN << "syncDataExists is false " << logString();
    }

    /* Of sync metadata Object size and object id don't require a merge.
     * They can directly be applied to meta data. NOTE: If obj_map has
     * these fields set, incoming sync entry must match with existing.
     */
    if (obj_map.obj_size == 0) {
        obj_map.obj_size = data.obj_len;
        memcpy(obj_map.obj_id.metaDigest, data.object_id.digest.data(),
                sizeof(obj_map.obj_id.metaDigest));
    } else {
        fds_assert(obj_map.obj_size == static_cast<uint32_t>(data.obj_len));
        fds_assert(memcmp(obj_map.obj_id.metaDigest, data.object_id.digest.data(),
                sizeof(obj_map.obj_id.metaDigest)) == 0);
    }

    fds_assert(sync_data.mod_ts <= static_cast<uint64_t>(data.modification_ts));

    /* Creation time */
    sync_data.born_ts = data.born_ts;

    /* Modification time */
    sync_data.mod_ts = data.modification_ts;

    /* association entries */
    sync_data.assoc_entries.clear();
    for (auto itr : data.associations) {
        obj_assoc_entry_t e;
        e.ref_cnt = itr.ref_cnt;
        e.vol_uuid = itr.vol_id.uuid;
        sync_data.assoc_entries.push_back(e);
    }
}

/**
 * Merges sync data with current metadata.  This is done after
 * sync transfer.
 */
void ObjMetaData::mergeNewAndUnsyncedData()
{
    fds_assert(syncDataExists());
    /* We will resolve one field at a time*/

#if 0
    if (sync_data.mod_ts >= obj_map.assoc_mod_time) {
        /* Sync metadata is more recent compared to object metadata.
         * Clear sync-able fields.  We'll just use them from sync entry
         * See merge code after this if statement.
         */
        obj_map.obj_create_time = 0;
        obj_map.assoc_mod_time = 0;
        obj_map.obj_num_assoc_entry = 0;
        assoc_entry.clear();
        obj_map.obj_refcnt = 0;

        /* Merge happens below */
    }
#endif

    /* Creation timestamp.  We will honor the one from sync entry */
    obj_map.obj_create_time = sync_data.born_ts;

    /* Modification timestamp.  We'll use the recent of two */
    obj_map.assoc_mod_time = (sync_data.mod_ts > obj_map.assoc_mod_time) ?
            sync_data.mod_ts : obj_map.assoc_mod_time;

    /* Association entries.  We will merge these */
    mergeAssociationArrays_();

    /* Other adjustments/accounting of obj_map based on above changes */
    obj_map.obj_num_assoc_entry = assoc_entry.size();
    obj_map.obj_refcnt = 0;
    for (auto e : assoc_entry)  {
        obj_map.obj_refcnt += e.ref_cnt;
    }
    /* Sync data isn't needed anymore */
    obj_map.obj_flags &= ~SYNCMETADATA_MASK;
    sync_data.reset();
}

/**
 * Less than operator
 */
struct obj_assoc_entryLess {
    bool operator() (const obj_assoc_entry_t &a, const obj_assoc_entry_t& b)
    {
        return a.vol_uuid < b.vol_uuid;
    }
};

/**
 * Merges the sync association array with current metadata association array
 * This is done after sync transfer completes
 */
void ObjMetaData::mergeAssociationArrays_()
{
    if (sync_data.assoc_entries.size() == 0) {
        /* nothing to do */
        return;
    } else if (assoc_entry.size() == 0) {
        assoc_entry = sync_data.assoc_entries;
        return;
    }
    std::sort(sync_data.assoc_entries.begin(),
            sync_data.assoc_entries.end(), obj_assoc_entryLess());
    std::sort(assoc_entry.begin(), assoc_entry.end(), obj_assoc_entryLess());

    // TODO(Rao): consider doing a move here
    auto temp = assoc_entry;
    assoc_entry.clear();

    auto itr1 = sync_data.assoc_entries.begin();
    auto itr2 = temp.begin();
    while (itr1 != sync_data.assoc_entries.end() &&
           itr2 != temp.end()) {
        obj_assoc_entry_t e;
        if (itr1->vol_uuid == itr2->vol_uuid) {
            e = *itr1;
            e.ref_cnt += itr2->ref_cnt;
            ++itr1;
            ++itr2;
        } else if (itr1->vol_uuid < itr2->vol_uuid) {
            e = *itr1;
            ++itr1;
        } else {
            e = *itr2;
            ++itr2;
        }
        assoc_entry.push_back(e);
    }
    if (itr1 == sync_data.assoc_entries.end() &&
        itr2 == temp.end()) {
        return;
    } else if (itr1 != sync_data.assoc_entries.end()) {
        assoc_entry.insert(assoc_entry.end(), itr1, sync_data.assoc_entries.end());
    } else {
        assoc_entry.insert(assoc_entry.end(), itr2, temp.end());
    }
}

bool ObjMetaData::dataPhysicallyExists() const
{
    if (phy_loc[diskio::diskTier].obj_tier == diskio::diskTier ||
            phy_loc[diskio::flashTier].obj_tier == diskio::flashTier) {
        return true;
    }
    return false;
}

void ObjMetaData::setSyncMask()
{
    obj_map.obj_flags |= SYNCMETADATA_MASK;
}

void ObjMetaData::setObjCorrupted() {
    obj_map.obj_flags |= OBJ_FLAG_CORRUPTED;
}

fds_bool_t ObjMetaData::isObjCorrupted() const {
    return (obj_map.obj_flags & OBJ_FLAG_CORRUPTED);
}

bool ObjMetaData::syncDataExists() const
{
    return (obj_map.obj_flags & SYNCMETADATA_MASK) != 0;
}

/**
 * This operator compares subset of the ObjectMetaData.  For now, we are only
 * comparing following fields in ObjectMetaData.
 * - objID
 * - compression type
 * - compression len
 * - obj_blk_len
 * - obj_size
 * - obj_refcnt
 * - obj_flags
 * - expire_time
 * - vector<volume association>
 *
 *   Note: we are not memcmp() the whole obj_map data, since some information
 *         can change, like timestamp related fields, which we really don't
 *         care about.  This is currently used for token migration.
 *
 *   TODO(Sean):  Need to re-arrange the meta_obj_map to avoid
 *                this type of comparison.  We can memcmp() only relevent fields
 *                by packing them together at the top.
 *                i.e.  memcmp(&first_field,
 *                             &rhs.first_field,
 *                             offsetof(struct..., field_right_after_pertinent_elem));
 */
bool ObjMetaData::operator==(const ObjMetaData &rhs) const
{
    /* If any of the field do not match, then return false */
    if ((0 != memcmp(obj_map.obj_id.metaDigest,
                     rhs.obj_map.obj_id.metaDigest,
                     sizeof(obj_map.obj_id.metaDigest))) ||
        (obj_map.compress_type != rhs.obj_map.compress_type) ||
        (obj_map.compress_len != rhs.obj_map.compress_len) ||
        (obj_map.obj_blk_len != rhs.obj_map.obj_blk_len) ||
        (obj_map.obj_size != rhs.obj_map.obj_size) ||
        (obj_map.obj_refcnt != rhs.obj_map.obj_refcnt) ||
        (obj_map.expire_time != rhs.obj_map.expire_time)) {
        return false;
    }

    /* if the volume association entry size is different, then
     * metadata is different.
     */
    if (assoc_entry.size() != rhs.assoc_entry.size()) {
        return false;
    } else {
        /* assoc_entry size is the same.  Just memcmp the vector data */
        if (0 != memcmp(assoc_entry.data(),
                        rhs.assoc_entry.data(),
                        sizeof(obj_assoc_entry_t) * assoc_entry.size())) {
            return false;
        }
    }

    return true;
}


std::string ObjMetaData::logString() const
{
    std::ostringstream oss;

    ObjectID obj_id(std::string((const char*)(obj_map.obj_id.metaDigest),
                    sizeof(obj_map.obj_id.metaDigest)));

    oss << "id=" << obj_id
        << " refcnt=" << obj_map.obj_refcnt
        << " flags=" << (uint32_t)obj_map.obj_flags
        << " compression_type=" << obj_map.compress_type
        << " compress_len=" << obj_map.compress_len
        << " blk_len=" << obj_map.obj_blk_len
        << " len=" << obj_map.obj_size
        << " expire_time=" << obj_map.expire_time
        << " assoc_entry_cnt=" << assoc_entry.size()
        << " vol_id:refcnt=";
    for (auto entry : assoc_entry)  {
        oss << "(" << std::hex << entry.vol_uuid << std::dec << ":" << entry.ref_cnt << "), ";
    }
    oss << std::endl;
    return oss.str();
}
}  // namespace fds
