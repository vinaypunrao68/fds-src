/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <concurrency/Mutex.h>
#include <fdsp/FDSP_types.h>
#include <fdsp/FDSP_DataPathReq.h>
#include <fdsp/FDSP_DataPathResp.h>
#include <fds_volume.h>
#include <fds_types.h>
#include <odb.h>
#include <unistd.h>
#include <assert.h>
#include <iostream>
#include <util/Log.h>
#include <persistentdata.h>
#include <persistent_layer/dm_service.h>
#include <persistent_layer/dm_io.h>

#include <fds_assert.h>
#include <fds_config.hpp>
#include <fds_stopwatch.h>
#include <tokFileMgr.h>

#include <utility>
#include <atomic>
#include <unordered_map>

#define PL_TOKEN_MASK 0x000000ff

namespace diskio {
typedef fds_uint32_t fds_token_id;

    tokenFileDb::tokenFileDb() {
        tokenFileDbMutex = new fds::fds_mutex("tokenFileDb Mutex");
    }
    tokenFileDb::~tokenFileDb() {
        delete tokenFileDbMutex;
    }

    inline fds_token_id tokenFileDb::GetTokenFileDb(const fds_token_id &tokId) 
    {
        return tokId & PL_TOKEN_MASK;
    }

    FilePersisDataIO  *tokenFileDb::openTokenFile(DataTier tier,
                                                 fds_uint32_t disk_idx,
                                                 fds_token_id tokId,
                                                 fds_uint32_t fileId) {
        fds_token_id dbId = GetTokenFileDb(tokId);
        FilePersisDataIO *fdesc = NULL;
        std::string  filename;
        if (tier == diskTier)  {
           filename = dataDiscoveryMod.disk_hdd_path(disk_idx);
           filename = filename + "/tokenFile_" + std::to_string(dbId)
                                   + "_" + std::to_string(fileId);
        } else {
           filename = dataDiscoveryMod.disk_ssd_path(disk_idx);
           filename = filename + "/tokenFile_" + std::to_string(dbId)
                                  + "_" + std::to_string(fileId);
        }

        tokenFileDbMutex->lock();
        if ((fdesc = tokenFileTbl[filename]) == NULL) {
            fdesc  = new FilePersisDataIO(filename.c_str(), disk_idx);
            tokenFileTbl[filename] = fdesc;
        }
        tokenFileDbMutex->unlock();

        return fdesc;
    }

    void tokenFileDb::closeTokenFile(fds_uint32_t disk_idx,
                                     fds_token_id tokId,
                                     DataTier tier,
                                     fds_uint32_t fileId) {
        fds_token_id dbId = GetTokenFileDb(tokId);
        FilePersisDataIO *fdesc = NULL;
        std::string  filename;
        if (tier == diskTier)  {
           filename = dataDiscoveryMod.disk_hdd_path(disk_idx);
           filename = filename + "/tokenFile_" + std::to_string(dbId) +
                                   "_" + std::to_string(fileId);
        } else {
           filename = dataDiscoveryMod.disk_ssd_path(disk_idx);
           filename = filename + "/tokenFile_" + std::to_string(dbId)
                                  + "_" + std::to_string(fileId);
        }

        tokenFileDbMutex->lock();
        if ((fdesc = tokenFileTbl[filename]) == NULL ) {
            tokenFileDbMutex->unlock();
            return;
        }
        tokenFileTbl[filename] = NULL;
        tokenFileDbMutex->unlock();
        //delete fdesc;
    }


    FilePersisDataIO *tokenFileDb::getTokenFile(fds_uint32_t disk_idx,
                                                fds_token_id tokId,
                                                 DataTier tier,
                                                fds_uint32_t fileId) {
        fds_token_id dbId = GetTokenFileDb(tokId);
        FilePersisDataIO *fdesc = NULL;
        std::string  filename;
        if (tier == diskTier)  {
           filename = dataDiscoveryMod.disk_hdd_path(disk_idx);
           filename = filename  + "/tokenFile_" + std::to_string(dbId) +
                                   "_" + std::to_string(fileId);
        } else {
           filename = dataDiscoveryMod.disk_ssd_path(disk_idx);
           filename = filename + "/tokenFile_" + std::to_string(dbId)
                                  + "_" + std::to_string(fileId);
        }

        tokenFileDbMutex->lock();
        fdesc = tokenFileTbl[filename];
        tokenFileDbMutex->unlock();
        return fdesc;
    }
}  // namespace fds
