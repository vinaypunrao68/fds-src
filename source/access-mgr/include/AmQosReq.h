/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMQOSREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMQOSREQ_H_

#include "fds_types.h"
#include "FdsBlobReq.h"

namespace fds
{

/*
 * Internal wrapper class for AM QoS
 * requests.
 */
struct AmQosReq : public FDS_IOType {
    AmQosReq(FdsBlobReq *_br, fds_uint32_t _reqId);

    fds_bool_t magicInUse() const {
        return blobReq->magicInUse();
    }

    FdsBlobReq* getBlobReqPtr() const {
        return blobReq;
    }

    void setVolId(fds_volid_t volid) {
        io_vol_id = volid;
        blobReq->vol_id = volid;
    }

 private:
    FdsBlobReq *blobReq;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMQOSREQ_H_
