/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_KVSTORE_TOKENSTATEDB_H_
#define SOURCE_INCLUDE_KVSTORE_TOKENSTATEDB_H_
#include <kvstore/kvstore.h>
#include <platform/node-inventory.h>
#include <fds_typedefs.h>

#include <string>
#include <vector>

namespace fds {
    namespace kvstore {
        struct TokenStateDB : KVStore {
            TokenStateDB(const std::string& host = "localhost",
                       uint port = 6379,
                       uint poolsize = 5);

            bool getTokens(const NodeUuid& uuid, std::vector<fds_token_id>& vecTokens);
            bool addToken(const NodeUuid& uuid, fds_token_id token);
            bool setTokenInfo(const NodeUuid& uuid, fds_token_id token, ConstString value); // NOLINT
            bool removeToken(const NodeUuid& uuid, fds_token_id token);

            virtual ~TokenStateDB();
        };
    }  // namespace kvstore // NOLINT
}  // namespace fds

#endif  // SOURCE_INCLUDE_KVSTORE_TOKENSTATEDB_H_
