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
                   uint poolsize) : kv_store(host, port, poolsize) {
    LOGNORMAL << "instantiating kvstore";
}

KVStore::~KVStore() {
    LOGNORMAL << "destroying kvstore";
}

bool KVStore::isConnected() {
    return kv_store.isConnected();
}

bool KVStore::set(const std::string& key, const std::string& value) {
    return kv_store.set(key, value);
}

std::string KVStore::get(const std::string& key) {
    return kv_store.get(key).getString();
}

}  // namespace kvstore
}  // namespace fds
