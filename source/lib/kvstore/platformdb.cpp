/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <kvstore/platformdb.h>
#include <util/Log.h>
#include <stdlib.h>
#include <string>
namespace fds { namespace kvstore {

PlatformDB::PlatformDB(const std::string& host,
                   uint port,
                   uint poolsize) : KVStore(host, port, poolsize) {
    LOGNORMAL << "instantiating platformdb";
}

PlatformDB::~PlatformDB() {
    LOGNORMAL << "destroying platformdb";
}

}  // namespace kvstore
}  // namespace fds
