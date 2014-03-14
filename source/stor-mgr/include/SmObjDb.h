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
        modification_ts_ = 0;
    }

    virtual ~SmObjMetadata()
    {
    }

    uint64_t get_modification_ts() const
    {
        return  modification_ts_;
    }


    size_t marshall(char *buf, const size_t &buf_sz) const
    {
        size_t copied_sz = 0;

        memcpy(&buf[copied_sz], &modification_ts_, sizeof(modification_ts_));
        copied_sz += sizeof(modification_ts_);

        const char *loc_map_buf = loc_map_.marshalling();
        size_t loc_map_buf_sz = loc_map_.marshalledSize();
        memcpy(&buf[copied_sz], loc_map_buf, loc_map_buf_sz);
        copied_sz += loc_map_buf_sz;

        return copied_sz;
    }

    size_t unmarshall(char *buf, const size_t &buf_sz)
    {
        size_t idx = 0;

        modification_ts_ = *(reinterpret_cast<uint64_t*>(&buf[idx]));
        idx += sizeof(modification_ts_);

        loc_map_.unmarshalling(&buf[idx], buf_sz-idx);
        // NOTE: Ideally, unmarshalling() returns # of bytes unmarshalled
        idx += loc_map_.marshalledSize();

        return idx;
    }

    size_t marshalledSize() const
    {
        return sizeof(modification_ts_) + loc_map_.marshalledSize();
    }

protected:
    /* NOTE: If you change the type here, it affects marshalling/unmarshalling code */
    uint64_t modification_ts_;
    diskio::MetaObjMap loc_map_;
};

#define SMOBJ_METADATA_MASK             0x1
#define SMOBJ_SYNC_METADATA_MASK        0x2

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
    SmObjMetadata getMetaData() {
        return meta_data;
    }
    SmObjMetadata getSyncMetaData() {
            return sync_meta_data;
    }
    void setMetaData(const SmObjMetadata& md) {
        data_mask |= SMOBJ_METADATA_MASK;
        meta_data = md;
    }
    void setSyncMetaData(const SmObjMetadata& md) {
        data_mask |= SMOBJ_SYNC_METADATA_MASK;
        sync_meta_data = md;
    }
    void removeMetaData() {
        data_mask &= ~SMOBJ_METADATA_MASK;
        DBG(memset(&meta_data, 0, sizeof(meta_data)));
    }
    void removeSyncMetaData() {
        data_mask &= ~SMOBJ_SYNC_METADATA_MASK;
        DBG(memset(&sync_meta_data, 0, sizeof(sync_meta_data)));
    }
    bool metadataExists() {
        return (data_mask & SMOBJ_METADATA_MASK) > 0;
    }
    bool syncMetadataExists() {
        return (data_mask & SMOBJ_SYNC_METADATA_MASK) > 0;
    }
    bool metadataOlderThan(const uint64_t &ts) {
        return ts >= meta_data.get_modification_ts();
    }
    void demoteMetadataToSyncMetadata() {
        fds_assert(!syncMetadataExists());
        setSyncMetaData(meta_data);
        removeMetaData();
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

    fds::Error Get(const ObjectID& obj_id, ObjectBuf& obj_buf);

    fds::Error Put(const ObjectID& obj_id, ObjectBuf& obj_buf);

    Error get(const View &view,
            const ObjectID& obj_id, SmObjMetadata& obj_buf);
    Error readObjectLocations(const View &view, const ObjectID &objId,
            diskio::MetaObjMap &objMaps);

    Error put(const ObjectID& obj_id, const SmObjMetadata& obj_buf);
    Error writeObjectLocation(const ObjectID& objId,
            meta_obj_map_t *obj_map,
            fds_bool_t      append);
    Error deleteObjectLocation(const ObjectID& objId);
    Error putSyncEntry(const ObjectID& obj_id,
            const SmObjMetadata& obj_buf);

    void  iterRetrieveObjects(const fds_token_id &token,
            const size_t &max_size,
            FDSP_MigrateObjectList &obj_list,
            SMTokenItr &itr);
private:
    std::unordered_map<fds_token_id, ObjectDB *> tokenTbl;
    std::string stor_prefix;
    fds_mutex *smObjDbMutex;
    ObjectStorMgr *objStorMgr;
};

}
#endif
