/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_GETBLOBREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_GETBLOBREQ_H_

#include <string>

#include "AmRequest.h"

namespace fds
{

struct GetBlobReq: public AmRequest {
    fds_bool_t get_metadata;

    GetBlobReq(fds_volid_t _volid,
               const std::string& _volumeName,
               const std::string& _blob_name,
               CallbackPtr cb,
               fds_uint64_t _blob_offset,
               fds_uint64_t _data_len,
               char* _data_buf = nullptr);

    ~GetBlobReq();
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_GETBLOBREQ_H_
