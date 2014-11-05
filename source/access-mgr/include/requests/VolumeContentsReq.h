/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_VOLUMECONTENTSREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_VOLUMECONTENTSREQ_H_

#include <string>

#include "native_api.h"
#include "AmRequest.h"

namespace fds
{

class VolumeContentsReq: public AmRequest {
    static const fds_uint64_t fds_sh_volume_list_magic = 0xBCE12345;

  public:
    BucketContext *bucket_ctxt;
    std::string prefix;
    std::string marker;
    std::string delimiter;
    fds_uint32_t maxkeys;
    void *request_context;
    fdsnVolumeContentsHandler handler;
    void *callback_data;
    fds_long_t iter_cookie = 0;

    /* sets bucket name to blob name in the base class,
     * which is used to get trans id in journal table, and
     * some magic number for offset */
    VolumeContentsReq(fds_volid_t _volid,
                  BucketContext *_bucket_ctxt,
                  const std::string& _prefix,
                  const std::string& _marker,
                  const std::string& _delimiter,
                  fds_uint32_t _max_keys,
                  void* _req_context,
                  fdsnVolumeContentsHandler _handler,
                  void* _callback_data)
            : AmRequest(FDS_VOLUME_CONTENTS, _volid, _bucket_ctxt->bucketName,
                        "", fds_sh_volume_list_magic, 0, nullptr, FDS_NativeAPI::DoCallback,
                        this, Error(ERR_OK), 0),
        bucket_ctxt(_bucket_ctxt),
        prefix(_prefix),
        marker(_marker),
        delimiter(_delimiter),
        maxkeys(_max_keys),
        request_context(_req_context),
        handler(_handler),
        callback_data(_callback_data),
        iter_cookie(0) {
        e2e_req_perf_ctx.type = AM_VOLUME_CONTENTS_REQ;
        fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
    }

    VolumeContentsReq(fds_volid_t _volid,
                  BucketContext *_bucket_ctxt,
                  fds_uint32_t _max_keys,
                  CallbackPtr cb)
            :   AmRequest(FDS_VOLUME_CONTENTS, _volid, _bucket_ctxt->bucketName, "", cb),
                bucket_ctxt(_bucket_ctxt) {
        e2e_req_perf_ctx.type = AM_VOLUME_CONTENTS_REQ;
        fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
    }

    void DoCallback(int isTruncated,
                    const char* next_marker,
                    int contents_count,
                    const ListBucketContents* contents,
                    FDSN_Status status,
                    ErrorDetails* errDetails) {
        (handler)(isTruncated, next_marker, contents_count,
                  contents, 0, NULL, callback_data, status);
    }
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_VOLUMECONTENTSREQ_H_
