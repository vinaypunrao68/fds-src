/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_KVSTORE_PLATFORMDB_H_
#define SOURCE_INCLUDE_KVSTORE_PLATFORMDB_H_
#include <kvstore/kvstore.h>
#include <string>

namespace fds {
    namespace kvstore {
        struct PlatformDB : KVStore {
            PlatformDB(const std::string& host = "localhost",
                       uint port = 6379,
                       uint poolsize = 10);
            virtual ~PlatformDB();
        };
    }  // namespace kvstore

}  // namespace fds

#endif  // SOURCE_INCLUDE_KVSTORE_PLATFORMDB_H_
