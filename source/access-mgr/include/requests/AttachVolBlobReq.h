/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_ATTACHVOLBLOBREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_ATTACHVOLBLOBREQ_H_

#include <string>

#include "AmRequest.h"

namespace fds
{

/**
 * AM request to locally attach a volume.
 * This request is not specific to a blob,
 * but needs to be in the blob wait queue
 * to call the callback and notify that the
 * attach is complete.
 */
struct AttachVolBlobReq : public AmRequest {
    /**
     * Request constructor. Some of the fields
     * are not actually needed...the base blob
     * request class just expects them.
     */
    AttachVolBlobReq(fds_volid_t        _volid,
                     const std::string& _vol_name,
                     CallbackPtr        cb) :
        AmRequest(FDS_ATTACH_VOL, _volid, _vol_name, "", cb)
    {
    }

    ~AttachVolBlobReq() { }
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_ATTACHVOLBLOBREQ_H_
