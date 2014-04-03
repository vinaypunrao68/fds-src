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

    std::string tokenFileDb::getFileName(fds_uint32_t disk_idx,
                                         fds_token_id tok_id,
                                         DataTier tier,
                                         fds_uint32_t file_id) const
    {
        fds_token_id dbId = tok_id & PL_TOKEN_MASK;
        std::string  filename;
        if (tier == diskTier)  {
            filename = dataDiscoveryMod.disk_hdd_path(disk_idx);
            filename = filename + "/tokenFile_" + std::to_string(dbId)
                    + "_" + std::to_string(file_id);
        } else {
            filename = dataDiscoveryMod.disk_ssd_path(disk_idx);
            filename = filename + "/tokenFile_" + std::to_string(dbId)
                    + "_" + std::to_string(file_id);
        }
        return filename;
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

        // TODO(WIN-289) if fileid == 0, get current file id for
        // <disk_idx, tokId, tier> tuple

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

    fds_uint32_t tokenFileDb::getWriteFileId(fds_uint32_t disk_idx,
                                             fds_token_id tokId,
                                             DataTier tier)
    {
        std::string key = getKeyString(disk_idx, tokId, tier);
        fds_uint32_t file_id = INVALID_FILE_ID;
        if (write_fileids.count(key) > 0) {
            file_id = write_fileids[key];
        }
        return file_id;
    }

    // either write new or re-set existing
    void tokenFileDb::setWriteFileId(fds_uint32_t disk_idx,
                                     fds_token_id tokId,
                                     DataTier tier,
                                     fds_uint32_t file_id)
    {
        std::string key = getKeyString(disk_idx, tokId, tier);
        write_fileids[key] = file_id;
    }

    fds::Error tokenFileDb::notifyStartGC(fds_uint32_t disk_idx,
                                          fds_token_id tok_id,
                                          DataTier tier) {
        fds::Error err(fds::ERR_OK);
        // TODO(WIN-289) get the next start file id. Check that there
        // must be no files that have this file id range (since we are
        // assigning start file ids by flipping one bit, we don't want
        // to keep the files that are from two garbage collections back
        return err;
    }

    fds::Error tokenFileDb::notifyEndGC(fds_uint32_t disk_idx,
                                        fds_token_id tok_id,
                                        DataTier tier) {
        fds::Error err(fds::ERR_OK);
        // TODO(WIN-289) remove all files that belong to old file ids
        // We know old file ids by getting current start file id, flipping
        // its bit and trying to delete all files:
        // old start file id ... old start file id + MAX_FILES_PER_TOKEN - 1
        return err;
    }

}  // namespace fds
