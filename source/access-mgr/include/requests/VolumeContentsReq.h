/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_VOLUMECONTENTSREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_VOLUMECONTENTSREQ_H_

#include <string>

#include "native_api.h"
#include "FdsBlobReq.h"

namespace fds
{

class VolumeContentsReq: public FdsBlobReq {
    static const fds_uint64_t fds_sh_volume_list_magic = 0xBCE12345;

  public:
    typedef std::function<void (const Error&)> VolumeContentsProcCb;

    BucketContext *bucket_ctxt;
    std::string prefix;
    std::string marker;
    std::string delimiter;
    fds_uint32_t maxkeys;
    void *request_context;
    fdsnVolumeContentsHandler handler;
    void *callback_data;
    fds_long_t iter_cookie = 0;

    VolumeContentsProcCb processorCb;
    fds_volid_t base_vol_id;

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
            : FdsBlobReq(FDS_VOLUME_CONTENTS, _volid,
                         _bucket_ctxt->bucketName, fds_sh_volume_list_magic, 0, NULL,
                         FDS_NativeAPI::DoCallback, this, Error(ERR_OK), 0),
        bucket_ctxt(_bucket_ctxt),
        prefix(_prefix),
        marker(_marker),
        delimiter(_delimiter),
        maxkeys(_max_keys),
        request_context(_req_context),
        handler(_handler),
        callback_data(_callback_data),
        iter_cookie(0) {
    }

    VolumeContentsReq(fds_volid_t _volid,
                  BucketContext *_bucket_ctxt,
                  fds_uint32_t _max_keys,
                  CallbackPtr cb)
            : FdsBlobReq(FDS_VOLUME_CONTENTS, _volid,
                         _bucket_ctxt->bucketName, 0, 0, NULL, cb), bucket_ctxt(_bucket_ctxt) {
    }

    ~VolumeContentsReq() {}

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
