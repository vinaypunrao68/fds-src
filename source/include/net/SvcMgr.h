/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_SVCMGR_H_
#define SOURCE_INCLUDE_NET_SVCMGR_H_

#include <vector>
#include <unordered_map>
#include <fds_module.h>
#include <boost/shared_ptr.hpp>

// Forward declarations
namespace apache { namespace thrift { namespace transport {
    class TSocket;
    class TTransport;
}}}  // namespace apache::thrift::transport

namespace FDS_ProtocolInterface {
    class PlatNetSvcClient;
    class SvcInfo;
    class SvcUuid;
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

/**
* @brief Overall manager class for service layer
*/
struct SvcMgr : public Module {
    explicit SvcMgr(fpi::PlatNetSvcProcessorPtr processor);
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
    * @brief For sending async request via message passing
    *
    * @param svcUuid
    * @param header
    * @param payload
    */
    void sendAsyncSvcRequest(const fpi::SvcUuid &svcUuid,
                             fpi::AsyncHdrPtr &header, StringPtr &payload);
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
    * with OM should go through sendAsyncSvcRequest()
    * NOTE: This call always creates brand new connection against OM.
    *
    * @return OMSvcClient
    */
    fpi::OMSvcClientPtr getNewOMSvcClient() const;

    /**
    * @brief allocates thrift rpc client
    *
    * @tparam T
    * @param ip
    * @param port
    * @param blockOnConnect - if set, will block until connection succeeds
    *
    * @return 
    */
    template<class T>
    static boost::shared_ptr<T> allocRpcClient(const std::string ip,
                                               const int &port,
                                               const bool &blockOnConnect = false);
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
    std::unordered_map<fpi::SvcUuid, SvcHandlePtr, SvcUuidHash> svcHandleMap_;

    /* Server that accepts service layer messages */
    boost::shared_ptr<SvcServer> svcServer_;

    /* OM details */
    std::string omIp_;
    int omPort_;
    fpi::SvcUuid omSvcUuid_;

    /* Service base uuid */
    fpi::SvcUuid svcUuid_;
    /* Service port */
    int port_;
};
template<>
boost::shared_ptr<fpi::PlatNetSvcClient> SvcMgr::allocRpcClient(
    const std::string ip,
    const int &port,
    const bool &blockOnConnect);
template<>
boost::shared_ptr<fpi::OMSvcClient> SvcMgr::allocRpcClient(const std::string ip,
                                                                  const int &port,
                                                                  const bool &blockOnConnect);

/**
* @brief Wrapper around service information and service rpc client.
*/
struct SvcHandle {
    SvcHandle();
    explicit SvcHandle(const fpi::SvcInfo &info);
    virtual ~SvcHandle();

    void sendAsyncSvcRequest(fpi::AsyncHdrPtr &header, StringPtr &payload);

    void updateSvcHandle(const fpi::SvcInfo &newInfo);

    std::string logString() const;

    static std::string logString(const fpi::SvcInfo &info);

 protected:
    bool isSvcDown_() const;
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

