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
class AttachVolBlobReq : public AmRequest {
  public:
    /**
     * Request constructor. Some of the fields
     * are not actually needed...the base blob
     * request class just expects them.
     */
    AttachVolBlobReq(fds_volid_t          _volid,
                     const std::string   &_vol_name,
                     const std::string   &_blob_name,
                     fds_uint64_t         _blob_offset,
                     fds_uint64_t         _data_len,
                     char                *_data_buf,
                     CallbackPtr cb) :
            AmRequest(FDS_ATTACH_VOL, _volid, _blob_name, _blob_offset,
                       _data_len, _data_buf, cb) {
        volume_name = _vol_name;
    }
    ~AttachVolBlobReq() {
    }
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_ATTACHVOLBLOBREQ_H_
