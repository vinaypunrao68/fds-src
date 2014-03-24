/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_PERSISTENT_LAYER_INCLUDE_TOKFILEMGR_H_
#define SOURCE_PERSISTENT_LAYER_INCLUDE_TOKFILEMGR_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <strings.h>
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

#include <utility>
#include <atomic>
#include <unordered_map>

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
