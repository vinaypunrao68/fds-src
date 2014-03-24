
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <ObjMeta.h>

namespace fds {

uint32_t SyncMetaData::write(serialize::Serializer* serializer) const
{

}

uint32_t SyncMetaData::read(serialize::Deserializer* deserializer)
{

}
uint32_t SyncMetaData::getEstimatedSize() const
{

}

ObjMetaData::ObjMetaData()
{
}

ObjMetaData::~ObjMetaData()
{
}

uint32_t ObjMetaData::write(serialize::Serializer* serializer) const
{
    serializer->writeString(persistBuffer);
    return persistBuffer.size();
}

uint32_t ObjMetaData::read(serialize::Deserializer* deserializer)
{
    deserializer->readString(persistBuffer);

    uint8_t* buf = const_cast<uint8_t*>(persistBuffer.data());
    obj_map = (meta_obj_map_t *)buf;
    phy_loc = &obj_map->loc_map[0];
    assoc_entry = (obj_assoc_entry_t *)(buf + sizeof(meta_obj_map_t ));

    return persistBuffer.size();
}

uint32_t ObjMetaData::getEstimatedSize() const
{
    return persistBuffer.size();
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
    fds_assert(!"not impl");
}

void ObjMetaData::checkAndDemoteUnsyncedData(const uint64_t& syncTs)
{
    fds_assert(!"not impl");
}

bool ObjMetaData::syncMetadataExists()
{
    fds_assert(!"not impl");
    return false;
}

void ObjMetaData::applySyncData(const FDSP_MigrateObjectMetadata& data)
{
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
}  // namespace fds
