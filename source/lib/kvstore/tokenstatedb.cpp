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
TokenStateDB::TokenStateDB(const std::string& host,
                           uint port,
                           uint poolsize) : KVStore(host, port, poolsize) {
    LOGNORMAL << "instantiating tokenstatedb";
}

TokenStateDB::~TokenStateDB() {
    LOGNORMAL << "destroying tokenstatedb";
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
