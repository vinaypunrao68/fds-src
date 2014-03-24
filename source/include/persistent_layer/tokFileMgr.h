#ifndef __TOK_FILE_MGR__
#define __TOK_FILE_MGR__
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

#define PL_TOKEN_MASK 0x000000ff

using namespace FDS_ProtocolInterface;
using namespace fds;
using namespace osm;
using namespace std;
using namespace diskio;

namespace fds {
extern ObjectStorMgr *objStorMgr;
typedef fds_uint32_t fds_token_id;




class tokenFileDb : public HasLogger { 
public:

    tokenFileDb( fds_log* _log) {
        SetLog(_log);
        tokenFileDbMutex = new fds_mutex("tokenFileDb Mutex");
    }
    ~tokenFileDb() {
        delete tokenFileDbMutex;
    }

    inline fds_token_id GetTokenFileDb(const fds_token_id &tokId) const
    {
        return tokId & PL_TOKEN_MASK;
    }

    FilePersisDataIO  *openTokenFile(fds_tier_t tier, fds_uint32_t disk_idx, fds_token_id tokId, fds_uint32_t fileId) {
        fds_token_id dbId = GetTokenFileDb(tokId);
        if (tier == disk_tier )  {
           std::string filename = disk_hdd_path[disk_idx] + "tokenFile_" + std::to_string(dbId) + "_" + std::to_string(fileId);
        } else {
           std::string filename = disk_ssd_path[disk_idx] + "tokenFile_" + std::to_string(dbId) + "_" + std::to_string(fileId);
        }
        FilePersisDataIO *fdesc = NULL;

        tokenFileDbMutex->lock();
        if ( (fdesc = tokenFileTbl[filename]) == NULL ) {
            fdesc  = new FilePersisDataIO(filename, disk_idx);
            tokenTbl[dbId] = fdesc;
        }
        tokenFileDbMutex->unlock();

        return fdesc;
    }

    void closeTokenFile(fds_uint32_t disk_idx, fds_token_id tokId, fds_uint32_t fileId) {
        fds_token_id dbId = GetTokenFileDb(tokId);
        FilePersisDataIO *fdesc = NULL;
        if (tier == disk_tier )  {
           std::string filename = disk_hdd_path[disk_idx] + "tokenFile_" + std::to_string(dbId) + "_" + std::to_string(fileId);
        } else {
           std::string filename = disk_ssd_path[disk_idx] + "tokenFile_" + std::to_string(dbId) + "_" + std::to_string(fileId);
        }

        tokenFileDbMutex->lock();
        if ( (fdesc = tokenTbl[filename]) == NULL ) {
            tokenFileDbMutex->unlock();
            delete fdesc;
            return;
        }
        tokenTbl[filename] = NULL;
        tokenFileDbMutex->unlock();
        delete objdb;
    }


    FilePersisDataIO *getTokenFile(fds_uint32_t disk_idx, fds_token_id tokId, fds_uint32_t fileId) {
        fds_token_id dbId = GetTokenFileDb(tokId);
        FilePersisDataIO *fdesc = NULL;
        if (tier == disk_tier )  {
           std::string filename = disk_hdd_path[disk_idx] + "tokenFile_" + std::to_string(dbId) + "_" + std::to_string(fileId);
        } else {
           std::string filename = disk_ssd_path[disk_idx] + "tokenFile_" + std::to_string(dbId) + "_" + std::to_string(fileId);
        }

        tokenFileDbMutex->lock();
        fdesc = tokenTbl[filename];
        tokenFileDbMutex->unlock();
        return fdesc;
    }

private:
    std::unordered_map<std::string, FilePersisDataIO *> tokenTbl;
    fds_mutex *tokenFileDbMutex;
};

}
#endif
