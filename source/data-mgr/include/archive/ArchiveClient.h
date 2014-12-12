/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_ARCHIVE_CLIENT_H_
#define SOURCE_DATA_MGR_INCLUDE_ARCHIVE_CLIENT_H_

#include <fds_types.h>

namespace fds {

/**
* @brief
*/
struct ArchiveClReq {
    fds_volid_t volId;
};

struct ArchiveClPutReq {
    std::string snapLoc;
};

struct ArchiveClGetReq {
    std::string snapName;
};

/**
* @brief 
*/
struct ArchiveClient {
    typedef std::function<void ()> ArchivePutCb;
    typedef std::function<void ()> ArchiveGetCb;

    void putSnap(const fds_volid_t &volId, const std::string &snapLoc, ArchivePutCb cb);
    void getSnap(const fds_volid_t &volId, const std::string &snapName, ArchiveGetCb cb);
};

}  // namespace fds
#endif
