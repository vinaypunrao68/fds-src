/* Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_BASEASYNCSVCHANDLER_H_
#define SOURCE_INCLUDE_NET_BASEASYNCSVCHANDLER_H_

#include <string>
#include <unordered_map>
#include <fdsp/fds_service_types.h>
#include <fdsp/BaseAsyncSvc.h>
#include <fdsp_utils.h>
#include <fds_module_provider.h>
#include <fds_module.h>
#include <concurrency/SynchronizedTaskExecutor.hpp>

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

/* Forward declarations */
struct SvcRequestTracker;
using StringPtr = boost::shared_ptr<std::string>;

class BaseAsyncSvcHandler : public HasModuleProvider,
    virtual public FDS_ProtocolInterface::BaseAsyncSvcIf,
    public Module {
    typedef std::function<void (boost::shared_ptr<FDS_ProtocolInterface::AsyncHdr>&,
                                boost::shared_ptr<std::string>&)> FdspMsgHandler;

 public:
    explicit BaseAsyncSvcHandler(CommonModuleProviderIf *provider);
    virtual ~BaseAsyncSvcHandler();

    virtual int mod_init(SysParams const *const param) override;
    virtual void mod_startup() override;
    virtual void mod_shutdown() override;

    void setTaskExecutor(SynchronizedTaskExecutor<uint64_t>  * taskExecutor);

    void asyncReqt(const FDS_ProtocolInterface::AsyncHdr& header,
                   const std::string& payload) override;
    void asyncReqt(boost::shared_ptr<FDS_ProtocolInterface::AsyncHdr>& header,
                   boost::shared_ptr<std::string>& payload) override;

    // void asyncResp2(boost::shared_ptr<FDS_ProtocolInterface::AsyncHdr>& header,
                    // boost::shared_ptr<std::string>& payload);
    void asyncResp(const FDS_ProtocolInterface::AsyncHdr& header,
            const std::string& payload) override;
    void asyncResp(boost::shared_ptr<FDS_ProtocolInterface::AsyncHdr>& header,
                   boost::shared_ptr<std::string>& payload) override;

    void uuidBind(fpi::AsyncHdr &_return, const fpi::UuidBindMsg& msg) override;
    void uuidBind(fpi::AsyncHdr &_return,
                          boost::shared_ptr<fpi::UuidBindMsg>& msg) override;

    static void asyncRespHandler(SvcRequestTracker* reqTracker,
        boost::shared_ptr<FDS_ProtocolInterface::AsyncHdr>& header,
        boost::shared_ptr<std::string>& payload);


    void sendAsyncResp_(const fpi::AsyncHdr& reqHdr,
                        const fpi::FDSPMsgTypeId &msgTypeId,
                        StringPtr &payload);
    /**
    * @brief Sends async response to the src identified by req_hdr
    *
    * @tparam PayloadT Paylaod type (FDSP message response)
    * @param req_hdr - header from the source
    * @param msgTypeId - FDSP message id.  Use macro FDSP_MSG_TYPEID()
    * @param payload - payload to send
    */
    template<class PayloadT>
    void sendAsyncResp(boost::shared_ptr<fpi::AsyncHdr>& reqHdr,
                              const fpi::FDSPMsgTypeId &msgTypeId,
                              boost::shared_ptr<PayloadT>& payload) {
        sendAsyncResp(*reqHdr, msgTypeId, *payload);
    }

    template<class PayloadT>
    void sendAsyncResp(const fpi::AsyncHdr& reqHdr,
                              const fpi::FDSPMsgTypeId &msgTypeId,
                              const PayloadT& payload)
    {
        GLOGDEBUG;

        boost::shared_ptr<std::string> respBuf;
        fds::serializeFdspMsg(payload, respBuf);
        this->sendAsyncResp_(reqHdr, msgTypeId, respBuf);
    }

    // protected:
    std::unordered_map<fpi::FDSPMsgTypeId, FdspMsgHandler, std::hash<int>> asyncReqHandlers_;
    SynchronizedTaskExecutor<uint64_t>  * taskExecutor_;
};
}  // namespace fds

#endif  // SOURCE_INCLUDE_NET_BASEASYNCSVCHANDLER_H_
