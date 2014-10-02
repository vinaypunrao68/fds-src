/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_SMOBJDB_H_
#define SOURCE_STOR_MGR_INCLUDE_SMOBJDB_H_

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <strings.h>
extern "C" {
#include <assert.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
}

#include <atomic>
#include <string>
#include <unordered_map>
#include <utility>

#include "fdsp/FDSP_types.h"
#include "fdsp/FDSP_DataPathReq.h"
#include "fdsp/FDSP_DataPathResp.h"
#include "fds_volume.h"
#include "fds_types.h"
#include "odb.h"
#include "util/Log.h"
#include "StorMgrVolumes.h"
#include "persistent-layer/dm_service.h"
#include "persistent-layer/dm_io.h"

#include "fds_qos.h"
#include "qos_ctrl.h"
#include "fds_obj_cache.h"
#include "fds_assert.h"
#include "fds_config.hpp"
#include "util/timeutils.h"

#include "ObjStats.h"
#include "ObjMeta.h"

#define SM_TOKEN_MASK 0x000000ff

namespace fds {

typedef fds_uint32_t fds_token_id;
class ObjMetaData;

struct OpCtx {
    enum OpType {
        GET,
        PUT,
        DELETE,
        RELOCATE,
        COPY,
        SYNC,
        GC_COPY,
        WRITE_BACK
    };
    explicit OpCtx(const OpType &t);
    OpCtx(const OpType &t, const uint64_t &timestamp);
    bool isClientIO() const;

    OpType type;
    uint64_t ts;
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

     SmObjDb(ObjectStorMgr *storMgr,
             std::string stor_prefix_,
             fds_log* _log);
     ~SmObjDb();

     inline fds_token_id GetSmObjDbId(const fds_token_id &tokId) const;

     osm::ObjectDB *openObjectDB(fds_token_id tokId);

     void closeObjectDB(fds_token_id tokId);

     osm::ObjectDB *getObjectDB(fds_token_id tokId);

     void snapshot(const fds_token_id& tokId,
                   leveldb::DB*& db, leveldb::ReadOptions& options);
     void lock(const ObjectID& objId);
     void unlock(const ObjectID& objId);

#if 0
     Error readObjectLocations(const View &view, const ObjectID &objId,
                               diskio::MetaObjMap &objMaps);

     Error writeObjectLocation(const ObjectID& objId,
                               meta_obj_map_t *obj_map,
                               fds_bool_t      append);

     Error deleteObjectLocation(const ObjectID& objId);
#endif

     Error putSyncEntry(const ObjectID& objId,
                        const FDSP_MigrateObjectMetadata& data,
                        bool &dataExists);
     Error resolveEntry(const ObjectID& objId);
     void  iterRetrieveObjects(const fds_token_id &token,
                               const size_t &max_size,
                               FDSP_MigrateObjectList &obj_list,
                               SMTokenItr &itr);

     bool dataPhysicallyExists(const ObjectID& obj_id);

     // TODO(Rao:) Make these private.  Exposed for mock testing

     Error get(const ObjectID& objId, ObjMetaData& md);
     Error put(const OpCtx &opCtx, const ObjectID& objId, ObjMetaData& md);
     Error remove(const ObjectID& objId);

     static Error get_from_snapshot(leveldb::Iterator* itr, ObjMetaData& md);

    private:
     inline fds_token_id getTokenId_(const ObjectID& objId);
     inline osm::ObjectDB* getObjectDB_(const fds_token_id& tokId);
     Error put_(const ObjectID& objId, const ObjMetaData& md);
     inline bool isTokenInSyncMode_(const fds_token_id& tokId);

     std::unordered_map<fds_token_id, osm::ObjectDB *> tokenTbl;
     using TokenTblIter = std::unordered_map<fds_token_id, osm::ObjectDB *>::const_iterator;
     std::string stor_prefix;
     fds_mutex *smObjDbMutex;
     ObjectStorMgr *objStorMgr;

     friend class ObjectStorMgr;
     friend class MObjStore;
};

}  // namespace fds
#endif  // SOURCE_STOR_MGR_INCLUDE_SMOBJDB_H_
