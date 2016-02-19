/* Copyright 2015 Formation Data Systems, Inc.
 */

#include <vector>
#include <string>
#include <sstream>
#include <limits>
#include <concurrency/Mutex.h>
#include <arpa/inet.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/protocol/TMultiplexedProtocol.h>
#include "fdsp/common_constants.h"
#include <fdsp/PlatNetSvc.h>
#include <fdsp/health_monitoring_api_types.h>
#include <net/PlatNetSvcHandler.h>
#include <fdsp/OMSvc.h>
#include <net/fdssocket.h>
#include <fdsp/Streaming.h>
#include <fdsp/ConfigurationService.h>
#include <fdsp_utils.h>
#include <fds_module_provider.h>
#include <net/SvcRequestPool.h>
#include <net/SvcServer.h>
#include <net/SvcMgr.h>
#include <dlt.h>
#include <fds_dmt.h>
#include <fiu-control.h>
#include <util/fiu_util.h>
#include <json/json.h>

namespace fds {

/*********************************************************************************
 * Static variables
 ********************************************************************************/
const int32_t SvcMgr::MIN_CONN_RETRIES = 3;
const int32_t SvcMgr::MAX_CONN_RETRIES = 1024;

/*********************************************************************************
 * Free functions
 ********************************************************************************/
std::string logString(const FDS_ProtocolInterface::SvcInfo &info)
{
    std::stringstream ss;
    ss << "Svc handle svc_uuid: "
        << SvcMgr::mapToSvcUuidAndName(info.svc_id.svc_uuid)
        << " ip: " << info.ip << " port: " << info.svc_port
        << " incarnation: " << info.incarnationNo << " status: " << info.svc_status;
    return ss.str();
}

std::string logDetailedString(const FDS_ProtocolInterface::SvcInfo &info)
{
    std::stringstream ss;
    ss << fds::logString(info) << "\n";
    auto &props = info.props;
    for (auto &kv : props) {
        ss << kv.first << " : " << kv.second << std::endl;
    }
    return ss.str();
}

/**
 * Conditionally (guarded by feature toggle) creates multiplexed service.client.
 * Multiplexed servers enable us to break up existing large services.
 * Multiplexed servers enable the introduction of new services for an IP/port
 * when a change to an existing service would be backward incompatible.
 */
template<class ClientT>
boost::shared_ptr<ClientT> allocRpcClient(const std::string &ip, const int &port,
    const int &retryCnt,
    const std::string &strThriftServiceName,
    const boost::shared_ptr<FdsConfig> plc)
{
    auto sock = bo::make_shared<net::Socket>(ip, port);
    auto trans = bo::make_shared<tt::TFramedTransport>(sock);
    auto proto = bo::make_shared<tp::TBinaryProtocol>(trans);
    boost::shared_ptr<ClientT> client;
    if (plc) {
        // The module provider might not supply the base path we want,
        // so make our own config access. This is libConfig data, not
        // to be confused with FDS configuration (platform.conf).
        FdsConfigAccessor configAccess(plc, "fds.feature_toggle.");
        /**
         * FEATURE TOGGLE: enable subscriptions (async replication)
         * Mon Dec 28 16:51:58 MST 2015
         */
        bool enableSubscriptions = configAccess.get<bool>("common.enable_subscriptions", false);
        if (enableSubscriptions) {
            boost::shared_ptr<::apache::thrift::protocol::TMultiplexedProtocol> multiproto =
                boost::make_shared<tp::TMultiplexedProtocol>(proto, strThriftServiceName);
            client = bo::make_shared<ClientT>(multiproto);
        }
    }
    if (!client) {
        client = bo::make_shared<ClientT>(proto);
    }
    bool bConnected = sock->connect(retryCnt);
    if (!bConnected) {
        GLOGWARN << "Failed to connect to ip: " << ip << " port: " << port;
        throw tt::TTransportException();
    }
    return client;
}
/*********************************************************************************
 * Explicit template instantiations
 ********************************************************************************/
template
boost::shared_ptr<fpi::PlatNetSvcClient> allocRpcClient<fpi::PlatNetSvcClient>(
    const std::string &ip, const int &port,
    const int &retryCnt, const std::string &strServiceName,
    const boost::shared_ptr<FdsConfig> plc);
// Adding a new service method is a backward compatible change.
// If an old client stub is used, the consumer code can not even know about
// the new service method. If a new client stub is used against an older
// server, 'invalid method name' is raised.
template
boost::shared_ptr<fpi::OMSvcClient> allocRpcClient<fpi::OMSvcClient>(
    const std::string &ip, const int &port,
    const int &retryCnt, const std::string &strServiceName,
    const boost::shared_ptr<FdsConfig> plc);
template
boost::shared_ptr<fpi::StreamingClient> allocRpcClient<fpi::StreamingClient>(
    const std::string &ip, const int &port,
    const int &retryCnt, const std::string &strServiceName,
    const boost::shared_ptr<FdsConfig> plc);
template
boost::shared_ptr<apis::ConfigurationServiceClient> allocRpcClient<apis::ConfigurationServiceClient>(
    const std::string &ip, const int &port,
    const int &retryCnt, const std::string &strServiceName,
    const boost::shared_ptr<FdsConfig> plc);

/*********************************************************************************
 * class methods
 ********************************************************************************/
std::size_t SvcUuidHash::operator()(const fpi::SvcUuid& svcId) const {
    return svcId.svc_uuid;
}

SvcMgr::SvcMgr(CommonModuleProviderIf *moduleProvider,
               PlatNetSvcHandlerPtr handler,
               fpi::PlatNetSvcProcessorPtr processor,
               const fpi::SvcInfo &svcInfo,
               const std::string &strServiceName)
    : HasModuleProvider(moduleProvider),
    Module("SvcMgr")
{
    auto config = MODULEPROVIDER()->get_conf_helper();
    auto platformPort = config.get_abs<int>("fds.pm.platform_port");

    // NOTE: OM ip and port are managed differently from other services which are
    // offsets from the Platform port.  The default OM Port happens to be the same
    // as is returned by mapToServicePort in the single node case, but it will not work
    // properly when running multiple nodes on the same host or when the OM is running
    // on a non-default port.
    omIp_ = config.get_abs<std::string>("fds.common.om_ip_list");
    omPort_ = config.get_abs<int>("fds.common.om_port",
                                  SvcMgr::mapToSvcPort(platformPort, fpi::FDSP_ORCH_MGR));
    omSvcUuid_.svc_uuid = static_cast<int64_t>(config.get_abs<fds_uint64_t>("fds.common.om_uuid"));

    fds_assert(omSvcUuid_.svc_uuid != 0);
    fds_assert(omPort_ != 0);

    svcRequestHandler_ = handler;
    svcInfo_ = svcInfo;

    /* Create the server */
    int port = svcInfo_.svc_port;

    // NOTE: Everyone communicates with the OM on port 7004 (platform + 4) and that
    // is what we advertise in the SvcInfo.  However, when the OM Service
    // Proxy is enabled 7004 is now the default port that the Java-OM listens on.
    // It forwards info to port 8904 which is now run by the C++ OM.
    // Port 8904 should be considered internal to the OM and should not
    // be accessed by any other services.
    if (config.get_abs<bool>("fds.feature_toggle.om.enable_java_om_svc_proxy", false) &&
         svcInfo_.svc_type ==  fpi::FDSP_ORCH_MGR) {

        int omSvcProxyPortOffset = config.get_abs<int>("fds.om.java_svc_proxy_port_offset", 1900);
        port += omSvcProxyPortOffset; //(defaults to 8904)

        LOGNOTIFY << "OM Java Service Proxy enabled.  Java OM process will run on port "
                <<  svcInfo_.svc_port
                << " and proxy all operations to OM Native (C++) service at port "
                << port
                << ".";
    }

    LOGNOTIFY << "Initializing Service Layer server for " << SvcMgr::mapToSvcName( svcInfo_.svc_type ) <<
            "[" << svcInfo_.ip << ":" << svcInfo_.svc_port << "]";

    svcServer_ = boost::make_shared<SvcServer>(port, processor, strServiceName, moduleProvider);

    taskExecutor_ = new SynchronizedTaskExecutor<uint64_t>(*MODULEPROVIDER()->proc_thrpool());

    svcRequestMgr_ = new SvcRequestPool(MODULEPROVIDER(), getSelfSvcUuid(), handler);
    gSvcRequestPool = svcRequestMgr_;

    dltMgr_.reset(new DLTManager());
    dmtMgr_.reset(new DMTManager());

}

fpi::FDSP_MgrIdType SvcMgr::mapToSvcType(const std::string &svcName)
{
    /* NOTE: Chagne to a map if this is invoked several times */
    if (svcName == "pm") {
        return fpi::FDSP_PLATFORM;
    } else if (svcName == "sm") {
        return fpi::FDSP_STOR_MGR;
    } else if (svcName == "dm") {
        return fpi::FDSP_DATA_MGR;
    } else if (svcName == "am") {
        return fpi::FDSP_ACCESS_MGR;
    } else if (svcName == "om") {
        return fpi::FDSP_ORCH_MGR;
    } else if (svcName == "console") {
        return fpi::FDSP_CONSOLE;
    } else if (svcName == "test") {
        return fpi::FDSP_TEST_APP;
    } else {
        fds_panic("Unknown svcName");
        return fpi::FDSP_INVALID_SVC;
    }
}

std::string SvcMgr::mapToSvcName(const fpi::FDSP_MgrIdType &svcType)
{
    switch (svcType) {
    case fpi::FDSP_PLATFORM:
        return "pm";
    case fpi::FDSP_STOR_MGR:
        return "sm";
    case fpi::FDSP_DATA_MGR:
        return "dm";
    case fpi::FDSP_ACCESS_MGR:
        return "am";
    case fpi::FDSP_ORCH_MGR:
        return "om";
    case fpi::FDSP_CONSOLE:
        return "console";
    case fpi::FDSP_TEST_APP:
        return "test";
    default:
        return "unknown";
    }
}

std::string SvcMgr::mapToSvcUuidAndName(const fpi::SvcUuid &svcUuid)
{
    std::stringstream ss;
    ss << std::hex << svcUuid.svc_uuid << std::dec
        << ":" << mapToSvcName(ResourceUUID(svcUuid).uuid_get_type());
    return ss.str();
}

int32_t SvcMgr::mapToSvcPort(const int32_t &platformPort, const fpi::FDSP_MgrIdType& svcType)
{
    int offset = static_cast<int>(svcType - fpi::FDSP_PLATFORM);
    return platformPort + offset;
}

fpi::SvcUuid SvcMgr::mapToSvcUuid(const fpi::SvcUuid &in,
                                  const fpi::FDSP_MgrIdType& svcType)
{
    ResourceUUID resIn(in);
    resIn.uuid_set_type(in.svc_uuid, svcType);
    return resIn.toSvcUuid();
}

fpi::SvcUuid SvcMgr::mapToSvcUuid(const NodeUuid &in,
                                  const fpi::FDSP_MgrIdType& svcType)
{
    ResourceUUID resIn(in);
    resIn.uuid_set_type(in.uuid_get_val(), svcType);
    return resIn.toSvcUuid();
}


SvcMgr::~SvcMgr()
{
    svcServer_->stop();
    delete taskExecutor_;
    delete svcRequestMgr_;
    svcRequestMgr_ = gSvcRequestPool = nullptr;
}

int SvcMgr::mod_init(SysParams const *const p)
{
    GLOGNOTIFY << "module initialize";

    svcRequestHandler_->setTaskExecutor(taskExecutor_);
    Error e = startServer();
    return e.getFdspErr();
}

void SvcMgr::mod_startup()
{
    if (MODULEPROVIDER()->get_cntrs_mgr() != nullptr) {
        stateProviderId = "svcmgr";
        MODULEPROVIDER()->get_cntrs_mgr()->add_for_export(this);
    }
    
    GLOGNOTIFY;
}

void SvcMgr::mod_enable_service()
{
    GLOGNOTIFY;
}

void SvcMgr::mod_shutdown()
{
    if (MODULEPROVIDER()->get_cntrs_mgr() != nullptr) {
        MODULEPROVIDER()->get_cntrs_mgr()->remove_from_export(this);
    }
    GLOGNOTIFY;
}

void SvcMgr::setSvcServerListener(SvcServerListener* listener)
{
    svcServer_->setListener(listener);
}

SvcRequestPool* SvcMgr::getSvcRequestMgr() const {
    return svcRequestMgr_;
}

SvcRequestCounters* SvcMgr::getSvcRequestCntrs() const {
    return svcRequestMgr_->getSvcRequestCntrs();
}

SvcRequestTracker* SvcMgr::getSvcRequestTracker() const {
    return svcRequestMgr_->getSvcRequestTracker();
}

Error SvcMgr::startServer()
{
    return svcServer_->start();
}

void SvcMgr::stopServer()
{
    svcServer_->stop();
}

void SvcMgr::updateSvcMap(const std::vector<fpi::SvcInfo> &entries)
{
    fds_scoped_lock lock(svcHandleMapLock_);
    for (auto &e : entries) {
        auto svcHandleItr = svcHandleMap_.find(e.svc_id.svc_uuid);
        if (svcHandleItr == svcHandleMap_.end()) {
            /* New service handle entry.  Note, we don't allocate rpcClient.  We do this lazily
             * when needed to not incur the cost of socket creation.
             */
            auto svcHandle = boost::make_shared<SvcHandle>(MODULEPROVIDER(), e);
            svcHandleMap_.emplace(std::make_pair(e.svc_id.svc_uuid, svcHandle));
            GLOGDEBUG << "svcmap update.  svcuuid: "
                << mapToSvcUuidAndName(e.svc_id.svc_uuid)
                << " is new.  Incarnation: " << e.incarnationNo;
        } else {
            svcHandleItr->second->updateSvcHandle(e);
        }
    }
}

void SvcMgr::getSvcMap(std::vector<fpi::SvcInfo> &entries)
{
    entries.clear();
    fds_scoped_lock lock(svcHandleMapLock_);
    for (auto &itr : svcHandleMap_) {
        fpi::SvcInfo info;
        itr.second->getSvcInfo(info);
        entries.push_back(info);
    }
}

bool SvcMgr::getSvcHandle_(const fpi::SvcUuid &svcUuid, SvcHandlePtr& handle) const
{
    auto itr = svcHandleMap_.find(svcUuid);
    if (itr == svcHandleMap_.end()) {
        return false;
    }
    handle = itr->second;
    return true;
}

void SvcMgr::sendAsyncSvcReqMessage(fpi::AsyncHdrPtr &header,
                                 StringPtr &payload)
{
    SvcHandlePtr svcHandle;
    fpi::SvcUuid &svcUuid = header->msg_dst_uuid;

    if (svcUuid == getSelfSvcUuid()) {
        /* Routing local requests */
        svcRequestHandler_->asyncReqt(header, payload);
        return;
    }

    do {
        fds_scoped_lock lock(svcHandleMapLock_);
        if (!getSvcHandle_(svcUuid, svcHandle)) {
            GLOGWARN << "Svc handle for svcuuid: " << svcUuid.svc_uuid << " not found";
            break;
        }
    } while (false);

    if (svcHandle) {
        svcHandle->sendAsyncSvcReqMessage(header, payload);
    } else {
        postSvcSendError(header);
    }
}

void SvcMgr::sendAsyncSvcRespMessage(fpi::AsyncHdrPtr &header,
                                     StringPtr &payload)
{
    SvcHandlePtr svcHandle;
    fpi::SvcUuid &svcUuid = header->msg_dst_uuid;

    if (svcUuid == getSelfSvcUuid()) {
        /* Routing local responses */
        svcRequestHandler_->asyncResp(header, payload);
        return;
    }

    do {
        fds_scoped_lock lock(svcHandleMapLock_);
        if (!getSvcHandle_(svcUuid, svcHandle)) {
            GLOGWARN << "Svc handle for svcuuid: " << svcUuid.svc_uuid << " not found";
            break;
        }
    } while (false);

    if (svcHandle) {
        svcHandle->sendAsyncSvcRespMessage(header, payload);
    } else {
        GLOGWARN << "Failed to get svc handle: " << fds::logString(*header)
                << ".  Dropping the respone";
    }
}

void SvcMgr::broadcastAsyncSvcReqMessage(fpi::AsyncHdrPtr &h,
                                      StringPtr &payload,
                                      const SvcInfoPredicate& predicate)
{
    SvcHandleMap map;

    {
        /* Make a copy of the service map */
        fds_scoped_lock lock(svcHandleMapLock_);
        map = svcHandleMap_;
    }

    for (auto &kv : map) {
        /* Create header for the target */
        auto header = boost::make_shared<fpi::AsyncHdr>(*h);
        header->msg_dst_uuid = kv.first;

        kv.second->sendAsyncSvcReqMessageOnPredicate(header, payload, predicate);
    }
}

void SvcMgr::postSvcSendError(fpi::AsyncHdrPtr &header)
{
    swapAsyncHdr(header);
    header->msg_code = ERR_SVC_REQUEST_INVOCATION;

    svcRequestMgr_->postError(header);
}

fpi::SvcUuid SvcMgr::getSelfSvcUuid() const
{
    return svcInfo_.svc_id.svc_uuid;
}

fpi::SvcInfo SvcMgr::getSelfSvcInfo() const {
    return svcInfo_;
}

std::string SvcMgr::getSelfSvcName() const {
    return svcInfo_.name;
}

fpi::SvcUuid SvcMgr::getMappedSelfPlatformUuid() const
{
    return mapToSvcUuid(getSelfSvcUuid(), fpi::FDSP_PLATFORM);
}

int SvcMgr::getMappedSelfPlatformPort() const
{
    int dist = svcInfo_.svc_type - fpi::FDSP_PLATFORM;
    return svcInfo_.svc_port - dist;
}

bool SvcMgr::getSvcInfo(const fpi::SvcUuid &svcUuid, fpi::SvcInfo& info) const {
    fds_scoped_lock lock(svcHandleMapLock_);
    auto svcHandleItr = svcHandleMap_.find(svcUuid);
    if (svcHandleItr == svcHandleMap_.end()) {
        return false;
    }
    svcHandleItr->second->getSvcInfo(info);
    return true;
}

int SvcMgr::getSvcPort() const
{
    return svcInfo_.svc_port;
}

void SvcMgr::getOmIPPort(std::string &omIp, fds_uint32_t &port) const
{
    omIp = omIp_;
    port = omPort_;
}

fpi::SvcUuid SvcMgr::getOmSvcUuid() const
{
    return omSvcUuid_;
}

fpi::OMSvcClientPtr SvcMgr::getNewOMSvcClient() const
{
    fpi::OMSvcClientPtr omClient;

    // never surrender.
    int omRetries = std::numeric_limits<int32_t>::max();
    while (true) {
        try {
            LOGTRACE << "Connecting to OM[" << omIp_ << ":" << omPort_ <<
                    "] with max retries of [" << omRetries << "]";
            omClient = allocRpcClient<fpi::OMSvcClient>(omIp_,
                omPort_,
                omRetries,
                fpi::commonConstants().OM_SERVICE_NAME,
                MODULEPROVIDER()->get_fds_config());
            break;
        } catch (std::exception &e) {
            GLOGWARN << "allocRpcClient failed.  Exception: " << e.what()
                << ".  ip: "  << omIp_ << " port: " << omPort_;
        } catch (...) {
            GLOGWARN << "allocRpcClient failed.  Unknown exception. ip: "
                << omIp_ << " port: " << omPort_;
        }
    }

    return omClient;
}

SynchronizedTaskExecutor<uint64_t>*
SvcMgr::getTaskExecutor() {
    return taskExecutor_;
}

bool SvcMgr::isSvcActionableError(const Error &e)
{
    // TODO(Rao): Implement
    return false;
}

void SvcMgr::handleSvcError(const fpi::SvcUuid &srcSvc, const Error &e)
{
    // TODO(Rao): Implement
}

DltTokenGroupPtr SvcMgr::getDLTNodesForDoidKey(const ObjectID &objId) {
    return dltMgr_->getDLT()->getNodes(objId);
}

DmtColumnPtr SvcMgr::getDMTNodesForVolume(fds_volid_t vol_id) {
    return dmtMgr_->getCommittedNodeGroup(vol_id);  // thread-safe, do not hold lock
}

DmtColumnPtr SvcMgr::getDMTNodesForVolume(fds_volid_t vol_id,
                                          fds_uint64_t dmt_version) {
    return dmtMgr_->getVersionNodeGroup(vol_id, dmt_version);  // thread-safe, do not hold lock
}

Error SvcMgr::getDLT(int maxAttempts) {
    ::FDS_ProtocolInterface::CtrlNotifyDLTUpdate fdsp_dlt;
	Error err(ERR_OK);
    int triedCnt = 0;
    while (true) {
        try {
            getDLTData(fdsp_dlt);
            break;
        } catch (Exception &e) {
            LOGWARN << "Failed to get dlt: " << e.what() << ".  Attempt# " << triedCnt;
        } catch (...) {
            LOGWARN << "Failed to get dlt.  Attempt# " << triedCnt;
        }
        /* Caught an exception...try again if needed */
        triedCnt++;
        if (triedCnt == maxAttempts) {
            return ERR_NOT_FOUND;
        }
    }

    /* Got the DLT */
	if (fdsp_dlt.dlt_version == DLT_VER_INVALID) {
		err = ERR_NOT_FOUND;
	} else {
		err = updateDlt(true, fdsp_dlt.dlt_data.dlt_data, NULL);
	}
	return err;
}

Error SvcMgr::getDMT(int maxAttempts) {
    ::FDS_ProtocolInterface::CtrlNotifyDMTUpdate fdsp_dmt;
	Error err(ERR_OK);
    int triedCnt = 0;
    while (true) {
        try {
            getDMTData(fdsp_dmt);
            break;
        } catch (Exception &e) {
            LOGWARN << "Failed to get dmt: " << e.what() << ".  Attempt# " << triedCnt;
        } catch (...) {
            LOGWARN << "Failed to get dmt.  Attempt# " << triedCnt;
        }
        /* Caught an exception...try again if needed */
        triedCnt++;
        if (triedCnt == maxAttempts) {
            return ERR_NOT_FOUND;
        }
    }

	if (fdsp_dmt.dmt_version == DMT_VER_INVALID) {
		err = ERR_NOT_FOUND;
	} else {
		err = updateDmt(DMT_COMMITTED, fdsp_dmt.dmt_data.dmt_data, nullptr);
	}
	return err;
}

Error SvcMgr::getAllVolumeDescriptors(fpi::GetAllVolumeDescriptors &list, int maxAttempts) {
	Error err(ERR_NOT_FOUND);
	int triedCnt = 0;
	while (maxAttempts > triedCnt++) {
		try {
			getAllVolumeDescriptorsData(list);
			if (list.volumeList.size() > 0) {
				err = ERR_OK;
			} else {
				// Could potentially not be an error, but we should have gotten SYSTEM_VOLUME
				// at least... so return ERR_NOT_FOUND
			}
			break;
		} catch (Exception &e) {
			LOGWARN << "Failed to get volume descriptor: " << e.what() << ". Attempt# " << triedCnt;
		} catch (...) {
			LOGWARN << "Failed to get volume descriptor. Attempt# " << triedCnt;
		}
	}

	return err;
}

const DLT* SvcMgr::getCurrentDLT() {
    return dltMgr_->getDLT();
}

DMTPtr SvcMgr::getCurrentDMT()
{
    return dmtMgr_->hasCommittedDMT() ? dmtMgr_->getDMT(DMT_COMMITTED) : nullptr;
}

bool SvcMgr::hasCommittedDMT() const {
    return dmtMgr_->hasCommittedDMT();
}

fds_uint64_t SvcMgr::getDMTVersion() {
    return dmtMgr_->getCommittedVersion();
}

Error SvcMgr::updateDlt(bool dlt_type, std::string& dlt_data, OmUpdateRespCbType const& cb) {
    Error err(ERR_OK);
    LOGNOTIFY << "Received new DLT version  " << dlt_type;

    // dltMgr is threadsafe
    err = dltMgr_->addSerializedDLT(dlt_data, cb, dlt_type);
    if (err.ok() || (err == ERR_IO_PENDING)) {
        dltMgr_->dump();
    } else if (ERR_DUPLICATE != err) {
        LOGERROR << "Failed to update DLT! check dlt_data was set " << err;
    }

    return err;
}

Error SvcMgr::updateDmt(bool dmt_type, std::string& dmt_data, OmUpdateRespCbType const& cb) {
    Error err(ERR_OK);
    LOGNOTIFY << "Received new DMT version  " << dmt_type;

    /* Check to ensure we only have one DMT when volumegrouping is enabled */
    auto volgroupingEnabled = MODULEPROVIDER()->get_fds_config()->get<bool>(
            "fds.feature_toggle.common.enable_volumegrouping", false);
    if (volgroupingEnabled && dmtMgr_->hasCommittedDMT()) {
        auto committed = dmtMgr_->getDMT(DMT_COMMITTED);
        if (committed) {
            auto serializer = serialize::getMemSerializer();
            committed->write(serializer);
            fds_verify(serializer->getBufferAsString() == dmt_data);
        }
    }

    err = dmtMgr_->addSerializedDMT(dmt_data, cb, DMT_COMMITTED);
    if (!err.ok()) {
        LOGERROR << "Failed to update DMT! check dmt_data was set";
    }

    return err;
}


void SvcMgr::getDMTData(::FDS_ProtocolInterface::CtrlNotifyDMTUpdate &fdsp_dmt)
{
	int64_t nullarg = 0;
	fpi::OMSvcClientPtr omSvcRpc = MODULEPROVIDER()->getSvcMgr()->getNewOMSvcClient();
	fds_verify(omSvcRpc);

	omSvcRpc->getDMT(fdsp_dmt, nullarg);
}

void SvcMgr::getDLTData(::FDS_ProtocolInterface::CtrlNotifyDLTUpdate &fdsp_dlt)
{
	int64_t nullarg = 0;
	fpi::OMSvcClientPtr omSvcRpc = MODULEPROVIDER()->getSvcMgr()->getNewOMSvcClient();
	fds_verify(omSvcRpc);

	omSvcRpc->getDLT(fdsp_dlt, nullarg);
}

void SvcMgr::getAllVolumeDescriptorsData(fpi::GetAllVolumeDescriptors &list)
{
	int nullarg = 0;
	fpi::OMSvcClientPtr omSvcRpc = MODULEPROVIDER()->getSvcMgr()->getNewOMSvcClient();
	fds_verify(omSvcRpc);

	omSvcRpc->getAllVolumeDescriptors(list, nullarg);
}

void SvcMgr::notifyOMSvcIsDown(const fpi::SvcInfo &info)
{
    auto svcDownMsg = boost::make_shared<fpi::NotifyHealthReport>();
    svcDownMsg->healthReport.serviceInfo = info;
    svcDownMsg->healthReport.serviceState = fpi::HEALTH_STATE_UNREACHABLE;

    auto asyncReq = getSvcRequestMgr()->newEPSvcRequest(getOmSvcUuid());
    asyncReq->setPayload(FDSP_MSG_TYPEID(fpi::NotifyHealthReport), svcDownMsg);
    asyncReq->invoke();
}

void SvcMgr::setUnreachableInjection(float frequency) {
    // Enable unreachable faults that occur infrequently. Since SvcLayer doesn't
    // know the request type, basic startup communication would fail if this
    // was triggered every time.
    // This enables the probability to be low enough to have the system run
    // for a bit before eventually hitting it.
    fiu_enable_random("svc.fault.unreachable", 1, NULL, 0, frequency);
    LOGNOTIFY << "Enabling unreachable fault injections at a probability of " << frequency;
}

std::string SvcMgr::getStateProviderId() {
    return stateProviderId;
}

std::string SvcMgr::getStateInfo() {

    Json::Value state;
    state["outstandingRequestsCount"] = static_cast<Json::Value::UInt64>(svcRequestMgr_->getOutstandingRequestsCount());

    std::stringstream ss;
    ss << state;
    return ss.str();
}

SvcHandle::SvcHandle(CommonModuleProviderIf *moduleProvider,
                     const fpi::SvcInfo &info)
: HasModuleProvider(moduleProvider)
{
    svcInfo_ = info;
    GLOGDEBUG << "Operation: new service handle";
    GLOGDEBUG << logString();
}

SvcHandle::~SvcHandle()
{
    GLOGDEBUG << logString();
}

void SvcHandle::sendAsyncSvcReqMessage(fpi::AsyncHdrPtr &header,
                                    StringPtr &payload)
{
    bool bSendSuccess = true;
    {
        fds_scoped_lock lock(lock_);
        bSendSuccess  = sendAsyncSvcMessageCommon_(true, header, payload);
    }
    if (!bSendSuccess) {
        MODULEPROVIDER()->getSvcMgr()->postSvcSendError(header);
    }
}

void SvcHandle::sendAsyncSvcRespMessage(fpi::AsyncHdrPtr &header,
                                        StringPtr &payload)
{
    bool bSendSuccess = true;
    {
        fds_scoped_lock lock(lock_);
        bSendSuccess  = sendAsyncSvcMessageCommon_(false, header, payload);
    }
    if (!bSendSuccess) {
        MODULEPROVIDER()->getSvcMgr()->postSvcSendError(header);
    }
}

void SvcHandle::sendAsyncSvcReqMessageOnPredicate(fpi::AsyncHdrPtr &header,
                                               StringPtr &payload,
                                               const SvcInfoPredicate& predicate)
{
    bool bSendSuccess = true;
    {
        fds_scoped_lock lock(lock_);
        /* Only send to services matching predicate */
        if (predicate(svcInfo_)) {
            bSendSuccess  = sendAsyncSvcMessageCommon_(true, header, payload);
        }
    }

    if (!bSendSuccess) {
        MODULEPROVIDER()->getSvcMgr()->postSvcSendError(header);
    }
}

bool SvcHandle::sendAsyncSvcMessageCommon_(bool isAsyncReqt,
                                           fpi::AsyncHdrPtr &header,
                                           StringPtr &payload)
{
    /* NOTE: This code assumes lock is held */

    if (isSvcDown_()) {
        /* No point trying to send when service is down */
        GLOGDEBUG << "No point in sending when service is down! ( "
                  << fds::logString(svcInfo_) << ")";
        return false;
    }
    try {
        if (!svcClient_) {
            GLOGDEBUG << "Allocating PlatNetSvcClient for: " << logString();
            svcClient_ = allocRpcClient<fpi::PlatNetSvcClient>(svcInfo_.ip,
                                                               svcInfo_.svc_port,
                                                               SvcMgr::MIN_CONN_RETRIES,
                                                               fpi::commonConstants().PLATNET_SERVICE_NAME,
                                                               MODULEPROVIDER()->get_fds_config());
        }
        if (isAsyncReqt) {
            /**
             * fault injection, if 'svc.fault.unreachable' is set the following lambda will execute
             */
            fiu_do_on("svc.fault.unreachable",
                      LOGNOTIFY << "Triggering unreachable fault"; throw "Fault injection unreachable";);

            svcClient_->asyncReqt(*header, *payload);
        } else {
            svcClient_->asyncResp(*header, *payload);
        }
        return true;
    } catch (std::exception &e) {
        GLOGWARN << "allocRpcClient failed.  Exception: " << e.what() << ".  "  << header
                 << " SvcInfo ( " << fds::logString(svcInfo_) << " )";
        markSvcDown_();
    } catch (...) {
        GLOGWARN << "allocRpcClient failed.  Unknown exception. " << header
                 << " SvcInfo ( " << fds::logString(svcInfo_) << " )";
        markSvcDown_();
    }
    return false;
}

bool
SvcHandle::shouldUpdateSvcHandle(const fpi::SvcInfoPtr &current, const fpi::SvcInfoPtr &incoming)
{
    fds_bool_t ret(false);

    if ( current->incarnationNo < incoming->incarnationNo ) {
        ret = true;
    } else if ( (current->incarnationNo == incoming->incarnationNo) &&
                (current->svc_status != incoming->svc_status) ) {
        /**
         * Once a process is declared INACTIVE_FAILED, it is possible that a service
         * can transition to multiple other states.
         * A svc gets set to failed is because svc layer for some reason cannot reach the service.
         * However, periodic heartbeat checks can revive the svcLayer connection in which case an
         * update can come in for the incarnation number for a change from FAILED -> ACTIVE
         *
         * We can also have scenarios where non-PM services in INACTIVE_FAILED state are being
         * re-started through activate_known_services call in which case a FAILED->STARTED update can
         * come in for the same incarnation number
         *
         * The other allowable use case is if a dead service were to restart, in which
         * it would then come with a newer incarnation number.
         *
         * TODO(Neil): make this check more detailed and centralize the conditionals...
         * Will happen in another PR.
         */
        ret = true;
    } else if (incoming->incarnationNo == 0) {
        /**
         * TODO
         * This is a workaround to handle when no incarnation number is given...
         * we have to assume that this is newer than the old incarnation number in the
         * configDB. Until all areas of PM and OM are sending incarnation number,
         * this has to be here... and bugs may be coming in.
         */
        LOGWARN << "Allowing update with zero incarnatioNo!";
        LOGDEBUG << "THIS NEEDS TO BE FIXED. Should be passing in with complete info.";
        ret = true;
    } else {
        LOGWARN << "Criteria not met, will not allow update of svcMap";
    }

    return (ret);
}

void SvcHandle::updateSvcHandle(const fpi::SvcInfo &newInfo)
{
    fds_scoped_lock lock(lock_);
    auto currentPtr = boost::make_shared<fpi::SvcInfo>(svcInfo_);
    auto newPtr = boost::make_shared<fpi::SvcInfo>(newInfo);
    GLOGDEBUG << "Incoming update: " << fds::logString(*newPtr) << " vs current status: "
            << fds::logString(*currentPtr);
    if (shouldUpdateSvcHandle(currentPtr, newPtr)) {
        svcInfo_ = newInfo;
        svcClient_.reset();
        GLOGDEBUG << "Operation Applied.";
    } else {
        GLOGDEBUG << "Operation not Applied.";
    }
}

void SvcHandle::getSvcInfo(fpi::SvcInfo &info) const
{
    fds_scoped_lock lock(lock_);
    info = svcInfo_;
}

std::string SvcHandle::logString() const
{
    /* NOTE: not protected by lock */
    return fds::logString(svcInfo_);
}

bool SvcHandle::isSvcDown_() const
{
    /* NOTE: Assumes this function is invoked under lock */
    return svcInfo_.svc_status == fpi::SVC_STATUS_INACTIVE_FAILED;
}

void SvcHandle::markSvcDown_()
{
    /* NOTE: Assumes this function is invoked under lock */
    svcInfo_.svc_status = fpi::SVC_STATUS_INACTIVE_FAILED;
    svcClient_.reset();
    GLOGDEBUG << logString();

    auto svcMgr = MODULEPROVIDER()->getSvcMgr();
    // Don't report OM to itself.
    if (svcMgr->getOmSvcUuid() != svcInfo_.svc_id.svc_uuid) {
        svcMgr->notifyOMSvcIsDown(svcInfo_);
    }
}

}  // namespace fds
