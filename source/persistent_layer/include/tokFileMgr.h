/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_PERSISTENT_LAYER_INCLUDE_TOKFILEMGR_H_
#define SOURCE_PERSISTENT_LAYER_INCLUDE_TOKFILEMGR_H_

#include <string>
#include <unordered_map>
#include <persistentdata.h>
#include <persistent_layer/dm_service.h>
#include <persistent_layer/dm_io.h>

#define PL_TOKEN_MASK 0x000000ff

namespace diskio {
typedef fds_uint32_t fds_token_id;


class FilePersisDataIO;

class tokenFileDb {
 public:
    tokenFileDb();
    ~tokenFileDb();
    fds::fds_mutex *tokenFileDbMutex;
    std::unordered_map<std::string, FilePersisDataIO *> tokenFileTbl;

    inline fds_token_id GetTokenFileDb(const fds_token_id &tokId);

    FilePersisDataIO  *openTokenFile(DataTier tier, fds_uint32_t disk_idx,
                                     fds_token_id tokId, fds_uint32_t fileId);

    void closeTokenFile(fds_uint32_t disk_idx,
                        fds_token_id tokId, DataTier tier, fds_uint32_t fileId);

    FilePersisDataIO *getTokenFile(fds_uint32_t disk_idx,
                                   fds_token_id tokId, DataTier tier,
                                   fds_uint32_t fileId);
};
}  // namespace diskio
#endif  // SOURCE_PERSISTENT_LAYER_INCLUDE_TOKFILEMGR_H_
