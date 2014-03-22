#ifndef __SM_OBJ_DB__
#define __SM_OBJ_DB__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fdsp/FDSP_types.h>
#include <fdsp/FDSP_DataPathReq.h>
#include <fdsp/FDSP_DataPathResp.h>
#include "stor_mgr_err.h"
#include <fds_volume.h>
#include <fds_types.h>
#include "ObjLoc.h"
#include <odb.h>
#include <unistd.h>
#include <assert.h>
#include <iostream>
#include <util/Log.h>
#include "StorMgrVolumes.h"
#include <persistent_layer/dm_service.h>
#include <persistent_layer/dm_io.h>

#include <fds_qos.h>
#include <qos_ctrl.h>
#include <fds_obj_cache.h>
#include <fds_assert.h>
#include <fds_config.hpp>
#include <fds_stopwatch.h>

#include <utility>
#include <atomic>
#include <unordered_map>
#include <ObjStats.h>

#define SM_TOKEN_MASK 0x000000ff

using namespace FDS_ProtocolInterface;
using namespace fds;
using namespace osm;
using namespace std;
using namespace diskio;

namespace fds {

typedef fds_uint32_t fds_token_id;


class SmObjMetadata
{
public:
    SmObjMetadata()
    {
        clearAll();
    }

    virtual ~SmObjMetadata()
    {
    }

    void clearAll() {
        modificationTs = 0;
        locMap.clear();
    }

    size_t marshall(char *buf, const size_t &buf_sz) const
    {
        size_t copied_sz = 0;

        memcpy(&buf[copied_sz], &modificationTs, sizeof(modificationTs));
        copied_sz += sizeof(modificationTs);

        copied_sz += locMap.marshall(&buf[copied_sz], buf_sz-copied_sz);

        return copied_sz;
    }

    size_t unmarshall(char *buf, const size_t &buf_sz)
    {
        size_t idx = 0;

        modificationTs = *(reinterpret_cast<uint64_t*>(&buf[idx]));
        idx += sizeof(modificationTs);

        idx += locMap.unmarshall(&buf[idx], buf_sz - idx);

        return idx;
    }

    size_t marshalledSize() const
    {
        return sizeof(modificationTs) + locMap.marshalledSize();
    }
    /*bool operator==(const SmObjMetadata& md) const
    {
        return modificationTs == md.modificationTs && locMap == md.locMap;
    }*/

    /* NOTE: If you change the type here, it affects marshalling/unmarshalling code */
    uint64_t modificationTs;
    diskio::MetaObjMap locMap;
};


#define SMOBJ_SYNC_METADATA_MASK        0x1

/*
 * Object meta data that is persisted on disk
 * NOTE:  This can be packed for saving space
 */
class OnDiskSmObjMetadata {
public:
    OnDiskSmObjMetadata() {
        memset(this, 0, sizeof(OnDiskSmObjMetadata));
        version = 1;
    }

    void writeObjectLocation(bool append, meta_obj_map_t *obj_map)
    {
        if (append == true) {
            meta_data.locMap.updateMap(*obj_map);
        } else {
            meta_data.locMap.clear();
            meta_data.locMap.updateMap(*obj_map);
        }
    }

    void readObjectLocations(diskio::MetaObjMap &objMaps)
    {
        objMaps = meta_data.locMap;
    }

    void deleteObjectLocation() {
        // TODO(Rao):  Implement this
    }

    SmObjMetadata* getSyncMetaDataP() {
            return &sync_meta_data;
    }

    uint64_t getModificationTs() const {
        return meta_data.modificationTs;
    }

    void setModificationTs(const uint64_t &ts) {
        meta_data.modificationTs = ts;
    }

    void applySyncData(const FDSP_MigrateObjectMetadata& data) {
        // TODO(Rao): impl
    }
    void extractSyncData(FDSP_MigrateObjectMetadata& data) {
        // TODO(Rao): impl
    }

    void setSyncMetaData(const SmObjMetadata& md) {
        data_mask |= SMOBJ_SYNC_METADATA_MASK;
        sync_meta_data = md;
    }

    void removeSyncMetaData() {
        data_mask &= ~SMOBJ_SYNC_METADATA_MASK;
        DBG(memset(&sync_meta_data, 0, sizeof(sync_meta_data)));
    }

    bool syncMetadataExists() const {
        return (data_mask & SMOBJ_SYNC_METADATA_MASK) > 0;
    }

    void checkAndDemoteUnsyncedData(const uint64_t &syncTs) {
        if (!syncMetadataExists() &&
            meta_data.modificationTs != 0 &&
            meta_data.modificationTs < syncTs) {
            setSyncMetaData(meta_data);
            meta_data.clearAll();
            meta_data.locMap = sync_meta_data.locMap;
        }
    }

    void mergeNewAndUnsyncedData() {
        // TODO(Rao):
    }

    size_t marshall(char *buf, const size_t &buf_sz) const
    {
        size_t copied_sz = 0;

        memcpy(&buf[copied_sz], &version, sizeof(version));
        copied_sz += sizeof(version);

        memcpy(&buf[copied_sz], &data_mask, sizeof(data_mask));
        copied_sz += sizeof(data_mask);

        copied_sz += meta_data.marshall(&buf[copied_sz], buf_sz-copied_sz);
        copied_sz += sync_meta_data.marshall(&buf[copied_sz], buf_sz-copied_sz);

        fds_assert(copied_sz <= buf_sz);
        return copied_sz;
    }

    size_t unmarshall(char *buf, const size_t &buf_sz)
    {
        size_t idx = 0;

        version = *(reinterpret_cast<uint8_t*>(&buf[idx]));
        idx += sizeof(version);

        data_mask = *(reinterpret_cast<uint8_t*>(&buf[idx]));
        idx += sizeof(data_mask);

        idx += meta_data.unmarshall(&buf[idx], buf_sz - idx);
        idx += sync_meta_data.unmarshall(&buf[idx], buf_sz - idx);

        fds_assert(idx == buf_sz);
        return idx;
    }

    size_t marshalledSize() const
    {
        return sizeof(version) + sizeof(data_mask) +
                meta_data.marshalledSize() + sync_meta_data.marshalledSize();
    }

    size_t unmarshall(const leveldb::Slice& s)
    {
        size_t sz = this->unmarshall(const_cast<char*>(s.data()), s.size());
        fds_assert(sz == s.size());
        return sz;
    }

    bool operator==(const OnDiskSmObjMetadata& md) const
    {
        return (version == md.version &&
                data_mask == md.data_mask/* &&
                meta_data == md.meta_data &&
                (!syncMetadataExists() || (sync_meta_data == md.sync_meta_data)) */);
    }

public:
    /* NOTE: If you change the type here, it affects marshalling/unmarshalling code */
    /* Current version */
    uint8_t version;
    /* Data mask to indicate which of the metadata is valid */
    uint8_t data_mask;
    /* Current metadata for client io */
    SmObjMetadata meta_data;
    /* Sync related metadata */
    SmObjMetadata sync_meta_data;
};

/**
 * Stores storage manages object metadata
 * Present implementation is based on level db
 */
class SmObjDb : public HasLogger { 
public:
    /* Signifies the view of metadata to operate on */
    enum View {
        /* Metadata served is not merged with sync entry.
         * We typically want this view for modification style (Put, Delete)
         * requests
         */
        NON_SYNC_MERGED,

        /* The metadata served is merged with sync entry.
         * We typically want this view for Get object type requests
         */
        SYNC_MERGED
    };
public:

    SmObjDb(ObjectStorMgr *storMgr,
            std::string stor_prefix_,
            fds_log* _log);
    ~SmObjDb();

    inline fds_token_id GetSmObjDbId(const fds_token_id &tokId) const;

    ObjectDB *openObjectDB(fds_token_id tokId);

    void closeObjectDB(fds_token_id tokId);

    ObjectDB *getObjectDB(fds_token_id tokId);

    void snapshot(const fds_token_id& tokId,
            leveldb::DB*& db, leveldb::ReadOptions& options);

    fds::Error Get(const ObjectID& obj_id, ObjectBuf& obj_buf);

    fds::Error Put(const ObjectID& obj_id, ObjectBuf& obj_buf);

    Error readObjectLocations(const View &view, const ObjectID &objId,
            diskio::MetaObjMap &objMaps);

    Error writeObjectLocation(const ObjectID& objId,
            meta_obj_map_t *obj_map,
            fds_bool_t      append);

    Error deleteObjectLocation(const ObjectID& objId);

    Error putSyncEntry(const ObjectID& objId,
            const FDSP_MigrateObjectMetadata& data);
    Error resolveEntry(const ObjectID& objId);
    void  iterRetrieveObjects(const fds_token_id &token,
            const size_t &max_size,
            FDSP_MigrateObjectList &obj_list,
            SMTokenItr &itr);

    // TODO(Rao:) Make these private.  Exposed for mock testing
    Error get_(const View &view,
            const ObjectID& objId, OnDiskSmObjMetadata& md);
    Error put_(const ObjectID& objId, const OnDiskSmObjMetadata& md);

    static Error get_from_snapshot(leveldb::Iterator* itr,
            OnDiskSmObjMetadata& md);
private:
    inline fds_token_id getTokenId_(const ObjectID& objId);
    inline ObjectDB* getObjectDB_(const fds_token_id& tokId);
    inline bool isTokenInSyncMode_(const fds_token_id& tokId);

    std::unordered_map<fds_token_id, ObjectDB *> tokenTbl;
    std::string stor_prefix;
    fds_mutex *smObjDbMutex;
    ObjectStorMgr *objStorMgr;
};

}
#endif
