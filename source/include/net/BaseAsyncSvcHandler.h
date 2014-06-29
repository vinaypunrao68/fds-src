/* Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_BASEASYNCSVCHANDLER_H_
#define SOURCE_INCLUDE_NET_BASEASYNCSVCHANDLER_H_

#include <string>
#include <unordered_map>
#include <fdsp/fds_service_types.h>
#include <fdsp/BaseAsyncSvc.h>

/**
 * Maps FDSPMsg type to FDSPMsgTypeId enum
 */
#define FDSP_MSG_TYPEID(FDSPMsgT) fpi::##FDSPMsgT##TypeId

/**
 * Use this macro for registering FDSP message handlers
 */
#define REGISTER_FDSP_MSG_HANDLER(FDSPMsgT, func) \
    asyncReqHandlers_[FDSP_MSG_TYPEID(FDSPMsgT)] = \
    [] (boost::shared_ptr<FDS_ProtocolInterface::AsyncHdr>& header, \
        boost::shared_ptr<std::string>& payload) \
    { \
        auto result = fds::deserializeFdspMsg(payload); \
        this->func(header, result); \
    }

namespace fpi = FDS_ProtocolInterface;

namespace fds {

class BaseAsyncSvcHandler : virtual public FDS_ProtocolInterface::BaseAsyncSvcIf {
    typedef std::function<void (boost::shared_ptr<FDS_ProtocolInterface::AsyncHdr>&,
                                boost::shared_ptr<std::string>&)> FdspMsgHandler;

 public:
    BaseAsyncSvcHandler();
    virtual ~BaseAsyncSvcHandler();

    void asyncReqt(const FDS_ProtocolInterface::AsyncHdr& header,
                   const std::string& payload) override;
    void asyncReqt(boost::shared_ptr<FDS_ProtocolInterface::AsyncHdr>& header,
                   boost::shared_ptr<std::string>& payload) override;

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

 protected:
    std::unordered_map<fpi::FDSPMsgTypeId, FdspMsgHandler, std::hash<int>> asyncReqHandlers_;
};
}  // namespace fds

#endif  // SOURCE_INCLUDE_NET_BASEASYNCSVCHANDLER_H_
