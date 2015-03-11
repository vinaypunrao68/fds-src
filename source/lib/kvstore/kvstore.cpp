/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <kvstore/kvstore.h>
#include <util/Log.h>
#include <stdlib.h>
#include <string>

namespace fds { namespace kvstore {
using redis::Reply;
KVStore::KVStore(const std::string& host,
                   uint port,
                   uint poolsize) : r(host, port, poolsize) {
    LOGNORMAL << "instantiating kvstore";
}

KVStore::~KVStore() {
    LOGNORMAL << "destroying kvstore";
}

bool KVStore::isConnected() {
    return r.isConnected();
}

bool KVStore::set(const std::string& key, const std::string& value) {
    return r.set(key, value);
}

std::string KVStore::get(const std::string& key) {
    Reply reply = r.get(key);
    return reply.getString();
}

}  // namespace kvstore
}  // namespace fds
