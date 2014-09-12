/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_SMDISKTYPES_H_
#define SOURCE_STOR_MGR_INCLUDE_SMDISKTYPES_H_

#include <fds_types.h>
#include <persistent_layer/dm_io.h>

namespace fds {

/**
 * Wrapper for an SM request to the persistent layer
 * TODO(Andrew): Is this needed? What value does it add?
 */
class SmPlReq : public diskio::DiskRequest {
  public:
    /*
     * TODO: This defaults to disk at the moment...
     * need to specify any tier, specifically for
     * read
     */
    SmPlReq(meta_vol_io_t   &vio,
            meta_obj_id_t   &oid,
            ObjectBuf        *buf,
            fds_bool_t        block)
            : diskio::DiskRequest(vio, oid, buf, block) {
    }
    SmPlReq(meta_vol_io_t   &vio,
            meta_obj_id_t   &oid,
            ObjectBuf        *buf,
            fds_bool_t        block,
            diskio::DataTier  tier)
            : diskio::DiskRequest(vio, oid, buf, block, tier) {
    }
    ~SmPlReq() { }

    void req_submit() {
        fdsio::Request::req_submit();
    }
    void req_complete() {
        fdsio::Request::req_complete();
    }
    void setTier(diskio::DataTier tier) {
        datTier = tier;
    }
};

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_SMDISKTYPES_H_
