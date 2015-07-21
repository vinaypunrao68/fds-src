/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <kvstore/platformdb.h>
#include <util/Log.h>
#include <stdlib.h>
#include <string>
#include <algorithm>
#include <fdsp_utils.h>
namespace fds {
namespace kvstore {

using redis::Reply;
using redis::RedisException;

PlatformDB::PlatformDB(std::string const &uniqueId,
                       const std::string& keyBase,
                       const std::string& host,
                       uint port,
                       uint poolsize) : KVStore(host, port, poolsize) {
    this->keyBase = uniqueId + keyBase;
    std::replace(this->keyBase.begin(), this->keyBase.end(), '/', '.');
    LOGNORMAL << "instantiating platformdb with key base: " << this->keyBase;
}

PlatformDB::~PlatformDB() {
    LOGNORMAL << "destroying platformdb";
}

bool PlatformDB::setNodeInfo(const fpi::NodeInfo& nodeInfo) {
    try {
        boost::shared_ptr<std::string> serialized;
        fds::serializeFdspMsg(nodeInfo, serialized);
        setInternal("node.info", *serialized);
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        return false;
    }

    LOGDEBUG << "Updated nodeInfo record stored in redis with: uuid " << nodeInfo.uuid <<
                                                              ", am " << nodeInfo.fHasAm <<
                                                              ", dm " << nodeInfo.fHasDm <<
                                                              ", sm " << nodeInfo.fHasSm <<
                                                         ", bam pid " << nodeInfo.bareAMPid <<
                                                         ", jam pid " << nodeInfo.javaAMPid <<
                                                          ", dm pid " << nodeInfo.dmPid <<
                                                          ", sm pid " << nodeInfo.smPid <<
                                                         ", bam ste " << nodeInfo.bareAMState <<
                                                         ", jam ste " << nodeInfo.javaAMState <<
                                                          ", dm ste " << nodeInfo.dmState <<
                                                          ", sm ste " << nodeInfo.smState;

    return true;
}

bool PlatformDB::getNodeInfo(fpi::NodeInfo& nodeInfo) {
    try {
        Reply reply = getInternal("node.info");
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
        setInternal("node.disk.capability", *serialized);
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        return false;
    }
    return true;
}

bool PlatformDB::getNodeDiskCapability(fpi::FDSP_AnnounceDiskCapability& diskCapability) {
    try {
        Reply reply = getInternal("node.disk.capability");
        if (reply.isNil()) return false;
        std::string value = reply.getString();
        fds::deserializeFdspMsg(value, diskCapability);
    } catch(const RedisException& e) {
        LOGCRITICAL << "error with redis " << e.what();
        return false;
    }
    return true;

}

Reply PlatformDB::getInternal(const std::string &key) {
   auto computedKey = computeKey(key);
   return r.get(computedKey);
}

void PlatformDB::setInternal(const std::string &key, const std::string &value) {
   auto computedKey = computeKey(key);
   r.set(computedKey, value);
}

std::string PlatformDB::computeKey(const std::string &k)
{
    if (keyBase.size() == 0) {
        return k;
    }
    return keyBase + k;
}

}  // namespace kvstore
}  // namespace fds
