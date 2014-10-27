/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#include "AmQosReq.h"

namespace fds
{

AmQosReq::AmQosReq(AmRequest *_br, fds_uint32_t _reqId)
    : blobReq(_br)
    {
        /*
         * Set the base class defaults
         */
        io_magic  = FDS_SH_IO_MAGIC_IN_USE;
        io_module = STOR_HV_IO;
        io_vol_id = blobReq->vol_id;
        io_type   = blobReq->getIoType();
        io_req_id = _reqId;
    }

}  // namespace fds
