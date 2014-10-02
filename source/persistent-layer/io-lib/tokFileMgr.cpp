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
#include <persistent-layer/tokFileMgr.h>

#define PL_TOKEN_MASK 0x000000ff

namespace diskio {

    tokenFileDb::tokenFileDb()
            : tokenFileDbMutex("tokenFileDb Mutex") {
    }
    tokenFileDb::~tokenFileDb() {
        std::unordered_map<std::string, FilePersisDataIO *>::iterator it;
        FilePersisDataIO *del_fdesc = NULL;

        for (it = tokenFileTbl.begin(); it != tokenFileTbl.end(); ++it) {
            del_fdesc = it->second;
            if (del_fdesc) {
                delete del_fdesc;
            }
        }
        tokenFileTbl.clear();
    }

    fds_token_id tokenFileDb::getTokenId(meta_obj_id_t const *const oid)
    {
        fds_token_id tokId = oid->metaDigest[0];
        return tokId & PL_TOKEN_MASK;
    }

    void tokenFileDb::getTokenRange(fds_token_id* start_tok, fds_token_id* end_tok)
    {
        *start_tok = 0;
        *end_tok = PL_TOKEN_MASK;
    }

    std::string tokenFileDb::getFileName(fds_uint16_t disk_id,
                                         fds_token_id tok_id,
                                         DataTier tier,
                                         fds_uint16_t file_id) const
    {
        std::string  filename;
        if (tier == diskTier)  {
            filename = dataDiscoveryMod.disk_hdd_path(disk_id);
            filename = filename + "/tokenFile_" + std::to_string(tok_id)
                    + "_" + std::to_string(file_id);
        } else {
            filename = dataDiscoveryMod.disk_ssd_path(disk_id);
            filename = filename + "/tokenFile_" + std::to_string(tok_id)
                    + "_" + std::to_string(file_id);
        }
        return filename;
    }

    FilePersisDataIO  *tokenFileDb::openTokenFile(DataTier tier,
                                                 fds_uint16_t disk_id,
                                                 fds_token_id tokId,
                                                 fds_uint16_t fileId)
    {
        FilePersisDataIO *fdesc = NULL;
        std::string  filename;
        fds_uint16_t file_id = fileId;

        fds_verify(file_id != INVALID_FILE_ID);

        fds::fds_mutex::scoped_lock l(tokenFileDbMutex);
        if (file_id == WRITE_FILE_ID) {
            file_id = getWriteFileId(disk_id, tokId, tier);
        }
        filename = getFileName(disk_id, tokId, tier, file_id);

        if ((fdesc = tokenFileTbl[filename]) == NULL) {
            fdesc  = new FilePersisDataIO(filename.c_str(), file_id, disk_id);
            tokenFileTbl[filename] = fdesc;
        }
        return fdesc;
    }

    void tokenFileDb::closeTokenFile(fds_uint16_t disk_id,
                                     fds_token_id tokId,
                                     DataTier tier,
                                     fds_uint16_t fileId)
    {
        FilePersisDataIO *del_fdesc = NULL;
        std::string  filename = NULL;

        // TODO(xxx) should we allow close the current file we
        // write to? if so, then we should getWriteFileId and remove
        // this fds_verify
        fds_verify(fileId != WRITE_FILE_ID);
        fds_verify(fileId != INVALID_FILE_ID);

        filename = getFileName(disk_id, tokId, tier, fileId);

        fds::fds_mutex::scoped_lock l(tokenFileDbMutex);
        if (tokenFileTbl.count(filename) > 0) {
            del_fdesc = tokenFileTbl[filename];
            if (del_fdesc) {
                delete del_fdesc;
                del_fdesc = NULL;
            }
            tokenFileTbl[filename] = NULL;
            tokenFileTbl.erase(filename);
        }
    }


    FilePersisDataIO *tokenFileDb::getTokenFile(fds_uint16_t disk_id,
                                                fds_token_id tokId,
                                                 DataTier tier,
                                                fds_uint16_t fileId)
    {
        FilePersisDataIO *fdesc = NULL;
        std::string  filename;
        fds_uint16_t file_id = fileId;

        fds_verify(file_id != INVALID_FILE_ID);

        fds::fds_mutex::scoped_lock l(tokenFileDbMutex);
        if (file_id == WRITE_FILE_ID) {
            file_id = getWriteFileId(disk_id, tokId, tier);
        }
        filename = getFileName(disk_id, tokId, tier, file_id);

        fdesc = tokenFileTbl[filename];
        return fdesc;
    }

    fds_bool_t tokenFileDb::getTokenStats(DataTier tier,
                                          fds_uint16_t disk_id,
                                          fds_token_id tokId,
                                          TokenStat* ret_stat)
    {
        FilePersisDataIO *fdesc = NULL;
        // for now we have two possible filenames
        // TODO(xxx) this must change if we start using a sequence of files
        std::string fn1 = getFileName(disk_id, tokId, tier, INIT_FILE_ID);
        std::string fn2 = getFileName(disk_id, tokId, tier,
                                      getShadowFileId(INIT_FILE_ID));

        fds::fds_mutex::scoped_lock l(tokenFileDbMutex);
        if (tokenFileTbl.count(fn1) > 0) {
            fdesc = tokenFileTbl[fn1];
        } else if (tokenFileTbl.count(fn2) > 0) {
            fdesc = tokenFileTbl[fn2];
        } else {
            return false;  // not token file
        }
        fds_verify(fdesc != NULL);

        // fill in token stat that we return
        (*ret_stat).tkn_id = tokId;
        (*ret_stat).tkn_tot_size = fdesc->get_total_bytes();
        (*ret_stat).tkn_reclaim_size = fdesc->get_deleted_bytes();
        return true;
    }

    fds_uint16_t tokenFileDb::getWriteFileId(fds_uint16_t disk_id,
                                             fds_token_id tokId,
                                             DataTier tier)
    {
        std::string key = getKeyString(disk_id, tokId, tier);
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

    fds_bool_t tokenFileDb::isShadowFileId(fds_uint16_t file_id,
                                           fds_uint16_t disk_id,
                                           fds_token_id tok_id,
                                           DataTier tier) {
        fds_uint16_t cur_file_id = INVALID_FILE_ID;
        std::string key = getKeyString(disk_id, tok_id, tier);

        fds::fds_mutex::scoped_lock l(tokenFileDbMutex);
        if (write_fileids.count(key) > 0) {
            cur_file_id = write_fileids[key];
        }
        return (cur_file_id == file_id);
    }

    fds::Error tokenFileDb::notifyStartGC(fds_uint16_t disk_id,
                                          fds_token_id tok_id,
                                          DataTier tier) {
        fds::Error err(fds::ERR_OK);
        std::string key = getKeyString(disk_id, tok_id, tier);
        fds_uint16_t cur_file_id = INVALID_FILE_ID;
        fds_uint16_t new_file_id = INVALID_FILE_ID;

        fds::fds_mutex::scoped_lock l(tokenFileDbMutex);
        if (write_fileids.count(key) > 0) {
            cur_file_id = write_fileids[key];
        } else {
            return fds::Error(fds::ERR_NOT_FOUND);
        }

        // calculate and set new file id (file we will start writing
	// and to which non-garbage data will be copied)
	new_file_id = getShadowFileId(cur_file_id);
	write_fileids[key] = new_file_id;

        // we are not creating a new token file here, it will be created
        // on the next write to this disk,tok,tier. We are keeping old file
	// until notifyEndGC() is called.

	// check we are in the consistent state
        std::string new_filename = getFileName(disk_id, tok_id, tier, new_file_id);
        // if asserts here, most likely didn't finish last GC properly
        fds_verify(tokenFileTbl.count(new_filename) == 0);

        return err;
    }

    fds::Error tokenFileDb::notifyEndGC(fds_uint16_t disk_id,
                                        fds_token_id tok_id,
                                        DataTier tier) {
        fds::Error err(fds::ERR_OK);
        std::string key = getKeyString(disk_id, tok_id, tier);
        fds_uint16_t shadow_file_id = INVALID_FILE_ID;
        fds_uint16_t old_file_id = INVALID_FILE_ID;
	FilePersisDataIO *del_fdesc = NULL;

        fds::fds_mutex::scoped_lock l(tokenFileDbMutex);
        if (write_fileids.count(key) > 0) {
            shadow_file_id = write_fileids[key];
        } else {
            return fds::Error(fds::ERR_NOT_FOUND);
        }

	// old_file_id is the same as next shadow file id
	old_file_id = getShadowFileId(shadow_file_id);

	// remove file with old_file_id (garbage collect)
	// it is up to the TokenCompactor to check that it copied all
	// non-garbage data to the shadow file
	// TODO(xxx) update below code when supporting multiple files
        std::string old_filename = getFileName(disk_id, tok_id, tier, old_file_id);
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

        return err;
    }

}  // namespace fds
