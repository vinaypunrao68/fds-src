
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <ObjMeta.h>

namespace fds {

SyncMetaData::SyncMetaData()
{
    header = nullptr;
    assoc_entries = nullptr;
}

ObjMetaData::ObjMetaData()
{
    persistBuffer = nullptr;
    size = 0;

    mask = nullptr;
    obj_map = nullptr;
    phy_loc = nullptr;
    assoc_entry = nullptr;

}

ObjMetaData::~ObjMetaData()
{
    if (persistBuffer) {
        delete persistBuffer;
    }
}

uint32_t ObjMetaData::write(serialize::Serializer* serializer) const
{
    serializer->writeString(std::string(persistBuffer));
    return size;
}

uint32_t ObjMetaData::read(serialize::Deserializer* deserializer)
{
    std::string s;
    deserializer->readString(s);
    size = s.size();
    memcpy(persistBuffer, s.data(), s.size());

    adjustPointers_();

    return size;
}

uint32_t ObjMetaData::getEstimatedSize() const
{
    return size;
}

uint32_t ObjMetaData::serializeTo(ObjectBuf& buf) const
{
    bool ret = getSerialized(buf.data);
    fds_assert(ret == true);
    buf.size = buf.data.size();
    return buf.size;
}

bool ObjMetaData::deserializeFrom(const ObjectBuf& buf)
{
    bool ret = loadSerialized(buf.data);
    fds_assert(ret == true);
    return ret;
}

bool ObjMetaData::deserializeFrom(const leveldb::Slice& s)
{
    // TODO(Rao): Avoid the extra copy from s.ToString().  Have
    // leveldb expose underneath string
    bool ret = loadSerialized(s.ToString());
    fds_assert(ret == true);
    return ret;
}

uint64_t ObjMetaData::getModificationTs() const
{
    return obj_map->assoc_mod_time;
}

void ObjMetaData::extractSyncData(FDSP_MigrateObjectMetadata &md) const
{
    md.obj_len = static_cast<int32_t>(getObjSize());
    md.modification_ts = getModificationTs();
    if (obj_map->obj_refcnt == 0)  {
        return;
    }
    for(uint32_t i = 0; i < obj_map->obj_num_assoc_entry; i++) {
        if (assoc_entry[i].ref_cnt > 0) {
            fds_assert(assoc_entry[i].vol_uuid != 0);
            FDSP_ObjectVolumeAssociation a;
            a.vol_id = assoc_entry[i].vol_uuid;
            a.ref_cnt = assoc_entry[i].ref_cnt;
            md.associations.push_back(a);
        }
    }
}

void ObjMetaData::checkAndDemoteUnsyncedData(const uint64_t& syncTs)
{
    fds_assert(!"not impl");
        if (!syncDataExists() &&
            obj_map->assoc_mod_time != 0 &&
            obj_map->assoc_mod_time < syncTs) {
            std::ostringstream oss;

            serialize::Serializer*s = serialize::getMemSerializer(2*size);
            uint32_t sz = 0;
            sz += s->writeI64(obj_map->obj_create_time);
            sz += s->writeI64(obj_map->assoc_mod_time);
            sz += s->writeI64(obj_map->obj_num_assoc_entry);
            sz += s->writeString(std::string(assoc_entry,
                    sizeof(obj_assoc_entry_t) * obj_map->obj_num_assoc_entry));

            setSyncMetaData(meta_data);
            meta_data.clearAll();
            meta_data.locMap = sync_meta_data.locMap;
        }
}

void ObjMetaData::applySyncData(const FDSP_MigrateObjectMetadata& data)
{
    fds_assert(syncDataExists());

    fds_assert(!"not impl");
}

void ObjMetaData::mergeNewAndUnsyncedData()
{
    fds_assert(!"not impl");
}

bool ObjMetaData::objectExists()
{
    fds_assert(!"not impl");
    return true;
}

bool ObjMetaData::syncDataExists()
{
    return (*mask & SYNCMETADATA_MASK) != 0;
}
void ObjMetaData::adjustPointers_(bool init)
{
    fds_assert(size >= (sizeof(uint8_t) +
            sizeof(meta_obj_map_t) +
            sizeof(obj_assoc_entry_t)));

    uint32_t offset = 0;
    /* assign pointers */
    mask = (uint8_t*) (persistBuffer + offset);
    offset += sizeof(uint8_t);

    obj_map = (meta_obj_map_t*) (persistBuffer + offset);
    offset += sizeof(meta_obj_map_t);

    /* In the initialization case we start with atleast one association
     * entry
     */
    if (init) {
        obj_map->obj_num_assoc_entry = 1;
    }
    fds_assert(obj_map->obj_num_assoc_entry >= 1);

    phy_loc = &obj_map->loc_map[0];

    assoc_entry = (obj_assoc_entry_t*) (persistBuffer + offset);
    offset += (sizeof(obj_assoc_entry_t) * obj_map->obj_num_assoc_entry);

    sync_data.header = nullptr;
    sync_data.assoc_entries = nullptr;
    if (syncDataExists()) {
        sync_data.header = (SyncMetaData::header_t*) (persistBuffer + offset);
        offset += sizeof(SyncMetaData::header_t);
        if (sync_data.header->assoc_entry_cnt > 0) {
            sync_data.assoc_entries = (obj_assoc_entry_t*) (persistBuffer + offset);
            offset += (sizeof(obj_assoc_entry_t) * sync_data.header->assoc_entry_cnt);
        }
    }
    fds_assert(size == offset);
}

#if 0
void newPersistentBuffer(int num_assoc_entries, bool sync_entry,
        int num_sync_assoc_entries)
{
    /* Allocate buffer */
    size = sizeof(uint8_t) + sizeof(meta_obj_map_t);
    size += sizeof(obj_assoc_entry_t) * num_assoc_entries;
    if (sync_entry) {
        size += sizeof(SyncMetaData::header_t);
        size += sizeof(obj_assoc_entry_t) * num_sync_assoc_entries;
    }

    persistBuffer = new char[size];
    uint32_t offset = 0;

    /* assign pointers */
    mask = (uint8_t*) (persistBuffer + offset);
    offset += sizeof(uint8_t);

    obj_map = (meta_obj_map_t*) (persistBuffer + offset);
    offset += sizeof(meta_obj_map_t);

    assoc_entry = nullptr;
    if (num_assoc_entries > 0) {
        assoc_entry = (obj_assoc_entry_t*) (persistBuffer + offset);
        offset += (sizeof(obj_assoc_entry_t) * num_assoc_entries);
    }

    sync_data.header = nullptr;
    sync_data.assoc_entries = nullptr;
    if (sync_entry) {
        sync_data.header = (SyncMetaData::header_t*) (persistBuffer + offset);
        offset += sizeof(SyncMetaData::header_t);
        if (num_sync_assoc_entries > 0) {
            sync_data.assoc_entries = (obj_assoc_entry_t*) (persistBuffer + offset);
            offset += (sizeof(obj_assoc_entry_t) * num_sync_assoc_entries);
        }
    }

    fds_assert(size == offset);
}
#endif
}  // namespace fds
