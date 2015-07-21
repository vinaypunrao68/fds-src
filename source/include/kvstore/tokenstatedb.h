/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_KVSTORE_TOKENSTATEDB_H_
#define SOURCE_INCLUDE_KVSTORE_TOKENSTATEDB_H_
#include <unordered_map>
#include <boost/shared_ptr.hpp>

#include <concurrency/Mutex.h>
#include <kvstore/kvstore.h>
#include "fds_resource.h"
#include <fds_typedefs.h>
#include <string>
#include <vector>

namespace fds {
namespace kvstore {
/**
 * Stores token state information
 */
class TokenStateInfo {
 public:
    enum State {
        /* Invalid or not owned */
        INVALID,

        /* Owned but in an unknown state */
        UNINITIALIZED,

        /* Token is upto date and healthy */
        HEALTHY,

        /* We copy objects and metadata */
        COPYING,

        /* During sync we sync metadata */
        SYNCING,

        /* Metadata sync is complete but data pull is not complete */
        PULL_REMAINING
    };
    TokenStateInfo();

    /* Current state */
    State state;

    /* Time when sync started.  Valid only when state is either
     * SYNCING or PULL_REMAINING
     */
    uint64_t syncStartTs;

    /* Timestamp upto which the token information is considered healthy */
    uint64_t healthyTs;
};

struct TokenStateDB : KVStore {
    TokenStateDB(const std::string& host = "localhost",
            uint port = 6379,
            uint poolsize = 5);

    Error addToken(const fds_token_id &tokId);
    Error removeToken(const fds_token_id &tokId);
    Error setTokenState(const fds_token_id &tokId,
            const TokenStateInfo::State& state);
    Error getTokenState(const fds_token_id &tokId, TokenStateInfo::State& state);
    Error setTokenSyncStartTS(const fds_token_id &tokId, const uint64_t &ts);
    Error getTokenSyncStartTS(const fds_token_id &tokId, uint64_t &ts);
    Error setTokenHealthyTS(const fds_token_id &tokId, const uint64_t &ts);
    Error getTokenHealthyTS(const fds_token_id &tokId, uint64_t &ts);
    Error updateHealthyTS(const fds_token_id &tokId, const uint64_t &ts);
    void getTokenStats(std::unordered_map<int, int> &stats);  // NOLINT

    bool getTokens(const NodeUuid& uuid, std::vector<fds_token_id>& vecTokens);
    bool addToken(const NodeUuid& uuid, fds_token_id token);
    bool setTokenInfo(const NodeUuid& uuid, fds_token_id token, ConstString value); // NOLINT
    bool removeToken(const NodeUuid& uuid, fds_token_id token);

    virtual ~TokenStateDB();

 private:
    fds_mutex lock_;
    std::unordered_map<fds_token_id, TokenStateInfo> tokenTbl_;
};
typedef boost::shared_ptr<TokenStateDB> TokenStateDBPtr;

}  // namespace kvstore // NOLINT

}  // namespace fds

#endif  // SOURCE_INCLUDE_KVSTORE_TOKENSTATEDB_H_
