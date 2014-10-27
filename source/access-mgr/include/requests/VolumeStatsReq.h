/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_VOLUMESTATSREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_VOLUMESTATSREQ_H_

#include <string>

#include "native/types.h"
#include "fds_volume.h"

#include "AmRequest.h"

namespace fds
{

struct VolumeStatsReq: public AmRequest {
    void *request_context;
    fdsnVolumeStatsHandler resp_handler;
    void *callback_data;

    /* for "get all bucket stats", blob name in base class is set to
     * "all", it's ok if some other bucket will have this name, because
     * we can identify response to a request by trans id (once request is dispatched */

    VolumeStatsReq(void *_req_context,
                   fdsnVolumeStatsHandler _handler,
                   void *_callback_data)
            : AmRequest(FDS_BUCKET_STATS, admin_vol_id, "all", "",
                        fds_sh_volume_stats_magic, 0, NULL,
                        FDS_NativeAPI::DoCallback, this, Error(ERR_OK), 0),
              request_context(_req_context),
              resp_handler(_handler),
              callback_data(_callback_data) {
    }

    ~VolumeStatsReq() {}

    void DoCallback(const std::string& timestamp,
                    int content_count,
                    const BucketStatsContent* contents,
                    FDSN_Status status,
                    ErrorDetails *err_details) {
        (resp_handler)(timestamp, content_count, contents,
                       request_context, callback_data, status, err_details);
    }

 private:
    static const fds_uint64_t fds_sh_volume_stats_magic = 0xCDF23456;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_VOLUMESTATSREQ_H_
