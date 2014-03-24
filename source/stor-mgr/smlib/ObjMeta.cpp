
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <ObjMeta.h>

namespace fds {

ObjMetaData::ObjMetaData()
{
}

ObjMetaData::~ObjMetaData()
{
}

uint32_t ObjMetaData::write(serialize::Serializer* serializer) const
{
    // todo(Rao): Impl
    fds_assert(!"Not impl");
    return 0;
}

uint32_t ObjMetaData::read(serialize::Deserializer* deserializer)
{
    // todo(Rao): Impl
    fds_assert(!"Not impl");
    return 0;
}

uint32_t ObjMetaData::getEstimatedSize() const
{
    fds_assert(!"Not impl");
    return 0;
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

}  // namespace fds
