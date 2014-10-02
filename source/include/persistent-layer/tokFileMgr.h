/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_PERSISTENT_LAYER_INCLUDE_TOKFILEMGR_H_
#define SOURCE_PERSISTENT_LAYER_INCLUDE_TOKFILEMGR_H_

#include <string>
#include <unordered_map>
#include <fds_types.h>
#include <persistent_layer/persistentdata.h>
#include <persistent_layer/dm_io.h>

/**
 * Definitions:
 * Active file(s), we also call old file(s), is a file or a set of
 * files that data resides in common case.
 * Shadow file(s) is a new file / set of files that is created when
 * garbage collection starts, and garbage collection copies non-deleted
 * data from active file(s) to shadow file(s). Put are always routed to
 * shadow file(s) if they exist.
 */

/**
 * We keep up to MAX_FILES_PER_TOKEN number of files per token.
 * We start writing to the first file, and once we exceed the max size
 * we continue writing to the next file, and so on. Initially the first
 * file id (start_file_id) is 0x000000001.
 * When garbage collection starts, the start file id of shadow file(s) is
 * calculated by flipping the bit START_TOKFILE_ID_MASK
 *
 * TODO(xxx) we are currently writing to one file per token, and not
 * creating a new file when we exceed max size, will have to implement
 * this feature.
 */
#define INVALID_FILE_ID         0
// no a real file id, use WRITE_FILE_ID when need to access a file we are
// currently appending
#define WRITE_FILE_ID           0x0001
#define INIT_FILE_ID            0x0002
#define START_TOKFILE_ID_MASK   0x4000
/**
 * Ok to change this define to a higher value (up to 0x2000), increasing this
 * number will only increase the runtime of notifyEndGC() function, but not much.
 */
#define MAX_FILES_PER_TOKEN     64

namespace diskio {

typedef fds_uint32_t fds_token_id;
class FilePersisDataIO;

class tokenFileDb {
 public:
    tokenFileDb();
    ~tokenFileDb();

    typedef std::unique_ptr<tokenFileDb> unique_ptr;

    static fds_token_id getTokenId(meta_obj_id_t const *const oid);
    static void getTokenRange(fds_token_id* start_tok, fds_token_id* end_tok);

    /**
     * For next three functions:
     * If 'fileId' is WRITE_FILE_ID, the function will use appropriate file
     * id of the active file (if no shadow file) or shadow file.
     */
    FilePersisDataIO  *openTokenFile(DataTier tier, fds_uint16_t disk_id,
                                     fds_token_id tokId, fds_uint16_t fileId);

    void closeTokenFile(fds_uint16_t disk_id,
                        fds_token_id tokId, DataTier tier, fds_uint16_t fileId);

    FilePersisDataIO *getTokenFile(fds_uint16_t disk_id,
                                   fds_token_id tokId, DataTier tier,
                                   fds_uint16_t fileId);

    /**
     * @return true if file is open and we could get stats
     * TODO(xxx) should also return true with stats if file is closed (persistence)
     */
    fds_bool_t getTokenStats(DataTier tier,
                             fds_uint16_t disk_id,
                             fds_token_id tokId,
                             TokenStat* ret_stat);

    /**
     * Handle notification that garbage collection will start for a given
     * <disk_id, tokId, tier> tuple. This will set a new file id as current.
     */
    fds::Error notifyStartGC(fds_uint16_t disk_id,
                             fds_token_id tok_id,
                             DataTier tier);

    /**
     * Handle notification that objects for this <disk_id, tok_id, tier>
     * tuple were copied to new files and set old files can be deleted.
     * Deletes set of old files.
     */
    fds::Error notifyEndGC(fds_uint16_t disk_id,
                           fds_token_id tok_id,
                           DataTier tier);

    /**
     * Returns true if a given file id is an id of a shadow file
     * or if no shadow file, currently active file we are writing to
     */
    fds_bool_t isShadowFileId(fds_uint16_t file_id,
                              fds_uint16_t disk_id,
                              fds_token_id tok_id,
                              DataTier tier);

 private:  // methods
    std::string getFileName(fds_uint16_t disk_id,
                            fds_token_id tok_id,
                            DataTier tier,
                            fds_uint16_t file_id) const;

    /**
     * translates <disk_id, tok_id, tier> tuple to key into
     * 'write_fileids' map.
     */
    inline std::string getKeyString(fds_uint16_t disk_id,
                                    fds_token_id tok_id,
                                    DataTier tier) const {
        return std::to_string(disk_id) + "_" + std::to_string(tok_id)
                + "_" + std::to_string(tier);
    }

    /*
     * Given file id, returns file id of the shadow file (shadow
     * file may actually not exist, so check that separately).
     * Only valid for file id of the first file in sequence.
     */
    inline fds_uint16_t getShadowFileId(fds_uint16_t file_id) const {
        if (file_id & START_TOKFILE_ID_MASK)
            return file_id & (~START_TOKFILE_ID_MASK);
        return file_id | START_TOKFILE_ID_MASK;
    }

    /**
     * Returns file id of file we are writing to for this <disk_idx, tokdId, tier>
     * tuple. If garbage collection is in progress, this will be the index of
     * of a shadow file, otherwise it will be the index of an active file
     */
    fds_uint16_t getWriteFileId(fds_uint16_t disk_id,
                                fds_token_id tokId,
                                DataTier tier);

 private:
    fds::fds_mutex tokenFileDbMutex;
    std::unordered_map<std::string, FilePersisDataIO *> tokenFileTbl;

    /**
     * map of strings representing <disk_id, tok_id, tier> tuple to a current
     * file id we are writing to. Note that we can infer the old file id from new
     * by flipping the 'start fileid' bit.
     * TODO(xxx) when we move to multiple files per token (if we overflow), we
     * should map to struct that contains start_file_id and offset to the file id
     * we are currently writing to
     */
    std::unordered_map<std::string, fds_uint16_t> write_fileids;
};
}  // namespace diskio
#endif  // SOURCE_PERSISTENT_LAYER_INCLUDE_TOKFILEMGR_H_
