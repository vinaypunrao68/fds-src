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
#include <net/PlatNetSvcHandler.h>
#include <boost/shared_ptr.hpp>
#include <fdsp/OMSvc.h>

#define NET_SVC_RPC_CALL(eph, rpc, rpc_fn, ...)                                         \
            fds_panic("not supported...use svcmgr api");

#define EpInvokeRpc(SendIfT, func, ip, port, ...)                                       \
    do {                                                                                \
        /* XXX: Reconnecting with each call. This needs to be changed with              \
         * Stat Streaming refactoring. Considering current frequency is                 \
         * 2 to 5 minutes, making connection every time is temp solution                \
         */                                                                             \
        auto eph = allocRpcClient<SendIfT>(ip, port, SvcMgr::MAX_CONN_RETRIES);         \
        if (!eph) {                                                                     \
            GLOGERROR << "Failed to get the end point handle for ip: " << ip            \
                    << ", port: " << port;                                              \
            break;                                                                      \
        }                                                                               \
        try {                                                                           \
            GLOGDEBUG << "[Svc] sending RPC, ip: " << ip << ", port: " << port;         \
            eph->func(__VA_ARGS__);                                                     \
        } catch (std::exception & e) {                                                  \
            GLOGWARN << "[Svc] RPC error " << e.what();                                 \
        } catch (...) {                                                                 \
            GLOGWARN << "[Svc] Unknown RPC error ";                                     \
            fds_assert(!"Unknown exception");                                           \
        }                                                                               \
    } while (0);                                                                        \

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
struct SvcRequestPool;
struct SvcRequestCounters;
struct SvcRequestTracker;
struct SvcServer;
struct SvcServerListener; 
struct SvcHandle;
using SvcHandlePtr = boost::shared_ptr<SvcHandle>;
using StringPtr = boost::shared_ptr<std::string>;
class PlatNetSvcHandler;
struct DLT;
struct DLTManager;
struct DMTManager;
using DLTManagerPtr = boost::shared_ptr<DLTManager>;
using DMTManagerPtr = boost::shared_ptr<DMTManager>;
class TableColumn;
typedef TableColumn DltTokenGroup;
typedef boost::shared_ptr<DltTokenGroup> DltTokenGroupPtr;
typedef TableColumn DmtColumn;
typedef boost::shared_ptr<DmtColumn> DmtColumnPtr;
using OmDltUpdateRespCbType = std::function<void (const Error&)> ;
// Callback for DMT close
typedef std::function<void(Error &err)> DmtCloseCb;

/*--------------- Floating functions --------------*/
std::string logString(const FDS_ProtocolInterface::SvcInfo &info);
std::string logDetailedString(const FDS_ProtocolInterface::SvcInfo &info);
template<class T>
extern boost::shared_ptr<T> allocRpcClient(const std::string &ip, const int &port,
                                           const int &retryCnt);

/*--------------- Utility classes --------------*/
struct SvcUuidHash {
    std::size_t operator()(const fpi::SvcUuid& svcId) const;
};

struct SvcKeyException : std::exception {
    explicit SvcKeyException(const std::string &desc)
    {
        desc_ = desc;
    }
    const char* what() const noexcept {return desc_.c_str();}
 private:
    std::string desc_;
};

/*--------------- Typedefs --------------*/
using PlatNetSvcHandlerPtr = boost::shared_ptr<PlatNetSvcHandler>;
using SvcInfoPredicate = std::function<bool (const fpi::SvcInfo&)>;
using SvcHandleMap = std::unordered_map<fpi::SvcUuid, SvcHandlePtr, SvcUuidHash>;


/*--------------- Primary classes --------------*/
/**
* @brief Overall manager class for service layer
*/
struct SvcMgr : HasModuleProvider, Module {
    SvcMgr(CommonModuleProviderIf *moduleProvider,
           PlatNetSvcHandlerPtr handler,
           fpi::PlatNetSvcProcessorPtr processor,
           const fpi::SvcInfo &svcInfo);
    virtual ~SvcMgr();

    /* Module overrides */
    virtual int  mod_init(SysParams const *const p) override;
    virtual void mod_startup() override;
    virtual void mod_enable_service() override;
    virtual void mod_shutdown() override;

    /**
    * @brief Set listener for svc server
    *
    * @param listener
    */
    void setSvcServerListener(SvcServerListener* listener);

    /**
    * @brief 
    */
    SvcRequestPool* getSvcRequestMgr() const;

    /**
    * @brief 
    */
    SvcRequestCounters* getSvcRequestCntrs() const;

    /**
    * @brief 
    */
    SvcRequestTracker* getSvcRequestTracker() const;

    /**
    * @brief Start the server.  Blocks untils server starts listening.
    *
    * @return OK if we are able to start server.
    */
    Error startServer();

    /**
    * @brief
    */
    void stopServer();

    /**
    * @brief Updates service handles based on the entries from service map
    *
    * @param entries
    */
    void updateSvcMap(const std::vector<fpi::SvcInfo> &entries);

    /**
    * @brief Returns the cached service info entries
    *
    * @param entries
    */
    void getSvcMap(std::vector<fpi::SvcInfo> &entries);
    

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
    * @brief  Returns property for service with svcUuid
    * @throws when service or key are not found
    */
    template<class T>
    T getSvcProperty(const fpi::SvcUuid &svcUuid, const std::string& key) {
        fpi::SvcInfo svcInfo;
        bool found = getSvcInfo(svcUuid, svcInfo);
        if (!found) {
            GLOGWARN << "Unknown svcuuid: " << svcUuid.svc_uuid;
            /* In this case we can lookup from OM, for now throw an exception */
            throw SvcKeyException("unknown service");
        }
        auto itr = svcInfo.props.find(key);
        if (itr == svcInfo.props.end()) {
            GLOGWARN << "Unknown key: " << key;
            throw SvcKeyException(key + " not found");
        }
        return boost::lexical_cast<T>(itr->second);
    }

    /**
    * @brief Returns svc property for key.  If key not found returns default value
    * @param svcUuid
    * @param key
    * @param defaultVal
    */
    template<class T>
    T getSvcProperty(const fpi::SvcUuid &svcUuid,
                     const std::string& key, const T &defaultVal) {
        try {
            return getSvcProperty<T>(svcUuid, key);
        } catch (std::exception &e) {
            return defaultVal;
        }
    }

    /**
    * @brief Returns svc base uuid
    */
    fpi::SvcUuid getSelfSvcUuid() const;

    /**
    * @brief 
    *
    */
    fpi::SvcInfo getSelfSvcInfo() const;

    std::string getSelfSvcName() const;

    /**
    * @brief Convenience function for returning mapped platform uuid
    */
    fpi::SvcUuid getMappedSelfPlatformUuid() const;

    /**
    * @brief Convenience function for returning mapped platform port
    */
    int getMappedSelfPlatformPort() const;

    /**
    * @brief 
    */
    bool getSvcInfo(const fpi::SvcUuid &svcUuid, fpi::SvcInfo& info) const;

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
    void getOmIPPort(std::string &omIp, fds_uint32_t &port) const;

    /**
    * @brief
    */
    fpi::SvcUuid getOmSvcUuid() const;

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
    * @brief Return current dlt
    */
    fds_uint64_t getDMTVersion();

    /**
    * @brief Return current dlt
    */
    const DLT* getCurrentDLT();

    /**
    * @brief Returns dlt manager
    */
    DLTManagerPtr getDltManager() { return dltMgr_; }

    /**
    * @brief Returns dmt manager
    */
    DMTManagerPtr getDmtManager() { return dmtMgr_; }

    /**
    * @brief 
    *
    * @param objId
    *
    * @return 
    */
    DltTokenGroupPtr getDLTNodesForDoidKey(const ObjectID &objId);

    /**
    * @brief 
    *
    * @param vol_id
    *
    * @return 
    */
    DmtColumnPtr getDMTNodesForVolume(fds_volid_t vol_id);

    /**
    * @brief 
    *
    * @param vol_id
    * @param dmt_version
    *
    * @return 
    */
    DmtColumnPtr getDMTNodesForVolume(fds_volid_t vol_id,
                                      fds_uint64_t dmt_version);
    /**
    * @brief 
    *
    * @return 
    */
    bool hasCommittedDMT() const;

    Error updateDlt(bool dlt_type, std::string& dlt_data, OmDltUpdateRespCbType cb);
    Error updateDmt(bool dmt_type, std::string& dmt_data);
    /**
    * @brief Gets dlt from OM and upadtes dlt manager
    *
    * @param maxAttempts - maximum attempts to try.  Set to -1 to try a lot
    *
    * @return
    */
    Error getDLT(int maxAttempts = -1);
    /**
    * @brief Gets dmt from OM and upadtes dmt manager
    *
    * @param maxAttempts - maximum attempts to try.  Set to -1 to try a lot
    *
    * @return
    */
    Error getDMT(int maxAttempts = -1);

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

    /**
     * @brief Accessor for service request handler
     */
    inline PlatNetSvcHandlerPtr getSvcRequestHandler() const {
        return svcRequestHandler_;
    }

    /**
    * @brief maps svcName->FDSP_MgrIdType
    *
    * @param svcName
    *
    * @return 
    */
    static fpi::FDSP_MgrIdType mapToSvcType(const std::string &svcName); 

    /**
    * @brief svcType -> svcName mapping
    *
    * @param svcType
    *
    * @return 
    */
    static std::string mapToSvcName(const fpi::FDSP_MgrIdType &svcType);

    /**
    * @brief maps svcUuid -> svcUuid:svcName string
    *
    * @param svcUuid
    *
    * @return 
    */
    static std::string mapToSvcUuidAndName(const fpi::SvcUuid &svcUuid);

    /**
    * @brief Based on the svcType and platforPort (as the base) determines what the service
    * port should be
    *
    * @param platformPort
    * @param svcType
    *
    * @return 
    */
    static int32_t mapToSvcPort(const int32_t &platformPort, const fpi::FDSP_MgrIdType& svcType);

    /**
    * @brief maps input (in) svc uuid to another svc uuid matching svcType
    *
    * @param in
    * @param svcType
    *
    * @return 
    */
    static fpi::SvcUuid mapToSvcUuid(const fpi::SvcUuid &in,
                                     const fpi::FDSP_MgrIdType& svcType);

    /**
     * @brief Method to retrieve the DMT for the service.
     * Caller will provide a Thrift interface object and the method will
     * figure out a connection and populate the data.
     * It should block until the data is retrieved.
     * TODO(Neil): should throw an exception for timeout?
     */
    void getDMTData(::FDS_ProtocolInterface::CtrlNotifyDMTUpdate &fdsp_dmt);

    /**
     * @brief Method to retrieve the DLT for the service.
     * Caller will provide a Thrift interface object and the method will
     * figure out a connection and populate the data.
     * It should block until the data is retrieved.
     * TODO(Neil): should throw an exception for timeout?
     */
    void getDLTData(::FDS_ProtocolInterface::CtrlNotifyDLTUpdate &fdsp_dlt);

    /**
    * @brief Do an async notification to OM that service is down
    *
    * @param info
    */
    void notifyOMSvcIsDown(const fpi::SvcInfo &info);

    /**
     * Sets unreachable fault injection for testing at
     * a given frequency.
     */
    void setUnreachableInjection(float frequency);

    /**
    * @brief Minimum connection retries
    */
    static int32_t MIN_CONN_RETRIES;
    /**
    * @brief Max connection retries
    */
    static int32_t MAX_CONN_RETRIES;

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
    mutable fds_mutex svcHandleMapLock_;
    /* Map of service handles */
    SvcHandleMap svcHandleMap_;

    /* Request handler */
    PlatNetSvcHandlerPtr svcRequestHandler_;
    /* Server that accepts service layer messages */
    boost::shared_ptr<SvcServer> svcServer_;

    /* Manage for service requests */
    SvcRequestPool *svcRequestMgr_;

    /* OM details */
    std::string omIp_;
    int omPort_;
    fpi::SvcUuid omSvcUuid_;

    /* Self service information */
    fpi::SvcInfo svcInfo_;

    /* For executing task in a threadpool in a synchronized manner */
    SynchronizedTaskExecutor<uint64_t> *taskExecutor_;

    /* Dlt manager */
    DLTManagerPtr dltMgr_;
    /* Dmt manager */
    DMTManagerPtr dmtMgr_;


};

/**
* @brief Wrapper around service information and service rpc client.
*/
struct SvcHandle : HasModuleProvider {
    SvcHandle(CommonModuleProviderIf *moduleProvider,
              const fpi::SvcInfo &info);
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

    /**
    * @brief 
    *
    * @param info
    */
    void getSvcInfo(fpi::SvcInfo &info) const;

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
    mutable fds_mutex lock_;
    /* Service information */
    fpi::SvcInfo svcInfo_;
    /* Rpc client.  Typcially this is PlatNetSvcClient */
    fpi::PlatNetSvcClientPtr svcClient_;
};

}  // namespace fds
#endif  // SOURCE_INCLUDE_NET_SVCMGR_H_

