/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_GETBLOBREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_GETBLOBREQ_H_

#include <string>

#include "FdsBlobReq.h"

namespace fds
{

class GetBlobReq: public FdsBlobReq {
  public:
    typedef std::function<void (const Error&)> GetBlobProcCb;
    GetBlobProcCb processorCb;

    fds_volid_t base_vol_id;

    GetBlobReq(fds_volid_t _volid,
               const std::string& _volumeName,
               const std::string& _blob_name,
               fds_uint64_t _blob_offset,
               fds_uint64_t _data_len,
               char* _data_buf,
               fds_uint64_t _byte_count,
               CallbackPtr cb);

    virtual ~GetBlobReq();

    void DoCallback(FDSN_Status status, ErrorDetails* errDetails) {
        fds_panic("Depricated. You shouldn't be here!");
    }
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_GETBLOBREQ_H_
