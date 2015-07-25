/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_KVSTORE_PLATFORMDB_H_
#define SOURCE_INCLUDE_KVSTORE_PLATFORMDB_H_
#include <kvstore/kvstore.h>
#include <string>
#include <fdsp/pm_data_types.h>
#include <fds_typedefs.h>
namespace fds {
namespace kvstore {

struct PlatformDB : KVStore {
    PlatformDB(std::string const &uniqueID = "",
               const std::string& keyBase = "",
               const std::string& host = "localhost",
               uint port = 6379,
               uint poolsize = 10);
    virtual ~PlatformDB();

    bool setNodeInfo(const fpi::NodeInfo& nodeInfo);
    bool getNodeInfo(fpi::NodeInfo& nodeInfo);
    bool setNodeDiskCapability(const fpi::FDSP_AnnounceDiskCapability& diskCapability); // NOLINT
    bool getNodeDiskCapability(fpi::FDSP_AnnounceDiskCapability& diskCapability); // NOLINT


 protected:
    redis::Reply getInternal(const std::string &key);
    bool setInternal(const std::string &key, const std::string &value);
    std::string computeKey(const std::string &k);

    std::string keyBase;
};
}  // namespace kvstore
}  // namespace fds

#endif  // SOURCE_INCLUDE_KVSTORE_PLATFORMDB_H_
