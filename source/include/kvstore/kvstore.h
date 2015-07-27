/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_KVSTORE_KVSTORE_H_
#define SOURCE_INCLUDE_KVSTORE_KVSTORE_H_
#include <kvstore/redis.h>
#include <string>

namespace fds {
    namespace kvstore{
        struct Exception: redis::RedisException {
            explicit Exception(const char* cstr) : RedisException(cstr) {
            }

            explicit Exception(const std::string& reason)
                    : redis::RedisException(reason) {
            }
        };

        struct KVStore {
            KVStore(const std::string& host = "localhost",
                    uint port = 6379,
                    uint poolsize = 5);
            virtual ~KVStore();
            bool isConnected();
            bool set(const std::string& key, const std::string& value);
            std::string get(const std::string& key);
          protected:
            redis::Redis kv_store;
        };

    }  // namespace kvstore //NOLINT ??
}  // namespace fds

#endif  // SOURCE_INCLUDE_KVSTORE_KVSTORE_H_
