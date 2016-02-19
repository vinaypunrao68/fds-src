/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <string>
#include <map>
#include <vector>
#include <csignal>
#include <unordered_map>

#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>

#include <net/PlatNetSvcHandler.h>
#include <util/Log.h>
#include <net/SvcRequest.h>
#include <net/SvcRequestTracker.h>
#include <net/SvcRequestPool.h>
#include <net/SvcMgr.h>
#include <concurrency/SynchronizedTaskExecutor.hpp>
#include <util/fiu_util.h>
#include <fiu-control.h>
#include <fds_process.h>
#include <fdsp/svc_api_types.h>

namespace fds {

thread_local StringPtr PlatNetSvcHandler::threadLocalPayloadBuf;

PlatNetSvcHandler::PlatNetSvcHandler(CommonModuleProviderIf *provider)
: HasModuleProvider(provider),
Module("PlatNetSvcHandler")
{
    handlerState_ = ACCEPT_REQUESTS;
    REGISTER_FDSP_MSG_HANDLER(fpi::GetSvcStatusMsg, getStatus);
    REGISTER_FDSP_MSG_HANDLER(fpi::UpdateSvcMapMsg, updateSvcMap);
    REGISTER_FDSP_MSG_HANDLER(fpi::GetSvcMapMsg, getSvcMap);
}

PlatNetSvcHandler::~PlatNetSvcHandler()
{
}
int PlatNetSvcHandler::mod_init(SysParams const *const param)
{
    return 0;
}

void PlatNetSvcHandler::mod_startup()
{
}

void PlatNetSvcHandler::mod_shutdown()
{
}

void PlatNetSvcHandler::setHandlerState(PlatNetSvcHandler::State newState)
{
    /* NOTE: For now the supported  state sequest is
     * DEFER_REQUESTS(During init)->ACCEPT_REQUESTS(After registration
     * completes)->DROP_REQUESTS(preparing to shutdown)
     */

    auto oldState = std::atomic_exchange(&handlerState_, newState);

    fds_assert((oldState == ACCEPT_REQUESTS && newState == DEFER_REQUESTS) ||
               (oldState == DEFER_REQUESTS && newState == ACCEPT_REQUESTS) ||
               (oldState == ACCEPT_REQUESTS && newState == DROP_REQUESTS) ||
               (oldState == DEFER_REQUESTS && newState == DROP_REQUESTS));

    if (oldState == DEFER_REQUESTS && newState == ACCEPT_REQUESTS) {
        /* Drain all the deferred requests.  NOTE it's possible to drain them
         * on a threadpool as well
         */
        for (auto &reqPair: deferredReqs_) {
            LOGNORMAL << "Replaying deferred request: " << fds::logString(*(reqPair.first));
            asyncReqt(reqPair.first, reqPair.second);        
        }
    }
}


/**
 * @brief 
 *
 * @param taskExecutor
 */
void PlatNetSvcHandler::setTaskExecutor(SynchronizedTaskExecutor<uint64_t>  *taskExecutor)
{
    taskExecutor_ = taskExecutor;
}

void PlatNetSvcHandler::updateHandler(const fpi::FDSPMsgTypeId msgId,
                                      const FdspMsgHandler &handler)
{
    asyncReqHandlers_[msgId] = handler;
}

/**
 * @brief
 *
 * @param _return
 * @param msg
 */

void PlatNetSvcHandler::asyncReqt(const FDS_ProtocolInterface::AsyncHdr& header,
                                       const std::string& payload)
{
}

/**
  * @brief Invokes registered request handler
  *
  * @param header
  * @param payload
  */
void PlatNetSvcHandler::asyncReqt(boost::shared_ptr<FDS_ProtocolInterface::AsyncHdr>& header,
                                     boost::shared_ptr<std::string>& payload)
{
    fiu_do_on("svc.uturn.asyncreqt", header->msg_code = ERR_INVALID;
              sendAsyncResp(*header, fpi::EmptyMsgTypeId, fpi::EmptyMsg()); return; );

    /* Based on current handler state take appropriate action */
    PlatNetSvcHandler::State state = handlerState_;
    if (state == DEFER_REQUESTS) {
        LOGWARN << "Deferring request: " << fds::logString(*header);
        deferredReqs_.push_back(std::make_pair(header, payload));
        return;
    } else if (state == DROP_REQUESTS) {
        LOGWARN << "Dropping request: " << fds::logString(*header);
        return;
    }

    fds_assert(state == ACCEPT_REQUESTS);
    // LOGDEBUG << logString(*header);
    try
    {
        /* Deserialize the message and invoke the handler.  Deserialization is performed
         * by deserializeFdspMsg().
         * For detaails see macro REGISTER_FDSP_MSG_HANDLER()
         */
         fds_assert(header->msg_type_id != fpi::UnknownMsgTypeId);
         asyncReqHandlers_.at(header->msg_type_id) (header, payload);
    } catch(std::out_of_range &e)
    {
        /*
         * TODO Please justify why in debug mode this is an abort?
         * 
         * We don't even provide the message type id so we can troubleshoot it
         * when we are in debug mode.
         * 
         * This make no sense, we log it in production and move on.
         */
//        fds_assert(!"Unregistered fdsp message type");
        LOGWARN << "Unknown message type: " << static_cast<int32_t>(header->msg_type_id)
                << " Ignoring";
    }
}

/**
  *
  * @param header
  * @param payload
  */
void PlatNetSvcHandler::asyncResp(const FDS_ProtocolInterface::AsyncHdr& header,
                                    const std::string& payload)
{
}

/**
  * Handler function for async responses
  * @param header
  * @param payload
  */
void PlatNetSvcHandler::asyncResp(boost::shared_ptr<FDS_ProtocolInterface::AsyncHdr>& header,
                                    boost::shared_ptr<std::string>& payload)
{
    fiu_do_on("svc.disable.schedule", asyncRespHandler(\
    MODULEPROVIDER()->getSvcMgr()->getSvcRequestTracker(), header, payload); return; );
    // fiu_do_on("svc.use.lftp", asyncResp2(header, payload); return; );


//    GLOGDEBUG << logString(*header);

    fds_assert(header->msg_type_id != fpi::UnknownMsgTypeId);

    /* Execute on synchronized task executor so that handling for requests
     * with same task id or executor id gets serialized
     */
    auto reqTracker = MODULEPROVIDER()->getSvcMgr()->getSvcRequestTracker();
    auto asyncReq = reqTracker->getSvcRequest(static_cast<SvcRequestId>(header->msg_src_id));
    if (asyncReq && asyncReq->taskExecutorIdIsSet()) {
        taskExecutor_->scheduleOnHashKey(asyncReq->getTaskExecutorId(),
                                         std::bind(&PlatNetSvcHandler::asyncRespHandler,
                                                   MODULEPROVIDER()->getSvcMgr()->getSvcRequestTracker(),
                                                   header,
                                                   payload));
    } else {
        taskExecutor_->scheduleOnHashKey(header->msg_src_id,
                                         std::bind(&PlatNetSvcHandler::asyncRespHandler,
                                                   MODULEPROVIDER()->getSvcMgr()->getSvcRequestTracker(),
                                                   header, payload));
    }
}


/**
  * @brief Static async response handler. Making this static so that this function
  * is accessible locally(with in the process) to everyone.
  *
  * @param header
  * @param payload
*/
void PlatNetSvcHandler::asyncRespHandler(SvcRequestTracker* reqTracker,
     boost::shared_ptr<FDS_ProtocolInterface::AsyncHdr>& header,
     boost::shared_ptr<std::string>& payload)
{
     auto asyncReq = reqTracker->getSvcRequest(static_cast<SvcRequestId>(header->msg_src_id));
     if (!asyncReq)
     {
         // TODO(Andrew/Rao): We should add this error message back when
         // we fix FS-2131 and we've received responses from all replicas.
         GLOGTRACE << logString(*header) << " Request doesn't exist (timed out/fire and forget)?";
         return;
     }

     asyncReq->handleResponse(header, payload);
}

 void PlatNetSvcHandler::sendAsyncResp_(const fpi::AsyncHdr& reqHdr,
                     const fpi::FDSPMsgTypeId &msgTypeId,
                     StringPtr &payload)
{
     auto respHdr = boost::make_shared<fpi::AsyncHdr>(
          std::move(SvcRequestPool::swapSvcReqHeader(reqHdr)));
     respHdr->msg_type_id = msgTypeId;

     MODULEPROVIDER()->getSvcMgr()->sendAsyncSvcRespMessage(respHdr, payload);
}

void PlatNetSvcHandler::getSvcMap(std::vector<fpi::SvcInfo> & _return,
                                  const int64_t nullarg)
{
    // thrift generated..don't add anything here
    fds_assert(!"shouldn't be here");
}

void
PlatNetSvcHandler::getSvcMap(boost::shared_ptr<fpi::AsyncHdr>&hdr,
                        boost::shared_ptr<fpi::GetSvcMapMsg> &msg)
{
    Error err(ERR_OK);
    LOGDEBUG << " received " << hdr->msg_code
             << " from:" << std::hex << hdr->msg_src_uuid.svc_uuid << std::dec;

    fpi::GetSvcMapRespMsgPtr respMsg (new fpi::GetSvcMapRespMsg());
    boost::shared_ptr<int64_t> nullarg;
    getSvcMap(respMsg->svcMap, nullarg);

    // initialize the response message with the service map
    hdr->msg_code = 0;
    sendAsyncResp (*hdr, FDSP_MSG_TYPEID(fpi::GetSvcMapRespMsg), *respMsg);
}

void PlatNetSvcHandler::getSvcMap(std::vector<fpi::SvcInfo> & _return,
                       boost::shared_ptr<int64_t>& nullarg)
{
    LOGDEBUG << "Service map request";
    MODULEPROVIDER()->getSvcMgr()->getSvcMap(_return);
}

void PlatNetSvcHandler::getDLT(fpi::CtrlNotifyDLTUpdate& dlt, SHPTR<int64_t>& nullarg)
{
    auto dtp = MODULEPROVIDER()->getSvcMgr()->getCurrentDLT();
    if (!dtp) {
		LOGDEBUG << "Not sending DLT, because no " << " committed DLT yet";
        dlt.__set_dlt_version(DLT_VER_INVALID);

	} else {
        std::string data_buffer;
        fpi::FDSP_DLT_Data_Type dlt_val;

		dtp->getSerialized(data_buffer);
		dlt.__set_dlt_version(dtp->getVersion());
		dlt_val.__set_dlt_data(data_buffer);
		dlt.__set_dlt_data(dlt_val);
	}
}

void PlatNetSvcHandler::getDMT(fpi::CtrlNotifyDMTUpdate& dmt, SHPTR<int64_t>& nullarg)
{
    DMTPtr dp = MODULEPROVIDER()->getSvcMgr()->getCurrentDMT();
    if (!dp) {
        LOGDEBUG << "Not sending DMT, because no committed DMT yet";
        dmt.__set_dmt_version(DMT_VER_INVALID);
    } else {
    	fpi::FDSP_DMT_Data_Type fdt;
        std::string data_buffer;

    	dp->getSerialized(data_buffer);
    	fdt.__set_dmt_data(data_buffer);
    	dmt.__set_dmt_version(dp->getVersion());
    	dmt.__set_dmt_data(fdt);
    }
}

void PlatNetSvcHandler::allUuidBinding(const fpi::UuidBindMsg& mine)
{
}

void
PlatNetSvcHandler::allUuidBinding(boost::shared_ptr<fpi::UuidBindMsg>& mine)  // NOLINT
{
}

void
PlatNetSvcHandler::notifyNodeActive(const fpi::FDSP_ActivateNodeType &info)
{
}

void
PlatNetSvcHandler::notifyNodeActive(boost::shared_ptr<fpi::FDSP_ActivateNodeType> &info)
{
    LOGDEBUG << "Boost notifyNodeActive";
}

void PlatNetSvcHandler::notifyNodeInfo(
    std::vector<fpi::NodeInfoMsg> & _return,      // NOLINT
    const fpi::NodeInfoMsg& info, const bool bcast)
{
}

void PlatNetSvcHandler::notifyNodeInfo(
    std::vector<fpi::NodeInfoMsg> & _return,      // NOLINT
    boost::shared_ptr<fpi::NodeInfoMsg>& info,                       // NOLINT
    boost::shared_ptr<bool>& bcast)
{
}

void PlatNetSvcHandler::getProperty(std::string& _return, const std::string& name) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}


void PlatNetSvcHandler::getProperty(std::string& _return, boost::shared_ptr<std::string>& name) {
    if (!MODULEPROVIDER() || !(MODULEPROVIDER()->getProperties())) return;
    _return = MODULEPROVIDER()->getProperties()->get(*name);
}

void PlatNetSvcHandler::getProperties(std::map<std::string, std::string> & _return, const int32_t nullarg) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}


void PlatNetSvcHandler::getProperties(std::map<std::string, std::string> & _return, boost::shared_ptr<int32_t>& nullarg) {
    if (!MODULEPROVIDER() || !(MODULEPROVIDER()->getProperties())) return;
    _return = MODULEPROVIDER()->getProperties()->getAllProperties();
}

void PlatNetSvcHandler::getConfig(std::map<std::string, std::string> & _return, const int32_t nullarg) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}


void PlatNetSvcHandler::getConfig(std::map<std::string, std::string> & _return, boost::shared_ptr<int32_t>& nullarg) {
    if (!MODULEPROVIDER()) return;
    MODULEPROVIDER()->get_fds_config()->getConfigMap(_return);
}


/**
 * Return list of domain nodes in the node inventory.
 */
void
PlatNetSvcHandler::getDomainNodes(fpi::DomainNodes &ret, const fpi::DomainNodes &domain)
{
}

void
PlatNetSvcHandler::getDomainNodes(fpi::DomainNodes                    &ret,
                                  boost::shared_ptr<fpi::DomainNodes> &domain)
{
#if 0
    /*  Only do the local domain for now */
    DomainContainer::pointer    local;

    local = Platform::platf_singleton()->plf_node_inventory();
    local->dc_node_svc_info(ret);
#endif
}

fpi::ServiceStatus PlatNetSvcHandler::getStatus(const int32_t nullarg)
{
    return fpi::SVC_STATUS_INVALID;
}

void PlatNetSvcHandler::getCounters(std::map<std::string, int64_t> & _return,    // NOLINT
                                    const std::string& id)
{
}

void PlatNetSvcHandler::resetCounters(const std::string& id) // NOLINT
{
}

void PlatNetSvcHandler::getStateInfo(std::string & _return,  // NOLINT
                                     const std::string& id)
{
}


void PlatNetSvcHandler::getFlags(std::map<std::string, int64_t> & _return,
                                 const int32_t nullarg)  // NOLINT
{
}

void PlatNetSvcHandler::setConfigVal(const std::string& name, const std::string& value)
{
}

void PlatNetSvcHandler::setFlag(const std::string& id, const int64_t value)
{
}

int64_t PlatNetSvcHandler::getFlag(const std::string& id)
{
    return 0;
}
bool PlatNetSvcHandler::setFault(const std::string& command)
{
    // Don't do anything here. This stub is just to keep cpp compiler happy
    return false;
}

/**
 * Returns the status of the service
 * @param nullarg
 * @return
 */
fpi::ServiceStatus PlatNetSvcHandler::
getStatus(boost::shared_ptr<int32_t>& nullarg)  // NOLINT
{
    if (!MODULEPROVIDER())
    {
        return fpi::SVC_STATUS_INVALID;
    }
    return fpi::SVC_STATUS_ACTIVE;
}

/**
 * Returns the stats associated with counters identified by id
 * @param _return
 * @param id
 */
void PlatNetSvcHandler::getCounters(std::map<std::string, int64_t> & _return,
                                    boost::shared_ptr<std::string>& id)
{
    if (!MODULEPROVIDER())
    {
        return;
    }

    if (*id == "*")
    {
        /* Request to return all counters */
        MODULEPROVIDER()->get_cntrs_mgr()->toMap(_return);
        return;
    }

    auto    cntrs = MODULEPROVIDER()->get_cntrs_mgr()->get_counters(*id);

    if (cntrs == nullptr)
    {
        return;
    }
    cntrs->toMap(_return);
}

/**
 * Reset counters identified by id
 * @param id
 */
void PlatNetSvcHandler::resetCounters(boost::shared_ptr<std::string>& id)
{
    if (!MODULEPROVIDER())
    {
        return;
    }

    if (*id == "*")
    {
        /* Request to return all counters */
        MODULEPROVIDER()->get_cntrs_mgr()->reset();
    }
}

/**
* @brief Returns state as json from StateProvider identified by id
*
* @param _return
* @param id
*/
void PlatNetSvcHandler::getStateInfo(std::string & _return,
                                     boost::shared_ptr<std::string>& id) 
{
    if (!MODULEPROVIDER()) {
        return;
    }
    MODULEPROVIDER()->get_cntrs_mgr()->getStateInfo(*id, _return);
}

/**
 * For setting a flag dynamically
 * @param id
 * @param val
 */
void PlatNetSvcHandler:: setConfigVal(SHPTR<std::string>& name, SHPTR<std::string>& value) {
    if (!MODULEPROVIDER()) {
        return;
    }

    try {
        if (name->find("log_severity") != std::string::npos) {
            LOGGERPTR->setSeverityFilter(fds_log::getLevelFromName(*value));
        }
        MODULEPROVIDER()->get_fds_config()->set(*name, *value);
    } catch(...) {
        // TODO(Rao): Only ignore SettingNotFound exception
        /* Ignore the error */
    }
}

/**
 * @brief
 *
 * @param id
 * @param value
 */
void PlatNetSvcHandler::setFlag(boost::shared_ptr<std::string>& id,  // NOLINT
                                boost::shared_ptr<int64_t>& value)
{
}


/**
 * @brief
 *
 * @param id
 *
 * @return
 */
int64_t PlatNetSvcHandler::getFlag(boost::shared_ptr<std::string>& id)  // NOLINT
{
    return 0;
}

/**
 * @brief map of all the flags
 *
 * @param _return
 * @param nullarg
 */
void PlatNetSvcHandler::getFlags(std::map<std::string, int64_t> & _return,  // NOLINT
                                 boost::shared_ptr<int32_t>& nullarg)
{
}

/**
 * @brief
 *
 * @param command
 */
bool PlatNetSvcHandler::setFault(boost::shared_ptr<std::string>& cmdline)  // NOLINT
{
    // exception case for rotate logs
    if (*cmdline == "log.rotate") {
        LOGGERPTR->rotate();
        return true;
    }

    boost::char_separator<char>                       sep(", ");
    /* Parse the cmd line */
    boost::tokenizer<boost::char_separator<char> >    toknzr(*cmdline, sep);
    std::unordered_map<std::string, std::string>      args;

    for (auto t : toknzr)
    {
        if (args.size() == 0)
        {
            args["cmd"] = t;
            continue;
        }

        std::vector<std::string>    parts;
        boost::split(parts, t, boost::is_any_of("= "));

        if (parts.size() != 2)
        {
            return false;
        }

        args[parts[0]] = parts[1];
    }

    if (args.count("cmd") == 0 || args.count("name") == 0)
    {
        return false;
    }

    /* Execute based on the cmd */
    auto    cmd = args["cmd"];
    auto    name = args["name"];

    if (cmd == "enable")
    {
        fiu_enable(name.c_str(), 1, NULL, 0);
        LOGDEBUG << "Enable fault: " << name;
    }else if (cmd == "enable_random") {
        // TODO(Rao): add support for this
    }else if (cmd == "disable") {
        fiu_disable(name.c_str());
        LOGDEBUG << "Disable fault: " << name;
    }
    return true;
}

void PlatNetSvcHandler::getStatus(fpi::AsyncHdrPtr &header,
                                  fpi::GetSvcStatusMsgPtr &statusMsg)
{
    static boost::shared_ptr<int32_t> nullarg;
    fpi::GetSvcStatusRespMsgPtr respMsg = boost::make_shared<fpi::GetSvcStatusRespMsg>();
    respMsg->status = getStatus(nullarg);
    sendAsyncResp(*header, FDSP_MSG_TYPEID(fpi::GetSvcStatusRespMsg), *respMsg);
}

void PlatNetSvcHandler::updateSvcMap(fpi::AsyncHdrPtr &header,
                                     fpi::UpdateSvcMapMsgPtr &svcMapMsg)
{
    MODULEPROVIDER()->getSvcMgr()->updateSvcMap(svcMapMsg->updates);
}

}  // namespace fds
