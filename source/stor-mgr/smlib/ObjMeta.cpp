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

ObjMetaData::ObjMetaData()
{
    memset(&obj_map, 0, sizeof(obj_map));

    obj_map.obj_magic = meta_obj_map_magic_value;
    obj_map.obj_map_ver = meta_obj_map_version;

    phy_loc = &obj_map.loc_map[0];
    phy_loc[diskio::flashTier].obj_tier = -1;
    phy_loc[diskio::diskTier].obj_tier = -1;
    phy_loc[3].obj_tier = -1;
}

ObjMetaData::ObjMetaData(const ObjectBuf& buf)
        : ObjMetaData() {
    fds_verify(deserializeFrom(buf) == true);

    fds_verify(meta_obj_map_magic_value == obj_map.obj_magic);
    fds_verify(meta_obj_map_version == obj_map.obj_map_ver);
}

ObjMetaData::ObjMetaData(const ObjMetaData::const_ptr &rhs) {
    memcpy(&obj_map, &(rhs->obj_map), sizeof(obj_map));

    fds_verify(meta_obj_map_magic_value == obj_map.obj_magic);
    fds_verify(meta_obj_map_version == obj_map.obj_map_ver);

    phy_loc = &obj_map.loc_map[0];
    assoc_entry = rhs->assoc_entry;
}

ObjMetaData::ObjMetaData(const ObjMetaData &rhs) {
    memcpy(&obj_map, &(rhs.obj_map), sizeof(obj_map));

    fds_verify(meta_obj_map_magic_value == obj_map.obj_magic);
    fds_verify(meta_obj_map_version == obj_map.obj_map_ver);

    phy_loc = &obj_map.loc_map[0];
    assoc_entry = rhs.assoc_entry;
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
    obj_map.obj_magic = meta_obj_map_magic_value;
    obj_map.obj_map_ver = meta_obj_map_version;
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

    for (fds_uint32_t i = 0; i < obj_map.obj_num_assoc_entry; ++i) {
        if (destVolId == fds_volid_t(assoc_entry[i].vol_uuid)) {
            GLOGWARN << "Entry already exists!";
            return;
        }
    }

    fds_uint32_t pos = 0;
    for (; pos < obj_map.obj_num_assoc_entry; ++pos) {
        if (srcVolId == fds_volid_t(assoc_entry[pos].vol_uuid)) {
            break;
        }
    }
    if (obj_map.obj_num_assoc_entry >= pos) {
        GLOGWARN << "Source volume not found!";
        return;
    }

    obj_assoc_entry_t new_association;
    new_association.vol_uuid = destVolId.get();
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
    for (fds_uint32_t i = 0; i < obj_map.obj_num_assoc_entry; ++i) {
        if (vol_id == fds_volid_t(assoc_entry[i].vol_uuid)) {
            assoc_entry[i].ref_cnt++;
            obj_map.obj_refcnt++;
            return;
        }
    }
    obj_assoc_entry_t new_association;
    new_association.vol_uuid = vol_id.get();
    new_association.ref_cnt = 1UL;
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
        if (vol_id == fds_volid_t((*it).vol_uuid)) break;
    }
    // If Volume did not put this objId then it delete is a noop
    if (it == assoc_entry.end()) return false;

    // found association, decrement ref counts
    fds_verify((*it).ref_cnt > 0UL);
    (*it).ref_cnt--;
    if ((*it).ref_cnt == 0UL) {
        assoc_entry.erase(it);
    }

    obj_map.obj_num_assoc_entry = assoc_entry.size();

    obj_map.obj_refcnt--;
    if (obj_map.obj_refcnt == 0UL) {
        obj_map.obj_del_time = ts;
    }
    return true;
}

void
ObjMetaData::getVolsRefcnt(std::map<fds_volid_t, fds_uint64_t>& vol_refcnt) const {
    vol_refcnt.clear();
    fds_assert(obj_map.obj_num_assoc_entry == assoc_entry.size());
    for (fds_uint32_t i = 0; i < obj_map.obj_num_assoc_entry; ++i) {
        if (assoc_entry[i].ref_cnt > 0UL) {
            fds_volid_t volId(assoc_entry[i].vol_uuid);
            if (vol_refcnt.count(volId) == 0UL) {
                vol_refcnt[volId] = 0UL;
            }
            vol_refcnt[volId] += assoc_entry[i].ref_cnt;
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
    for (fds_uint32_t i = 0; i < obj_map.obj_num_assoc_entry; ++i) {
        if (vol_id == fds_volid_t(assoc_entry[i].vol_uuid)) {
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
        if (volId == fds_volid_t((*it).vol_uuid)) {
            break;
        }
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
    for (fds_uint32_t i = 0; i < obj_map.obj_num_assoc_entry; ++i) {
        vols.push_back(fds_volid_t(assoc_entry[i].vol_uuid));
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

    fds_assert(memcmp(obj_map.obj_id.metaDigest,
               oldObjMetaData->obj_map.obj_id.metaDigest,
               sizeof(obj_map.obj_id.metaDigest)) == 0);

    /* calculate the refcnt change */
    obj_map.obj_refcnt = (uint64_t)((int64_t)obj_map.obj_refcnt -
                                    (int64_t)oldObjMetaData->obj_map.obj_refcnt);

    /* Assert that obj_refcnt cannot be 0. Since we are currently only concerned about
     * refcnt, if the recnt is 0, that means that the ref cnt is the same.
     */
    fds_assert(obj_map.obj_refcnt != 0);


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
     * NOTE: old iter cannot be empty, since we selected objects with >0 refcnt.  However,
     *       but new iter can be empty, since the object could've been deleted, resulting in
     *       empty volume association.
     */
    while (oldIter != oldObjMetaData->assoc_entry.end()) {
        if (newIter == assoc_entry.end()) {
            /* We've reached the end of the new iter, or the new iter was
             * empty to begin with.  If that's the case, we need to add
             * the entries from the old volume association entry to new volume association.
             */
            oldIter->ref_cnt = (uint64_t)((int64_t)-(oldIter->ref_cnt));
            assoc_entry.push_back(*oldIter);

            ++oldIter;
        } else if (oldIter->vol_uuid == newIter->vol_uuid) {
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
ObjMetaData::syncObjectMetaData(fpi::CtrlObjectMetaDataSync &objMetaData)
{
    fds::assign(objMetaData.objectID, obj_map.obj_id);

    objMetaData.objRefCnt = getRefCnt();

    fds_verify(obj_map.obj_num_assoc_entry == assoc_entry.size());
    for (uint32_t i = 0; i < obj_map.obj_num_assoc_entry; ++i) {
        /* Intentionally, not checking if the volume refcnt is > 0, since
         * this is used to filter against the the destination's object
         * set against the source SM.
         * If anything is different on source, we will , then we will
         */
        fds_verify(assoc_entry[i].vol_uuid != 0);
        fpi::MetaDataVolumeAssoc volAssoc;
        volAssoc.volumeAssoc = assoc_entry[i].vol_uuid;
        volAssoc.volumeRefCnt = assoc_entry[i].ref_cnt;
        objMetaData.objVolAssoc.push_back(volAssoc);
    }
}

bool
ObjMetaData::isEqualSyncObjectMetaData(fpi::CtrlObjectMetaDataSync &objMetaData)
{
    ObjectID oid(obj_map.obj_id.metaDigest);
    std::string oidStr = oid.ToString();

    /* If any of the field do not match, then return false */
    if ((oidStr != objMetaData.objectID.digest) ||
        (obj_map.obj_refcnt != (fds_uint64_t)objMetaData.objRefCnt)) {
        return false;
    }

    /* if the volume association entry size is different, then
     * metadata is different.
     */
    if (assoc_entry.size() !=objMetaData.objVolAssoc.size()) {
        return false;
    } else {
        /* assoc_entry size is the same.  Just memcmp the vector data */
        if (0 != memcmp(assoc_entry.data(),
                        objMetaData.objVolAssoc.data(),
                        sizeof(obj_assoc_entry_t) * assoc_entry.size())) {
            return false;
        }
    }

    return true;
}

void
ObjMetaData::propagateObjectMetaData(fpi::CtrlObjectMetaDataPropagate &objMetaData,
                                     fpi::ObjectMetaDataReconcileFlags reconcileFlag)
{
    fds::assign(objMetaData.objectID, obj_map.obj_id);

    /* Even ObjectMetaDataReconcileOny flag is set, still copy over
     * the entire ObjectMetaData.
     */
    objMetaData.objectReconcileFlag = reconcileFlag;
    objMetaData.objectRefCnt = getRefCnt();
    objMetaData.objectCompressType = obj_map.compress_type;
    objMetaData.objectCompressLen = obj_map.compress_len;
    objMetaData.objectBlkLen = obj_map.obj_blk_len;
    objMetaData.objectSize = getObjSize();
    objMetaData.objectFlags = obj_map.obj_flags;
    objMetaData.objectExpireTime = obj_map.expire_time;

    fds_verify(obj_map.obj_num_assoc_entry == assoc_entry.size());
    for (uint32_t i = 0; i < obj_map.obj_num_assoc_entry; ++i) {
        if (assoc_entry[i].ref_cnt > 0UL) {
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

    if (fpi::OBJ_METADATA_RECONCILE == objMetaData.objectReconcileFlag) {
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

        // sum of all volume association should match the obj_refcnt.  If not,
        // something is wrong.
        // We will panic if obj_refcnt and sum of volume association ref_cnt do
        // no match.
        fds_uint64_t sumVolRefCnt = 0;

        // reconcile volume association
        std::vector<obj_assoc_entry_t>::iterator it;
        for (auto volAssoc : objMetaData.objectVolumeAssoc) {

            it = getAssociationIt(fds_volid_t(volAssoc.volumeAssoc));

            if (it != assoc_entry.end()) {
                // found volume association, reconcile
                newRefcnt = it->ref_cnt + volAssoc.volumeRefCnt;

                if (newRefcnt >= 0) {
                    it->ref_cnt = newRefcnt;
                    if (newRefcnt == 0) {
                        assoc_entry.erase(it);
                        obj_map.obj_num_assoc_entry = assoc_entry.size();
                    }
                    // sum up volume refcnt to sum for validation later.
                    sumVolRefCnt += newRefcnt;
                } else {
                    err = ERR_SM_TOK_MIGRATION_METADATA_MISMATCH;
                }

            } else {
                // this is a new association..
                if (volAssoc.volumeRefCnt >= 0) {
                    obj_assoc_entry_t new_association;
                    new_association.vol_uuid = volAssoc.volumeAssoc;
                    new_association.ref_cnt = volAssoc.volumeRefCnt;
                    new_association.vol_migration_reconcile_ref_cnt = 0L;
                    assoc_entry.push_back(new_association);
                    obj_map.obj_num_assoc_entry = assoc_entry.size();

                    // sum up volume refcnt to sum for validation later.
                    sumVolRefCnt += new_association.ref_cnt;
                } else {
                    err = ERR_SM_TOK_MIGRATION_METADATA_MISMATCH;
                }
            }

            if (!err.ok()) {
                LOGERROR << "Cannot reconcile refcnt for volume "
                         << fds_volid_t(volAssoc.volumeAssoc)
                         << " : existing refcnt "
                         << obj_map.obj_refcnt << ", diff from destination SM "
                         << volAssoc.volumeRefCnt;
                return err;
            }
        }
        // Verify that sum of all volume association ref cnt matches the
        // per object refcnt.
        // For now, the best thing is to panic if this occurs.
        fds_verify(obj_map.obj_refcnt == sumVolRefCnt);
    } else {

        // In either NO_RECONCILE or OVERWRITE, we are just overwriting the meta
        fds_verify((fpi::OBJ_METADATA_NO_RECONCILE == objMetaData.objectReconcileFlag) ||
                   (fpi::OBJ_METADATA_OVERWRITE == objMetaData.objectReconcileFlag));

        // !metadatareconcileonly
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
            new_association.vol_migration_reconcile_ref_cnt = 0L;
            assoc_entry.push_back(new_association);
        }
        obj_map.obj_num_assoc_entry = assoc_entry.size();
    }

    return ERR_OK;
}




bool ObjMetaData::dataPhysicallyExists() const
{
    if (phy_loc[diskio::diskTier].obj_tier == diskio::diskTier ||
            phy_loc[diskio::flashTier].obj_tier == diskio::flashTier) {
        return true;
    }
    return false;
}

void ObjMetaData::setObjCorrupted() {
    obj_map.obj_flags |= OBJ_FLAG_CORRUPTED;

    ObjectID oid(obj_map.obj_id.metaDigest);

    LOGCRITICAL << "CORRUPTION: Setting OBJ_FLAG_CORRUPTED flag on obj="
                << oid;
}

fds_bool_t ObjMetaData::isObjCorrupted() const {
    return (obj_map.obj_flags & OBJ_FLAG_CORRUPTED);
}

void
ObjMetaData::setObjReconcileRequired()
{
    obj_map.obj_flags |= OBJ_FLAG_RECONCILE_REQUIRED;
}
void
ObjMetaData::unsetObjReconcileRequired()
{
    obj_map.obj_flags &= ~OBJ_FLAG_RECONCILE_REQUIRED;
}

fds_bool_t
ObjMetaData::isObjReconcileRequired() const
{
    return (obj_map.obj_flags & OBJ_FLAG_RECONCILE_REQUIRED);
}


void
ObjMetaData::initializeDelReconcile(const ObjectID& objId, fds_volid_t volId)
{
    initialize(objId, 0);

    obj_assoc_entry_t new_association;

    new_association.vol_uuid = volId.get();
    new_association.ref_cnt = 0UL;
    new_association.vol_migration_reconcile_ref_cnt = -1L;
    obj_map.obj_refcnt = 0UL;
    obj_map.obj_migration_reconcile_ref_cnt = -1L;
    assoc_entry.push_back(new_association);
    obj_map.obj_num_assoc_entry = assoc_entry.size();

    setObjReconcileRequired();
}

fds_bool_t
ObjMetaData::isVolAssocReconciled(fds_uint64_t& totalVolRefCnt,
                                  fds_int64_t& totalReconcileVolRefCnt)
{
    bool reconciled = false;
    fds_uint64_t reconciledVols = 0UL;

    totalVolRefCnt = 0UL;
    totalReconcileVolRefCnt = 0L;

    std::vector<obj_assoc_entry_t>::iterator it;
    for (it = assoc_entry.begin(); it != assoc_entry.end(); ++it) {
        fds_assert((*it).vol_migration_reconcile_ref_cnt <= 0L);
        if ((*it).vol_migration_reconcile_ref_cnt == 0L) {
            ++reconciledVols;
        }
        totalReconcileVolRefCnt += (*it).vol_migration_reconcile_ref_cnt;
        totalVolRefCnt += (*it).ref_cnt;
    }

    // If all the reconcile ref cnt are 0, then there is nothing to reconcile.
    if (reconciledVols == (fds_uint64_t)assoc_entry.size()) {
        reconciled = true;
        // If all volumes are reconciled, then obj reconcile ref_cnt should
        // also be reconciled.
        fds_verify(obj_map.obj_migration_reconcile_ref_cnt == 0L);
    }

    return reconciled;
}

fds_bool_t
ObjMetaData::reconcileDelObjMetaData(ObjectID objId,
                                     fds_volid_t volId,
                                     fds_uint64_t ts)
{
    // Since the meta is dealing with DELETE operation and already in
    // ReconcileRequired state (i.e. DELETE deficit), it will continue
    // to remain this state until PUT operation takes all the refcnt >=0.
    fds_assert(isObjReconcileRequired());
    fds_assert(obj_map.obj_num_assoc_entry == assoc_entry.size());

    LOGMIGRATE << "Reconcile DELETE (before):"
               << " ObjID=" << objId
               << " VolID=" << volId
               << " MetaData=" << logString();

    std::vector<obj_assoc_entry_t>::iterator it;
    for (it = assoc_entry.begin(); it != assoc_entry.end(); ++it) {
        if (volId == fds_volid_t((*it).vol_uuid)) {
            break;
        }
    }

    // While is Reconcile state, DELETE was called on this object.  We
    if (it == assoc_entry.end()) {
        obj_assoc_entry_t newAssociation;
        newAssociation.vol_uuid = volId.get();
        newAssociation.ref_cnt = 0;
        newAssociation.vol_migration_reconcile_ref_cnt = -1L;
        assoc_entry.push_back(newAssociation);
    } else {
        if ((*it).ref_cnt > 0) {
            fds_assert((*it).vol_migration_reconcile_ref_cnt == 0L);
            (*it).ref_cnt--;
            if ((*it).ref_cnt == 0UL) {
                assoc_entry.erase(it);
            }
        } else {
            (*it).vol_migration_reconcile_ref_cnt--;
        }
    }
    obj_map.obj_num_assoc_entry = assoc_entry.size();

    // Now update, object ref cnt.
    if (obj_map.obj_refcnt > 0) {
        obj_map.obj_refcnt--;
        fds_assert(obj_map.obj_migration_reconcile_ref_cnt == 0L);
        if (obj_map.obj_refcnt == 0UL) {
            obj_map.obj_del_time = ts;
        }
    } else {
        obj_map.obj_migration_reconcile_ref_cnt--;
    }

#if DEBUG
    {
        fds_uint64_t totalVolRefCnt = 0UL;
        fds_int64_t totalReconcileVolRefCnt = 0L;
        fds_assert(!isVolAssocReconciled(totalVolRefCnt, totalReconcileVolRefCnt));
    }

#endif

    LOGMIGRATE << "Reconcile DELETE (before):"
               << " ObjID=" << objId
               << " VolID=" << volId
               << " MetaData=" << logString();

    return true;
}


/*  This is called in the PUT path.  The method reconciles a metadata
 *  with a PUT IO, and updates associated volume refcnt and object
 *  refcnt.  If all ref_cnts are reconciled, then reset the
 *  RECONCILE_REQUIRED flag.
 */
void
ObjMetaData::reconcilePutObjMetaData(ObjectID objId,
                                     fds_uint32_t objSize,
                                     fds_volid_t volId)
{
    LOGMIGRATE << "Reconcile PUT (before):"
               << " ObjID=" << objId
               << " VolID=" << volId
               << " objSize= " << objSize
               << " MetaData=" << logString();

    fds_assert(isObjReconcileRequired());
    fds_assert(obj_map.obj_num_assoc_entry == assoc_entry.size());
    std::vector<obj_assoc_entry_t>::iterator it;
    for (it = assoc_entry.begin(); it != assoc_entry.end(); ++it) {

        // If the volume is found, then reconcile that particular volume.
        if (volId == fds_volid_t((*it).vol_uuid)) {
            break;
        }
    }

    // New volume association.  Add a new entry.
    if (it == assoc_entry.end()) {
        obj_assoc_entry_t new_association;
        new_association.vol_uuid = volId.get();
        new_association.ref_cnt = 1L;
        new_association.vol_migration_reconcile_ref_cnt = 0L;

        assoc_entry.push_back(new_association);
    } else {
        // Have to check if the reconcile ref_cnt is < 0.
        // If < 0, then the volume requires reconcile, and the rest of the metadata
        // requires reconciliation.
        fds_assert((*it).vol_migration_reconcile_ref_cnt <= 0);
        if ((*it).vol_migration_reconcile_ref_cnt < 0L) {
            // This is PUT operation, so reconcile the
            (*it).vol_migration_reconcile_ref_cnt++;
            // If the vol_miration_reconcile_ref_cnt ever reaches =, then
            // remove the association entry.
            if ((*it).vol_migration_reconcile_ref_cnt == 0L) {
                fds_assert((*it).ref_cnt == 0L);
                assoc_entry.erase(it);
            }
        } else {
            // This volume doesn't require reconciliation, so add refcnt.
            (*it).ref_cnt++;
        }
    }

    // Set the new number of assoc entry to current assoc size.
    obj_map.obj_num_assoc_entry = assoc_entry.size();

    // If the object refcnt requires reconciliation, then adjust
    // accordingly.
    if (obj_map.obj_migration_reconcile_ref_cnt < 0L) {
        obj_map.obj_migration_reconcile_ref_cnt++;
        if (obj_map.obj_migration_reconcile_ref_cnt == 0L) {
            fds_assert(obj_map.obj_refcnt == 0L);
        }
    } else {
        // If ref_cnt requires no reconciliation, then adjust the object
        // refcnt.
        obj_map.obj_refcnt++;
    }

    // Check if the metadata is reconciled or not.
    {
        fds_uint64_t totalVolRefCnt = 0UL;
        fds_int64_t totalReconcileVolRefCnt = 0L;
        fds_bool_t volsReconciled = isVolAssocReconciled(totalVolRefCnt, totalReconcileVolRefCnt);

        fds_verify(totalVolRefCnt == obj_map.obj_refcnt);
        fds_verify(totalReconcileVolRefCnt == obj_map.obj_migration_reconcile_ref_cnt);

        if (volsReconciled) {
            unsetObjReconcileRequired();
        }
    }

    // if this is the first time we are going to put an object (object does not
    // physically exist yet), set correct object size
    if (!dataPhysicallyExists()) {
        fds_verify(obj_map.obj_size == 0);
        obj_map.obj_size = objSize;
    }

    LOGMIGRATE << "Reconcile PUT (after):"
               << " Reconciled=" << !isObjReconcileRequired()
               << " ObjID=" << objId
               << " VolID=" << volId
               << " MetaData=" << logString();
}


void
ObjMetaData::reconcileDeltaObjMetaData(const fpi::CtrlObjectMetaDataPropagate& objMetaData)
{
    // ondisk metadata may have RECONCILE flag on or off.  But, propagate msg must
    // have the reconcile flag on.
    fds_assert(fpi::OBJ_METADATA_RECONCILE != objMetaData.objectReconcileFlag);

    std::vector<obj_assoc_entry_t>::iterator it;
    for (auto volAssoc : objMetaData.objectVolumeAssoc) {
        it = getAssociationIt(fds_volid_t(volAssoc.volumeAssoc));

        if (assoc_entry.end() == it) {
            // new association.  no need to add.  Just add the association.
            fds_verify(volAssoc.volumeRefCnt > 0);
            obj_assoc_entry_t newAssociation;
            newAssociation.vol_uuid = volAssoc.volumeAssoc;
            newAssociation.ref_cnt = volAssoc.volumeRefCnt;
            newAssociation.vol_migration_reconcile_ref_cnt = 0L;
            assoc_entry.push_back(newAssociation);
        } else {
            // existing volume assoctiona.  Need to reconcile by merging
#if DEBUG
            if (it->ref_cnt > 0UL) {
                fds_assert(0L == it->vol_migration_reconcile_ref_cnt);
            }
            if (it->vol_migration_reconcile_ref_cnt < 0L) {
                fds_assert(0UL == it->ref_cnt);
            }
#endif
            int64_t newRefCnt;
            if (it->vol_migration_reconcile_ref_cnt < 0L) {
                newRefCnt = it->vol_migration_reconcile_ref_cnt + volAssoc.volumeRefCnt;
            } else {
                newRefCnt = it->ref_cnt + volAssoc.volumeRefCnt;
            }

            if (newRefCnt >= 0) {
                it->ref_cnt = newRefCnt;
                it->vol_migration_reconcile_ref_cnt = 0L;
                if (0L == newRefCnt) {
                    assoc_entry.erase(it);
                }
            } else {
                it->vol_migration_reconcile_ref_cnt = newRefCnt;
                fds_verify(0UL == it->ref_cnt);
            }
        }
        obj_map.obj_num_assoc_entry = assoc_entry.size();
    }

    int64_t newObjRefCnt;
    if (obj_map.obj_migration_reconcile_ref_cnt < 0L) {
        newObjRefCnt = obj_map.obj_migration_reconcile_ref_cnt +
                       objMetaData.objectRefCnt;

    }  else {
        newObjRefCnt = obj_map.obj_refcnt +
                       objMetaData.objectRefCnt;
    }

    if (newObjRefCnt >= 0) {
        obj_map.obj_refcnt = newObjRefCnt;
        obj_map.obj_migration_reconcile_ref_cnt = 0;
    } else {
        obj_map.obj_migration_reconcile_ref_cnt = newObjRefCnt;
        fds_verify(0UL == obj_map.obj_refcnt);
    }

    // Check if the metadata is reconciled or not.
    {
        fds_uint64_t totalVolRefCnt = 0UL;
        fds_int64_t totalReconcileVolRefCnt = 0L;
        fds_bool_t volsReconciled = isVolAssocReconciled(totalVolRefCnt, totalReconcileVolRefCnt);

        if (volsReconciled) {
            fds_verify(totalVolRefCnt == obj_map.obj_refcnt);
            fds_verify(totalReconcileVolRefCnt == obj_map.obj_migration_reconcile_ref_cnt);
            unsetObjReconcileRequired();
        }
    }

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
    /* Add assert for debug code only */
    fds_assert(0 == memcmp(obj_map.obj_id.metaDigest,
                           rhs.obj_map.obj_id.metaDigest,
                           sizeof(obj_map.obj_id.metaDigest)));

    /* If any of the field do not match, then return false */
    if ((0 != memcmp(obj_map.obj_id.metaDigest,
                     rhs.obj_map.obj_id.metaDigest,
                     sizeof(obj_map.obj_id.metaDigest))) ||
        (obj_map.obj_refcnt != rhs.obj_map.obj_refcnt) ||
        (obj_map.compress_type != rhs.obj_map.compress_type) ||
        (obj_map.compress_len != rhs.obj_map.compress_len) ||
        (obj_map.obj_blk_len != rhs.obj_map.obj_blk_len) ||
        (obj_map.obj_size != rhs.obj_map.obj_size) ||
        (obj_map.expire_time != rhs.obj_map.expire_time)) {

        LOGMIGRATE << "ObjectMetaData compare content failed: "
                   << " THIS: " << logString()
                   << " RHS: " << rhs.logString();
        return false;
    }

    /* if the volume association entry size is different, then
     * metadata is different.
     */
    if (assoc_entry.size() != rhs.assoc_entry.size()) {
        LOGMIGRATE << "ObjectMetaData assoc_entry size failed: "
                   << " THIS: " << logString()
                   << " RHS: " << rhs.logString();
        return false;
    } else {
        /* assoc_entry size is the same.  Just memcmp the vector data */
        if (0 != memcmp(assoc_entry.data(),
                        rhs.assoc_entry.data(),
                        sizeof(obj_assoc_entry_t) * assoc_entry.size())) {
            LOGMIGRATE << "ObjectMetaData assoc_entry contente failed: "
                       << " THIS: " << logString()
                       << " RHS: " << rhs.logString();
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

    fds_assert(meta_obj_map_magic_value == obj_map.obj_magic);

    oss << "id=" << obj_id
        << " refcnt=" << obj_map.obj_refcnt
        << " flags=" << std::hex << obj_map.obj_flags << std::dec
        << " compression_type=" << (uint32_t)obj_map.compress_type
        << " compress_len=" << obj_map.compress_len
        << " blk_len=" << obj_map.obj_blk_len
        << " len=" << obj_map.obj_size
        << " expire_time=" << obj_map.expire_time
        << " obj_migration_reconcile_dlt_ver=" << obj_map.obj_migration_reconcile_dlt_ver
        << " obj_migration_reconcile_ref_cnt=" << obj_map.obj_migration_reconcile_ref_cnt
        << " assoc_entry_cnt=" << assoc_entry.size()
        << " vol_id:refcnt=";
    for (auto entry : assoc_entry)  {
        oss << "(" << std::hex << entry.vol_uuid << std::dec
            << ":" << entry.ref_cnt
            << ":" << entry.vol_migration_reconcile_ref_cnt
            << "), ";
    }
    oss << std::endl;
    return oss.str();
}
}  // namespace fds
