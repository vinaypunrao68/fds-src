/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_SVCMGR_H_
#define SOURCE_INCLUDE_NET_SVCMGR_H_

#include <vector>
#include <unordered_map>
#include <fds_module.h>
// TODO(Rao): Do forward decl here
#include <concurrency/SynchronizedTaskExecutor.hpp>
#include <boost/shared_ptr.hpp>

#define EpInvokeRpc(SendIfT, func, svc_id, maj, min, ...)                       \
    do {                                                                        \
        try {                                                                   \
            /* TODO(Rao): Fix this*/                                            \
            fds_panic("not imple");                                             \
            GLOGDEBUG << "[Svc] sent RPC "                                      \
                << std::hex << svc_id.svc_uuid << std::dec;                     \
        } catch(std::exception &e) {                                            \
            GLOGDEBUG << "[Svc] RPC error " << e.what();                        \
        } catch(...) {                                                          \
            GLOGDEBUG << "[Svc] Unknown RPC error ";                            \
            fds_assert(!"Unknown exception");                                   \
        }                                                                       \
    } while (0)

// Forward declarations
namespace apache { namespace thrift { namespace transport {
    class TSocket;
    class TTransport;
}}}  // namespace apache::thrift::transport

namespace FDS_ProtocolInterface {
    class PlatNetSvcClient;
    class SvcInfo;
    class SvcUuid;
    class AsyncHdr;
    using AsyncHdrPtr = boost::shared_ptr<AsyncHdr>;
}  // namespace FDS_ProtocolInterface
namespace FDS_ProtocolInterface {
class PlatNetSvcClient;
using PlatNetSvcClientPtr = boost::shared_ptr<PlatNetSvcClient>;
class PlatNetSvcProcessor;
using PlatNetSvcProcessorPtr = boost::shared_ptr<PlatNetSvcProcessor>;
class OMSvcClient;
using OMSvcClientPtr = boost::shared_ptr<OMSvcClient>;
}
namespace fpi = FDS_ProtocolInterface;

namespace fds {

namespace bo  = boost;
namespace tt  = apache::thrift::transport;
namespace tp  = apache::thrift::protocol;

class fds_mutex;
struct SvcServer;
struct SvcHandle;
using SvcHandlePtr = boost::shared_ptr<SvcHandle>;
using StringPtr = boost::shared_ptr<std::string>;

struct SvcUuidHash {
    std::size_t operator()(const fpi::SvcUuid& svcId) const;
};

using SvcInfoPredicate = std::function<bool (const fpi::SvcInfo&)>;
using SvcHandleMap = std::unordered_map<fpi::SvcUuid, SvcHandlePtr, SvcUuidHash>;

/**
* @brief Overall manager class for service layer
*/
struct SvcMgr : public Module {
    explicit SvcMgr(fpi::PlatNetSvcProcessorPtr processor, const fpi::SvcInfo &svcInfo);
    virtual ~SvcMgr();

    /* Module overrides */
    virtual int  mod_init(SysParams const *const p) override;
    virtual void mod_startup() override;
    virtual void mod_enable_service() override;
    virtual void mod_shutdown() override;

    /**
    * @brief Updates service handles based on the entries from service map
    *
    * @param entries
    */
    void updateSvcMap(const std::vector<fpi::SvcInfo> &entries);
    

    /**
    * @brief For sending async message via message passing
    *
    * @param header
    * @param payload
    */
    void sendAsyncSvcReqMessage(fpi::AsyncHdrPtr &header, StringPtr &payload);

    /**
    * @brief 
    *
    * @param header
    * @param payload
    */
    void sendAsyncSvcRespMessage(fpi::AsyncHdrPtr &header, StringPtr &payload);

    /**
    * @brief Brodcasts the payload to all services matching predicate
    *
    * @param header
    * @param payload
    * @param predicate
    */
    void broadcastAsyncSvcReqMessage(fpi::AsyncHdrPtr &header,
                                  StringPtr &payload,
                                  const SvcInfoPredicate& predicate);
    /**
    * @brief For posting errors when we fail to send on service handle
    *
    * @param header
    */
    void postSvcSendError(fpi::AsyncHdrPtr &header);

    /**
    * @brief Returns svc base uuid
    */
    fpi::SvcUuid getSvcUuid() const;

    /**
    * @brief Return svc port
    */
    int getSvcPort() const;

    /**
    * @brief Return om ip, port
    *
    * @param omIp
    * @param port
    */
    void getOmIPPort(std::string &omIp, int &port);

    /**
    * @brief Constructs new client against OM and returns it.  This call will block until
    * connection aginst OM succeeds.
    * Client can cache the returned OM rpc handle.  Client is responsible for handling
    * disconnects and reconnects.  Service layer will not do this for you.  If you use
    * svc handle async interfaces service layer will handle disconnects/reconnects.
    * Primary use case for this call is during registration with OM.  All other interactions
    * with OM should go through sendAsyncSvcReqMessage()
    * NOTE: This call always creates brand new connection against OM.
    *
    * @return OMSvcClient
    */
    fpi::OMSvcClientPtr getNewOMSvcClient() const;

    /**
    * @brief Returns task executor
    *
    * @return 
    */
    SynchronizedTaskExecutor<uint64_t>* getTaskExecutor();

    /**
    * @brief Return true if e is an error service layer should handle
    *
    * @param e
    *
    * @return 
    */
    bool isSvcActionableError(const Error &e);

    /**
    * @brief Handle error e from source service srcSvc
    *
    * @param srcSvc
    * @param e
    */
    void handleSvcError(const fpi::SvcUuid &srcSvc, const Error &e);

 protected:
    /**
    * @brief For getting service handle.
    * NOTE: Assumes the lock is held
    *
    * @param svcUuid
    * @param handle
    *
    * @return  True if handle is found
    */
    bool getSvcHandle_(const fpi::SvcUuid &svcUuid, SvcHandlePtr& handle) const;

    /* This lock protects svcHandleMap_ */
    fds_mutex svcHandleMapLock_;
    /* Map of service handles */
    SvcHandleMap svcHandleMap_;

    /* Server that accepts service layer messages */
    boost::shared_ptr<SvcServer> svcServer_;

    /* OM details */
    std::string omIp_;
    int omPort_;
    fpi::SvcUuid omSvcUuid_;

    /* Self service information */
    fpi::SvcInfo svcInfo_;

    /* For executing task in a threadpool in a synchronized manner */
    SynchronizedTaskExecutor<uint64_t> *taskExecutor_;
};

/**
* @brief Wrapper around service information and service rpc client.
*/
struct SvcHandle {
    SvcHandle();
    explicit SvcHandle(const fpi::SvcInfo &info);
    virtual ~SvcHandle();

    /**
    * @brief Use it for sending async request messages.  This uses asynReqt() interface for 
    * sending the message
    *
    * @param header
    * @param payload
    */
    void sendAsyncSvcReqMessage(fpi::AsyncHdrPtr &header, StringPtr &payload);

    /**
    * @brief Use it for sending async response messages.  This uses asynResp() interface for
    * sending the message
    *
    * @param header
    * @param payload
    */
    void sendAsyncSvcRespMessage(fpi::AsyncHdrPtr &header, StringPtr &payload);

    /**
    * @brief Use it for sending async request messages based on predicate.  This uses
    * asynReqt() interface for 
    * sending the message
    *
    * @param header
    * @param payload
    * @param predicate
    */
    void sendAsyncSvcReqMessageOnPredicate(fpi::AsyncHdrPtr &header,
                                        StringPtr &payload,
                                        const SvcInfoPredicate& predicate);

    /**
    * @brief Updates the service handle based on newInfo
    *
    * @param newInfo
    */
    void updateSvcHandle(const fpi::SvcInfo &newInfo);

    std::string logString() const;

 protected:
    /**
    * @brief Common interface for sending asyn service messages.  isAsyncReqt determines
    * whether it's a request or response
    *
    * @param isAsyncReqt
    * @param header
    * @param payload
    *
    * @return 
    */
    bool sendAsyncSvcMessageCommon_(bool isAsyncReqt,
                                    fpi::AsyncHdrPtr &header,
                                    StringPtr &payload);
    /**
    * @brief Checks if service is down or not
    */
    bool isSvcDown_() const;

    /**
    * @brief Marks the service down.  Set rpcClient_ to null
    */
    void markSvcDown_();

    /* Lock for protecting svcInfo_ and rpcClient_ */
    fds_mutex lock_;
    /* Service information */
    fpi::SvcInfo svcInfo_;
    /* Rpc client.  Typcially this is PlatNetSvcClient */
    fpi::PlatNetSvcClientPtr svcClient_;
};

}  // namespace fds
#endif  // SOURCE_INCLUDE_NET_SVCMGR_H_

