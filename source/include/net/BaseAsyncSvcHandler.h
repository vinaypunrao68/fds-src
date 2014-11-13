/* Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_BASEASYNCSVCHANDLER_H_
#define SOURCE_INCLUDE_NET_BASEASYNCSVCHANDLER_H_

#include <string>
#include <unordered_map>
#include <fdsp/fds_service_types.h>
#include <fdsp/BaseAsyncSvc.h>
#include <fdsp_utils.h>
#include <net/net-service.h>
#include <net/net-service-tmpl.hpp>
#include <platform/node-inventory.h>
#include <net/SvcRequestPool.h>
#include <net/SvcRequest.h>

/**
 * Use this macro for registering FDSP message handlers
 * @param FDSPMsgT - fdsp message type
 * @param func - Name of the function to invoke
 */
#define REGISTER_FDSP_MSG_HANDLER(FDSPMsgT, func) \
    asyncReqHandlers_[FDSP_MSG_TYPEID(FDSPMsgT)] = \
    [this] (boost::shared_ptr<FDS_ProtocolInterface::AsyncHdr>& header, \
        boost::shared_ptr<std::string>& payloadBuf) \
    { \
        boost::shared_ptr<FDSPMsgT> payload; \
        fds::deserializeFdspMsg(payloadBuf, payload); \
        SVCPERF(header->rqHndlrTs = util::getTimeStampNanos()); \
        func(header, payload); \
    }

#define REGISTER_FDSP_MSG_HANDLER_GENERIC(platsvc, FDSPMsgT, func)  \
    platsvc->asyncReqHandlers_[FDSP_MSG_TYPEID(FDSPMsgT)] = \
    [this] (boost::shared_ptr<FDS_ProtocolInterface::AsyncHdr>& header, \
        boost::shared_ptr<std::string>& payloadBuf) \
    { \
        boost::shared_ptr<FDSPMsgT> payload; \
        fds::deserializeFdspMsg(payloadBuf, payload); \
        SVCPERF(header->rqHndlrTs = util::getTimeStampNanos()); \
        func(header, payload); \
    }

// note : CALLBACK & CALLBACK2 are same
#define BIND_MSG_CALLBACK(func, header , ...) \
    std::bind(&func, this, header, ##__VA_ARGS__ , std::placeholders::_1, \
              std::placeholders::_2);

#define BIND_MSG_CALLBACK2(func, header , ...) \
    std::bind(&func, this, header, ##__VA_ARGS__ , std::placeholders::_1, \
              std::placeholders::_2);

#define BIND_MSG_CALLBACK3(func, header , ...) \
    std::bind(&func, this, header, ##__VA_ARGS__ , std::placeholders::_1, \
              std::placeholders::_2, std::placeholders::_3);


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

    virtual void uuidBind(fpi::AsyncHdr &_return, const fpi::UuidBindMsg& msg) override;
    virtual void uuidBind(fpi::AsyncHdr &_return,
                          boost::shared_ptr<fpi::UuidBindMsg>& msg) override;

    static void asyncRespHandler(
        boost::shared_ptr<FDS_ProtocolInterface::AsyncHdr>& header,
        boost::shared_ptr<std::string>& payload);


    /**
    * @brief Sends async response to the src identified by req_hdr
    *
    * @tparam PayloadT Paylaod type (FDSP message response)
    * @param req_hdr - header from the source
    * @param msgTypeId - FDSP message id.  Use macro FDSP_MSG_TYPEID()
    * @param payload - payload to send
    */
    template<class PayloadT>
    static void sendAsyncResp(boost::shared_ptr<fpi::AsyncHdr>& req_hdr,
                              const fpi::FDSPMsgTypeId &msgTypeId,
                              boost::shared_ptr<PayloadT>& payload) {
        sendAsyncResp(*req_hdr, msgTypeId, *payload);
    }

    template<class PayloadT>
    static void sendAsyncResp(const fpi::AsyncHdr& req_hdr,
                              const fpi::FDSPMsgTypeId &msgTypeId,
                              const PayloadT& payload)
    {
        GLOGDEBUG;
        int minor;
        auto resp_hdr = NetMgr::ep_swap_header(req_hdr);
        resp_hdr.msg_type_id = msgTypeId;
        SVCPERF(resp_hdr.rspSerStartTs = util::getTimeStampNanos());

        bo::shared_ptr<tt::TMemoryBuffer> buffer(new tt::TMemoryBuffer());
        bo::shared_ptr<tp::TProtocol> binary_buf(new tp::TBinaryProtocol(buffer));

        auto written = payload.write(binary_buf.get());
        fds_verify(written > 0);

        extern const NodeUuid gl_OmUuid;
        if (resp_hdr.msg_dst_uuid.svc_uuid == gl_OmUuid.uuid_get_val()) {
            minor = 1;  // NET_SVC_CTRL;
        } else {
            minor = 0;
        }
        auto ep = NetMgr::ep_mgr_singleton()->\
                  svc_get_handle<fpi::BaseAsyncSvcClient>(resp_hdr.msg_dst_uuid, 0 , minor);
        if (ep == nullptr) {
            GLOGERROR << "Null destination client: " << resp_hdr.msg_dst_uuid.svc_uuid;
            return;
        }
        fiu_do_on("svc.use.lftp",
                  gSvcRequestPool->getSvcSendThreadpool()->\
                  scheduleWithAffinity(ep->getAffinity(), \
                                       &SvcRequestIf::sendRespWork, \
                                       ep, resp_hdr, buffer); \
                  return;);

        // TODO(Rao): Enable this code instead of the try..catch.  I was getting
        // a compiler error when I enabled the following code.
        // NET_SVC_RPC_CALL(ep, client, fpi::BaseAsyncSvcClient::asyncResp,
        // resp_hdr, buffer->getBufferAsString());
        try {
            SVCPERF(resp_hdr.rspSendStartTs = util::getTimeStampNanos());
            ep->svc_rpc<fpi::BaseAsyncSvcClient>()->\
                asyncResp(resp_hdr, buffer->getBufferAsString());
        } catch(std::exception &e) {
            GLOGERROR << logString(resp_hdr) << " exception: " << e.what();
            ep->ep_handle_net_error();
            fds_panic("uh oh");
        } catch(...) {
            ep->ep_handle_net_error();
            DBG(std::exception_ptr eptr = std::current_exception());
            fds_panic("uh oh");
        }
    }

    // protected:
    std::unordered_map<fpi::FDSPMsgTypeId, FdspMsgHandler, std::hash<int>> asyncReqHandlers_;
};
}  // namespace fds

#endif  // SOURCE_INCLUDE_NET_BASEASYNCSVCHANDLER_H_
