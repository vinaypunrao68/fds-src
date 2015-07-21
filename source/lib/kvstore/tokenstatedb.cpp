/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <kvstore/tokenstatedb.h>
#include <util/Log.h>
#include <stdlib.h>
#include <string>
#include <vector>

namespace fds { namespace kvstore {
using redis::Reply;
using redis::RedisException;


TokenStateInfo::TokenStateInfo()
{
    state = INVALID;
    syncStartTs = 0;
    healthyTs = 0;
}

TokenStateDB::TokenStateDB(const std::string& host,
                           uint port,
                           uint poolsize)
    : KVStore(host, port, poolsize),
      lock_("TokenStateDBLock")
{
    LOGNORMAL << "instantiating tokenstatedb";
}

TokenStateDB::~TokenStateDB() {
    LOGNORMAL << "destroying tokenstatedb";
}

Error TokenStateDB::addToken(const fds_token_id &tokId)
{
    fds_mutex::scoped_lock l(lock_);
    auto itr = tokenTbl_.find(tokId);
    if (itr != tokenTbl_.end()) {
        fds_assert(!"Token already exists");
        return ERR_SM_TOKENSTATEDB_DUPLICATE_KEY;
    }
    tokenTbl_[tokId] = TokenStateInfo();
    return ERR_OK;
}

Error TokenStateDB::removeToken(const fds_token_id &tokId)
{
    fds_mutex::scoped_lock l(lock_);
    tokenTbl_.erase(tokId);
    return ERR_OK;
}

Error TokenStateDB::setTokenState(const fds_token_id &tokId,
        const TokenStateInfo::State& state)
{
    fds_mutex::scoped_lock l(lock_);
    auto itr = tokenTbl_.find(tokId);
    if (itr == tokenTbl_.end()) {
        fds_assert(!"Token doesn't exist");
        return ERR_SM_TOKENSTATEDB_KEY_NOT_FOUND;
    }
    itr->second.state = state;
    LOGDEBUG << " state: " << state;
    return ERR_OK;
}

Error TokenStateDB::getTokenState(const fds_token_id &tokId,
        TokenStateInfo::State& state)
{
    fds_mutex::scoped_lock l(lock_);
    auto itr = tokenTbl_.find(tokId);
    if (itr == tokenTbl_.end()) {
        fds_assert(!"Token doesn't exist");
        return ERR_SM_TOKENSTATEDB_KEY_NOT_FOUND;
    }
    state = itr->second.state;
    return ERR_OK;
}

Error TokenStateDB::setTokenSyncStartTS(const fds_token_id &tokId,
        const uint64_t &ts)
{
    fds_mutex::scoped_lock l(lock_);
    auto itr = tokenTbl_.find(tokId);
    if (itr == tokenTbl_.end()) {
        fds_assert(!"Token doesn't exist");
        return ERR_SM_TOKENSTATEDB_KEY_NOT_FOUND;
    }
    itr->second.syncStartTs = ts;
    return ERR_OK;
}

Error TokenStateDB::getTokenSyncStartTS(const fds_token_id &tokId,
        uint64_t &ts)
{
    fds_mutex::scoped_lock l(lock_);
    auto itr = tokenTbl_.find(tokId);
    if (itr == tokenTbl_.end()) {
        fds_assert(!"Token doesn't exist");
        return ERR_SM_TOKENSTATEDB_KEY_NOT_FOUND;
    }
    ts = itr->second.syncStartTs;
    return ERR_OK;
}

Error TokenStateDB::setTokenHealthyTS(const fds_token_id &tokId, const uint64_t &ts)
{
    fds_mutex::scoped_lock l(lock_);
    auto itr = tokenTbl_.find(tokId);
    if (itr == tokenTbl_.end()) {
        fds_assert(!"Token doesn't exist");
        return ERR_SM_TOKENSTATEDB_KEY_NOT_FOUND;
    }
    itr->second.healthyTs = ts;
    return ERR_OK;
}

Error TokenStateDB::getTokenHealthyTS(const fds_token_id &tokId, uint64_t &ts)
{
    fds_mutex::scoped_lock l(lock_);
    auto itr = tokenTbl_.find(tokId);
    if (itr == tokenTbl_.end()) {
        fds_assert(!"Token doesn't exist");
        return ERR_SM_TOKENSTATEDB_KEY_NOT_FOUND;
    }
    ts = itr->second.healthyTs;
    return ERR_OK;
}

Error TokenStateDB::updateHealthyTS(const fds_token_id &tokId, const uint64_t &ts)
{
    fds_mutex::scoped_lock l(lock_);
    auto itr = tokenTbl_.find(tokId);
    if (itr == tokenTbl_.end()) {
        fds_assert(!"Token doesn't exist");
        return ERR_SM_TOKENSTATEDB_KEY_NOT_FOUND;
    }
    if (ts > itr->second.healthyTs) {
        itr->second.healthyTs = ts;
        LOGDEBUG << "token: " << tokId << " healthy ts: " << ts;
    }
    return ERR_OK;
}

void TokenStateDB::
getTokenStats(std::unordered_map<int, int> &stats)  // NOLINT
{
    fds_mutex::scoped_lock l(lock_);
    for (auto tInfo : tokenTbl_) {
        auto itr = stats.find(static_cast<int>(tInfo.second.state));
        if (itr == stats.end()) {
            stats[static_cast<int>(tInfo.second.state)] = 1;
        } else {
            itr->second++;
        }
    }
}

bool TokenStateDB::getTokens(const NodeUuid& uuid, std::vector<fds_token_id>& vecTokens) {
    try {
        Reply reply = r.sendCommand("smembers %ld:tokens", uuid);
        std::vector<long long> vecLongs; //NOLINT
        reply.toVector(vecLongs);

        vecTokens.resize(vecLongs.size());
        for (uint i = 0 ; i < vecLongs.size() ; i++) {
            vecTokens.push_back(vecLongs[i]);
        }

        return true;
    } catch(const RedisException& e) {
        LOGERROR << e.what();
    }
    return false;
}

bool TokenStateDB::addToken(const NodeUuid& uuid, fds_token_id token) {
    try {
        Reply reply = r.sendCommand("sadd %ld:tokens %ld", uuid, token);
        return reply.isOk();
    } catch(const RedisException& e) {
        LOGERROR << e.what();
    }
    return false;
}

bool TokenStateDB::setTokenInfo(const NodeUuid& uuid, fds_token_id token, ConstString value) { //NOLINT
    try {
        Reply reply = r.sendCommand("set %ld:token:%ld:info %b", uuid, token, value.data(), value.length()); //NOLINT
        return reply.isOk();
    } catch(const RedisException& e) {
        LOGERROR << e.what();
    }
    return false;
}

bool TokenStateDB::removeToken(const NodeUuid& uuid, fds_token_id token) {
    try {
        Reply reply = r.sendCommand("sdel %ld:tokens %ld", uuid, token);
        return reply.wasModified();
    } catch(const RedisException& e) {
        LOGERROR << e.what();
    }
    return false;
}

}  // namespace kvstore
}  // namespace fds
