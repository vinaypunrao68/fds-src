/* Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_ASYNC_RPC_RSP_HANDLER_H
#define SOURCE_INCLUDE_ASYNC_RPC_RSP_HANDLER_H

// #include <fdsp/fds_service_types.h>
#include <fdsp/BaseAsyncSvc.h>

namespace fds {

class BaseAsyncSvcHandler : virtual public FDS_ProtocolInterface::BaseAsyncSvcIf {
public:
    BaseAsyncSvcHandler();
    virtual ~BaseAsyncSvcHandler();

    void asyncReqt(const FDS_ProtocolInterface::AsyncHdr& header);
    void asyncReqt(boost::shared_ptr<FDS_ProtocolInterface::AsyncHdr>& header);

    void asyncResp(const FDS_ProtocolInterface::AsyncHdr& header,
            const std::string& payload);
    void asyncResp(boost::shared_ptr<FDS_ProtocolInterface::AsyncHdr>& header,
            boost::shared_ptr<std::string>& payload);

    void uuidBind(const FDS_ProtocolInterface::UuidBindMsg& msg);
    void uuidBind(boost::shared_ptr<FDS_ProtocolInterface::UuidBindMsg>& msg);
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_ASYNC_RPC_RSP_HANDLER_H
