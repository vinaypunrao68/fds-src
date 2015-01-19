/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_SVCMGR_H_
#define SOURCE_INCLUDE_NET_SVCMGR_H_

#include <vector>
#include <unordered_map>
#include <fds_module.h>
#include <boost/shared_ptr.hpp>
#include <fdsp/fds_service_types.h>

// Forward declarations
namespace apache { namespace thrift { namespace transport {
    class TSocket;
    class TTransport;
}}}  // namespace apache::thrift::transport

#if 0
namespace FDS_ProtocolInterface {
    class PlatNetSvcClient;
    class SvcInfo;
    class SvcUuid;
    class SvcUuid;
    class AsyncHdr;
    using AsyncHdrPtr = boost::shared_ptr<AsyncHdr>;
}  // namespace FDS_ProtocolInterface
#endif
namespace FDS_ProtocolInterface {
class PlatNetSvcClient;
using PlatNetSvcClientPtr = boost::shared_ptr<PlatNetSvcClient>;
}
namespace fpi = FDS_ProtocolInterface;

namespace fds {

namespace bo  = boost;
namespace tt  = apache::thrift::transport;
namespace tp  = apache::thrift::protocol;

class fds_mutex;
struct SvcHandle;
using SvcHandlePtr = boost::shared_ptr<SvcHandle>;
using StringPtr = boost::shared_ptr<std::string>;

struct SvcUuidHash {
    std::size_t operator()(const fpi::SvcUuid& svcId) const {
        return svcId.svc_uuid;
    }
};

/**
* @brief Overall manager class for service layer
*/
struct SvcMgr : public Module {
    SvcMgr();
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

    /* This lock protects both svcMap_ and svcHandleMap_.  Both svcMap_ and svcHandleMap_ are
     * typically used together
     */
    fds_mutex svcHandleMapLock_;
    /* Map of service handles */
    std::unordered_map<fpi::SvcUuid, SvcHandlePtr, SvcUuidHash> svcHandleMap_;
};

/**
* @brief Wrapper around service information and service rpc client.
*/
struct SvcHandle {
    SvcHandle();
    explicit SvcHandle(const fpi::SvcInfo &info);
    virtual ~SvcHandle();

    void sendAsyncSvcRequest(fpi::AsyncHdrPtr &header, StringPtr &payload);

    void updateSvcHandle(const fpi::SvcInfo &newInfo);

    std::string logString();

    static std::string logString(const fpi::SvcInfo &info);

 protected:
    void allocSvcClient_();
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

