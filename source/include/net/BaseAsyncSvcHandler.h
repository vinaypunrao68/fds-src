/* Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_BASEASYNCSVCHANDLER_H_
#define SOURCE_INCLUDE_NET_BASEASYNCSVCHANDLER_H_

#include <string>
#include <fdsp/fds_service_types.h>
#include <fdsp/BaseAsyncSvc.h>

namespace fds {

class BaseAsyncSvcHandler : virtual public FDS_ProtocolInterface::BaseAsyncSvcIf {
 public:
    BaseAsyncSvcHandler();
    virtual ~BaseAsyncSvcHandler();

    void asyncReqt(const FDS_ProtocolInterface::AsyncHdr& header) override;
    void asyncReqt(boost::shared_ptr<FDS_ProtocolInterface::AsyncHdr>& header) override;

    void asyncResp(const FDS_ProtocolInterface::AsyncHdr& header,
            const std::string& payload) override;
    void asyncResp(boost::shared_ptr<FDS_ProtocolInterface::AsyncHdr>& header,
            boost::shared_ptr<std::string>& payload) override;

    virtual void uuidBind(FDS_ProtocolInterface::RespHdr& _return,
                        const FDS_ProtocolInterface::UuidBindMsg& msg) override;
    virtual void uuidBind(FDS_ProtocolInterface::RespHdr& _return,
                    boost::shared_ptr<FDS_ProtocolInterface::UuidBindMsg>& msg) override;

    static void asyncRespHandler(
        boost::shared_ptr<FDS_ProtocolInterface::AsyncHdr>& header,
        boost::shared_ptr<std::string>& payload);
};
}  // namespace fds

#endif  // SOURCE_INCLUDE_NET_BASEASYNCSVCHANDLER_H_
