/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMREQHANDLERS_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMREQHANDLERS_H_

#include <string>
#include <fds_types.h>
#include <native/types.h>
#include <StorHvVolumes.h>

namespace fds {

/**
 * Class containing fields common to most AM requests.
 * Extends from FDS_IOType, which allows the request
 * to be managed by the QoS controller.
 */
struct AmReqHandler : public FDS_IOType {
    fds_io_op_t                    opType;
    boost::shared_ptr<std::string> blobName;
    boost::shared_ptr<std::string> volumeName;
    fds_volid_t                    volumeId;

    /// Callback to AmProcessor
    typedef std::function<void (const Error&)> ProcessorCb;
    ProcessorCb procCb;

    /// Callback to AmServiceApi
    CallbackPtr serviceApiCb;
};

struct StartBlobTxReqHandler : public AmReqHandler {
    boost::shared_ptr<fds_int32_t> blobMode;

    typedef boost::shared_ptr<StartBlobTxReqHandler> ptr;
    StartBlobTxReqHandler(boost::shared_ptr<std::string> &_volumeName,
                          boost::shared_ptr<std::string> &_blobName,
                          boost::shared_ptr<fds_int32_t> &_blobMode,
                          CallbackPtr _serviceCb)
            : blobMode(_blobMode) {
        volumeName   = _volumeName;
        blobName     = _blobName;
        serviceApiCb = _serviceCb;
    }
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMREQHANDLERS_H_
