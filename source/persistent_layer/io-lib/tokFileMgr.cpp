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
                                         fds_uint16_t file_id) const
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
                                                 fds_uint16_t fileId)
    {
        FilePersisDataIO *fdesc = NULL;
        std::string  filename;
        fds_uint16_t file_id = fileId;

        fds_verify(file_id != INVALID_FILE_ID);

        tokenFileDbMutex->lock();
        if (file_id == WRITE_FILE_ID) {
            file_id = getWriteFileId(disk_idx, tokId, tier);
        }
        filename = getFileName(disk_idx, tokId, tier, file_id);

        if ((fdesc = tokenFileTbl[filename]) == NULL) {
            fdesc  = new FilePersisDataIO(filename.c_str(), file_id, disk_idx);
            tokenFileTbl[filename] = fdesc;
        }
        tokenFileDbMutex->unlock();

        return fdesc;
    }

    void tokenFileDb::closeTokenFile(fds_uint32_t disk_idx,
                                     fds_token_id tokId,
                                     DataTier tier,
                                     fds_uint16_t fileId)
    {
        FilePersisDataIO *fdesc = NULL;
        std::string  filename = NULL;

        // TODO(xxx) should we allow close the current file we
        // write to? if so, then we should getWriteFileId and remove
        // this fds_verify
        fds_verify(fileId != WRITE_FILE_ID);
        fds_verify(fileId != INVALID_FILE_ID);

        filename = getFileName(disk_idx, tokId, tier, fileId);

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
                                                fds_uint16_t fileId)
    {
        FilePersisDataIO *fdesc = NULL;
        std::string  filename;
        fds_uint16_t file_id = fileId;

        fds_verify(file_id != INVALID_FILE_ID);

        tokenFileDbMutex->lock();
        if (file_id == WRITE_FILE_ID) {
            file_id = getWriteFileId(disk_idx, tokId, tier);
        }
        filename = getFileName(disk_idx, tokId, tier, file_id);

        fdesc = tokenFileTbl[filename];
        tokenFileDbMutex->unlock();
        return fdesc;
    }

    fds_uint16_t tokenFileDb::getWriteFileId(fds_uint32_t disk_idx,
                                             fds_token_id tokId,
                                             DataTier tier)
    {
        std::string key = getKeyString(disk_idx, tokId & PL_TOKEN_MASK, tier);
        fds_uint16_t file_id = INVALID_FILE_ID;
        if (write_fileids.count(key) > 0) {
            file_id = write_fileids[key];
        } else {
            // otherwise set initial file id
            file_id = INIT_FILE_ID;
            write_fileids[key] = file_id;
        }
        return file_id;
    }

    // either write new or re-set existing
    void tokenFileDb::setWriteFileId(fds_uint32_t disk_idx,
                                     fds_token_id tokId,
                                     DataTier tier,
                                     fds_uint16_t file_id)
    {
        std::string key = getKeyString(disk_idx, tokId & PL_TOKEN_MASK, tier);
        write_fileids[key] = file_id;
    }

    fds::Error tokenFileDb::notifyStartGC(fds_uint32_t disk_idx,
                                          fds_token_id tok_id,
                                          DataTier tier) {
        fds::Error err(fds::ERR_OK);
        std::string key = getKeyString(disk_idx, tok_id & PL_TOKEN_MASK, tier);
        fds_uint16_t cur_file_id = INVALID_FILE_ID;
        fds_uint16_t new_file_id = INVALID_FILE_ID;

        tokenFileDbMutex->lock();
        if (write_fileids.count(key) > 0) {
            cur_file_id = write_fileids[key];
        } else {
            tokenFileDbMutex->unlock();
            return fds::Error(fds::ERR_NOT_FOUND);
        }

        // calculate and set new file id (file we will start writing
	// and to which non-garbage data will be copied)
	new_file_id = getShadowFileId(cur_file_id);
	write_fileids[key] = new_file_id;

        tokenFileDbMutex->unlock();

        // we are not creating a new token file here, it will be created
        // on the next write to this disk,tok,tier. We are keeping old file
	// until notifyEndGC() is called.

	// check we are in the consistent state
        std::string cur_filename = getFileName(disk_idx, tok_id, tier, cur_file_id);
        std::string new_filename = getFileName(disk_idx, tok_id, tier, new_file_id);
        fds_verify(tokenFileTbl.count(cur_filename) > 0);
        // if asserts here, most likely didn't finish last GC properly
        fds_verify(tokenFileTbl.count(new_filename) == 0);

        return err;
    }

    fds::Error tokenFileDb::notifyEndGC(fds_uint32_t disk_idx,
                                        fds_token_id tok_id,
                                        DataTier tier) {
        fds::Error err(fds::ERR_OK);
        std::string key = getKeyString(disk_idx, tok_id & PL_TOKEN_MASK, tier);
        fds_uint16_t shadow_file_id = INVALID_FILE_ID;
        fds_uint16_t old_file_id = INVALID_FILE_ID;
	FilePersisDataIO *del_fdesc = NULL;

        tokenFileDbMutex->lock();
        if (write_fileids.count(key) > 0) {
            shadow_file_id = write_fileids[key];
        } else {
            tokenFileDbMutex->unlock();
            return fds::Error(fds::ERR_NOT_FOUND);
        }

	// old_file_id is the same as next shadow file id
	old_file_id = getShadowFileId(shadow_file_id);

	// remove file with old_file_id (garbage collect)
	// it is up to the TokenCompactor to check that it copied all
	// non-garbage data to the shadow file
	// TODO(xxx) update below code when supporting multiple files
        std::string old_filename = getFileName(disk_idx, tok_id, tier, old_file_id);
	if (tokenFileTbl.count(old_filename)) {
	  // get the file to delete and remove token file entry
	  del_fdesc = tokenFileTbl[old_filename];
          if (del_fdesc) {
              err = del_fdesc->delete_file();
              delete del_fdesc;
              del_fdesc = NULL;
          }
	  tokenFileTbl[old_filename] = NULL;
	  tokenFileTbl.erase(old_filename);
	}
        tokenFileDbMutex->unlock();

        return err;
    }

}  // namespace fds
