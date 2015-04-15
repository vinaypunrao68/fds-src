/* Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_PLATNETSVCHANDLER_H_
#define SOURCE_INCLUDE_NET_PLATNETSVCHANDLER_H_

#include <string>
#include <list>
#include <utility>
#include <unordered_map>
#include <fdsp/svc_api_types.h>
#include <fdsp_utils.h>
#include <fds_module_provider.h>
#include <fds_module.h>
#include <concurrency/SynchronizedTaskExecutor.hpp>
#include <fdsp/PlatNetSvc.h>
/*
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

struct PlatNetSvcHandler : HasModuleProvider,
    virtual FDS_ProtocolInterface::PlatNetSvcIf, Module
{
    typedef std::function<void (boost::shared_ptr<FDS_ProtocolInterface::AsyncHdr>&,
                                boost::shared_ptr<std::string>&)> FdspMsgHandler;
    /* Handler state */
    enum State {
        /* In thi state requests are queued up to be replayed when you
         * transition to ACCEPT_REQUESTS */
        DEFER_REQUESTS, 
        /* Accept requests */
        ACCEPT_REQUESTS,
        /* Drop requests */
        DROP_REQUESTS
    };

    explicit PlatNetSvcHandler(CommonModuleProviderIf *provider);
    virtual ~PlatNetSvcHandler();

    virtual int mod_init(SysParams const *const param) override;
    virtual void mod_startup() override;
    virtual void mod_shutdown() override;

    void setHandlerState(PlatNetSvcHandler::State newState);

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
    /* Typically whether registration with OM is complete or not.  When
     * registration isn't complete all incoming async requests get queued up
     */
    using AsyncReqPair = std::pair<fpi::AsyncHdrPtr, StringPtr>;
    std::atomic<State> handlerState_;
    std::list<AsyncReqPair> deferredReqs_; 

    /* Request handlers */
    std::unordered_map<fpi::FDSPMsgTypeId, FdspMsgHandler, std::hash<int>> asyncReqHandlers_;
    SynchronizedTaskExecutor<uint64_t>  * taskExecutor_;


    virtual void allUuidBinding(const fpi::UuidBindMsg& mine);
    virtual void allUuidBinding(boost::shared_ptr<fpi::UuidBindMsg>& mine);  // NOLINT

    virtual void notifyNodeActive(const fpi::FDSP_ActivateNodeType &info);
    virtual void notifyNodeActive(boost::shared_ptr<fpi::FDSP_ActivateNodeType> &);

    virtual void notifyNodeInfo(std::vector<fpi::NodeInfoMsg> & _return,  // NOLINT
                        const fpi::NodeInfoMsg& info, const bool bcast);
    virtual void notifyNodeInfo(std::vector<fpi::NodeInfoMsg> & _return,  // NOLINT
                        boost::shared_ptr<fpi::NodeInfoMsg>& info,  // NOLINT
                        boost::shared_ptr<bool>& bcast);

    virtual void getDomainNodes(fpi::DomainNodes &ret, const fpi::DomainNodes &d);
    virtual void getDomainNodes(fpi::DomainNodes &ret, fpi::DomainNodesPtr &d);

    virtual fpi::ServiceStatus getStatus(const int32_t nullarg);
    virtual fpi::ServiceStatus getStatus(boost::shared_ptr<int32_t>& nullarg);  // NOLINT

    virtual void getCounters(std::map<std::string, int64_t> &, const std::string &);
    virtual void getCounters(std::map<std::string, int64_t> & _return,
                             boost::shared_ptr<std::string>& id);

    virtual void resetCounters(const std::string& id);
    virtual void resetCounters(boost::shared_ptr<std::string>& id);

    virtual void setConfigVal(const std::string& id, const int64_t val);
    virtual void setConfigVal(boost::shared_ptr<std::string>& id,  // NOLINT
                              boost::shared_ptr<int64_t>& val);

    virtual void setFlag(const std::string& id, const int64_t value);
    virtual void setFlag(boost::shared_ptr<std::string>& id,
                 boost::shared_ptr<int64_t>& value);  // NOLINT

    virtual int64_t getFlag(const std::string& id);
    virtual int64_t getFlag(boost::shared_ptr<std::string>& id);  // NOLINT

    virtual void getFlags(std::map<std::string, int64_t> &, const int32_t nullarg);  // NOLINT
    virtual void getFlags(std::map<std::string, int64_t> & _return, boost::shared_ptr<int32_t>& nullarg);  // NOLINT

    virtual void getProperty(std::string& _return, const std::string& name);
    virtual void getProperty(std::string& _return, boost::shared_ptr<std::string>& name);
    virtual void getProperties(std::map<std::string, std::string> & _return, const int32_t nullarg);  // NOLINT
    virtual void getProperties(std::map<std::string, std::string> & _return, boost::shared_ptr<int32_t>& nullarg);   // NOLINT


    virtual bool setFault(const std::string& command);
    virtual bool setFault(boost::shared_ptr<std::string>& command);  // NOLINT
    void getStatus(fpi::AsyncHdrPtr &header,
                   fpi::GetSvcStatusMsgPtr &statusMsg);
    virtual void updateSvcMap(fpi::AsyncHdrPtr &header,
                              fpi::UpdateSvcMapMsgPtr &svcMapMsg);
};

using PlatNetSvcHandlerPtr = boost::shared_ptr<PlatNetSvcHandler>;

}  // namespace fds
#endif  // SOURCE_INCLUDE_NET_PLATNETSVCHANDLER_H_
