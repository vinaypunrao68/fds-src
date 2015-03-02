/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <kvstore/platformdb.h>
#include <util/Log.h>
#include <stdlib.h>
#include <string>
#include <fdsp_utils.h>
namespace fds { 
namespace kvstore {

using redis::Reply;
using redis::RedisException;

PlatformDB::PlatformDB(const std::string& host,
                   uint port,
                   uint poolsize) : KVStore(host, port, poolsize) {
    LOGNORMAL << "instantiating platformdb";
}

PlatformDB::~PlatformDB() {
    LOGNORMAL << "destroying platformdb";
}

bool PlatformDB::setNodeInfo(const fpi::NodeInfo& nodeInfo) {
    try {
        boost::shared_ptr<std::string> serialized;
        fds::serializeFdspMsg(nodeInfo, serialized);
        r.set("node.info", *serialized);
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        return false;
    }
    return true;
}

bool PlatformDB::getNodeInfo(fpi::NodeInfo& nodeInfo) {
    try {
        Reply reply = r.get("node.info");
        if (reply.isNil()) return false;
        std::string value = reply.getString();
        fds::deserializeFdspMsg(value, nodeInfo);
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        return false;
    }
    return true;
}

bool PlatformDB::setNodeDiskCapability(const fpi::FDSP_AnnounceDiskCapability& diskCapability) {
    try {
        boost::shared_ptr<std::string> serialized;
        fds::serializeFdspMsg(diskCapability, serialized);
        r.set("node.disk.capability", *serialized);
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        return false;
    }
    return true;
}

bool PlatformDB::getNodeDiskCapability(fpi::FDSP_AnnounceDiskCapability& diskCapability) {
    try {
        Reply reply = r.get("node.disk.capability");
        if (reply.isNil()) return false;
        std::string value = reply.getString();
        fds::deserializeFdspMsg(value, diskCapability);
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        return false;
    }
    return true;

}

}  // namespace kvstore
}  // namespace fds
