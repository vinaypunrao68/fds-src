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
extern ObjectStorMgr *objStorMgr;
typedef fds_uint32_t fds_token_id;


/* Metadata that is persisted on disk */
// TODO: Currently deriving from objectBuf.  For perf reasons we should
// refactor to either hold a shared pointer to ObjectBuf or unmarshall and
// have our own private data
class SmObjMetadata : public ObjectBuf
{
public:
    SmObjMetadata()
    {
        modification_ts_ = 0;
    }
    explicit SmObjMetadata(const std::string &str)
    : ObjectBuf(str) {
        modification_ts_ = 0;
    }
    explicit SmObjMetadata(const ObjectBuf& buf)
    : ObjectBuf(buf)
    {
        modification_ts_ = 0;
    }
    virtual ~SmObjMetadata()
    {
    }

    uint64_t get_modification_ts() const;

    void unmarshall() {
        // TODO(Rao): Impl
        fds_assert(!"Not impl");
    }

    const char* buf() const {
        return data.data();
    }

    size_t len() const {
        return data.length();
    }

protected:
    /* Unmarshalled fields */
    uint64_t modification_ts_;
};

class SmObjDb : public HasLogger { 
public:

    SmObjDb(std::string stor_prefix_,
            fds_log* _log) {
        SetLog(_log);
        stor_prefix = stor_prefix_;
        smObjDbMutex = new fds_mutex("SmObjDb Mutex");
    }
    ~SmObjDb() {
        delete smObjDbMutex;
    }

    inline fds_token_id GetSmObjDbId(const fds_token_id &tokId) const
    {
        return tokId & SM_TOKEN_MASK;
    }

    ObjectDB *openObjectDB(fds_token_id tokId) {
        ObjectDB *objdb = NULL;
        fds_token_id dbId = GetSmObjDbId(tokId);

        smObjDbMutex->lock();
        if ( (objdb = tokenTbl[dbId]) == NULL ) {
            // Create leveldb
            std::string filename= stor_prefix + "SNodeObjIndex_" + std::to_string(dbId);
            objdb  = new ObjectDB(filename);
            tokenTbl[dbId] = objdb;
        }
        smObjDbMutex->unlock();

        return objdb;
    }

    void closeObjectDB(fds_token_id tokId) {
        fds_token_id dbId = GetSmObjDbId(tokId);
        ObjectDB *objdb = NULL;

        smObjDbMutex->lock();
        if ( (objdb = tokenTbl[dbId]) == NULL ) {
            smObjDbMutex->unlock();
            return;
        }
        tokenTbl[dbId] = NULL;
        smObjDbMutex->unlock();
        delete objdb;
    }


    ObjectDB *getObjectDB(fds_token_id tokId) {
        ObjectDB *objdb = NULL;
        fds_token_id dbId = GetSmObjDbId(tokId);

        smObjDbMutex->lock();
        objdb = tokenTbl[dbId];
        smObjDbMutex->unlock();
        return objdb;
    }

    fds::Error Get(const ObjectID& obj_id, ObjectBuf& obj_buf);

    fds::Error Put(const ObjectID& obj_id, ObjectBuf& obj_buf);

    void  iterRetrieveObjects(const fds_token_id &token,
            const size_t &max_size,
            FDSP_MigrateObjectList &obj_list,
            SMTokenItr &itr);
private:
    std::unordered_map<fds_token_id, ObjectDB *> tokenTbl;
    std::string stor_prefix;
    fds_mutex *smObjDbMutex;
};

}
#endif
