/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <stdlib.h>
#include <string.h>
#include <concurrency/Mutex.h>
#include <fds_volume.h>
#include <odb.h>
#include <assert.h>
#include <util/Log.h>
#include <fds_assert.h>
#include <tokFileMgr.h>

#define PL_TOKEN_MASK 0x000000ff

namespace diskio {

    tokenFileDb::tokenFileDb() {
        tokenFileDbMutex = new fds::fds_mutex("tokenFileDb Mutex");
    }
    tokenFileDb::~tokenFileDb() {
        delete tokenFileDbMutex;
    }

    fds_token_id tokenFileDb::GetTokenFileDb(const fds_token_id &tokId) 
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
