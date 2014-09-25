/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_DMHANDLER_H_
#define SOURCE_DATA_MGR_INCLUDE_DMHANDLER_H_

#include <string>
#include <fds_error.h>
#include <fds_types.h>
#include <util/Log.h>
#include <net/PlatNetSvcHandler.h>
#include <net/SvcRequestPool.h>
#include <DmIoReq.h>
#include <dm-platform.h>
#define DMHANDLER(CLASS, IOTYPE) \
    static_cast<CLASS*>(dataMgr->handlers.at(IOTYPE))

#define REGISTER_DM_MSG_HANDLER(FDSPMsgT, func) \
    REGISTER_FDSP_MSG_HANDLER_GENERIC(gl_DmPlatform.getDmRecv(), FDSPMsgT, func)

#define DM_SEND_ASYNC_RESP(...) gl_DmPlatform.getDmRecv()->sendAsyncResp(__VA_ARGS__)

namespace fds { namespace dm {
/**
 * ------ NOTE :: IMPORTANT ---
 * do NOT store any state in these classes for now.
 * handler functions should be reentrant 
 */
struct RequestHelper {
    dmCatReq *dmRequest;
    explicit RequestHelper(dmCatReq *dmRequest);
    ~RequestHelper();
};

struct QueueHelper {
    dmCatReq *dmRequest;
    Error err = ERR_OK;
    explicit QueueHelper(dmCatReq *dmRequest);
    ~QueueHelper();
};


struct Handler: HasLogger {
    // providing a default empty implmentation to support requests that
    // do not need queuing
    virtual void handleQueueItem(dmCatReq *dmRequest);
    virtual void addToQueue(dmCatReq *dmRequest);
    virtual ~Handler();
};

struct GetBucketHandler : Handler {
    GetBucketHandler();
    void handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                       boost::shared_ptr<fpi::GetBucketMsg>& message);
    void handleQueueItem(dmCatReq *dmRequest);
    void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                        boost::shared_ptr<fpi::GetBucketMsg>& message,
                        const Error &e, dmCatReq *dmRequest);
};

struct DeleteBlobHandler : Handler {
    DeleteBlobHandler();
    void handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                       boost::shared_ptr<fpi::DeleteBlobMsg>& message);
    void handleQueueItem(dmCatReq *dmRequest);
    void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                        boost::shared_ptr<fpi::DeleteBlobMsg>& message,
                        const Error &e, dmCatReq *dmRequest);
};


}  // namespace dm
}  // namespace fds
#endif  // SOURCE_DATA_MGR_INCLUDE_DMHANDLER_H_
