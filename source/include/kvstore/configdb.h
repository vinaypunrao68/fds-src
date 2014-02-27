/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_KVSTORE_CONFIGDB_H_
#define SOURCE_INCLUDE_KVSTORE_CONFIGDB_H_
#include <kvstore/redis.h>
namespace fds {
    namespace kvstore {
        
        struct ConfigDB {
            ConfigDB(const std::string& host = "localhost", uint port = 6379, uint poolsize = 10);
            ~ConfigDB();

          protected:
            Redis r;
        };

        struct Redis {
            static void encodeHex(const std::string& binary, std::string& hex);
            static void decodeHex(const std::string& hex, std::string& binary);
            Redis(const std::string& host = "localhost",
                  uint port = 6379, uint poolsize = 10);
            ~Redis();

            // send command
            Reply sendCommand(const char* cmdfmt, ...);

            // strings
            Reply set(const std::string& key, const std::string& value);
            Reply get(const std::string& key);
            Reply append(const std::string& key, const std::string& value);
            Reply incr(const std::string& key);
            Reply incrby(const std::string& key,long increment); // NOLINT

            // lists
            Reply lrange(const std::string& key, long start = 0, long end = -1); // NOLINT
            Reply llen(const std::string& key);
            Reply lpush(const std::string& key, const std::string& value);
            Reply rpush(const std::string& key, const std::string& value);

            // hashes
            Reply hset(const std::string& key, const std::string& field,
                       const std::string& value);
            Reply hget(const std::string& key, const std::string& field);
            Reply hgetall(const std::string& key);
            Reply hlen(const std::string& key);


          protected:
            ConnectionPool pool;
        };
    }  // namespace kvstore

}  // namespace fds

#endif  // SOURCE_INCLUDE_KVSTORE_CONFIGDB_H_
