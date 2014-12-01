/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <map>
#include <vector>
#include <csignal>
#include<unordered_map>
#include <fiu-control.h>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <fds_process.h>
#include <net/PlatNetSvcHandler.h>
#include <platform/platform-lib.h>
#include <platform/node-workflow.h>

namespace fds {
PlatNetSvcHandler::PlatNetSvcHandler()
{
    REGISTER_FDSP_MSG_HANDLER(fpi::NodeInfoMsg, notify_node_info);
    REGISTER_FDSP_MSG_HANDLER(fpi::NodeQualify, notify_node_qualify);
    REGISTER_FDSP_MSG_HANDLER(fpi::NodeUpgrade, notify_node_upgrade);
    REGISTER_FDSP_MSG_HANDLER(fpi::NodeIntegrate, notify_node_integrate);
    REGISTER_FDSP_MSG_HANDLER(fpi::NodeDeploy, notify_node_deploy);
    REGISTER_FDSP_MSG_HANDLER(fpi::NodeFunctional, notify_node_functional);
    REGISTER_FDSP_MSG_HANDLER(fpi::NodeDown, notify_node_down);
    REGISTER_FDSP_MSG_HANDLER(fpi::NodeEvent, notify_node_event);
}

PlatNetSvcHandler::~PlatNetSvcHandler()
{
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
    /*  Only do the local domain for now */
    DomainContainer::pointer local;

    local = Platform::platf_singleton()->plf_node_inventory();
    local->dc_node_svc_info(ret);
}

void
PlatNetSvcHandler::getSvcEvent(fpi::NodeEvent &ret, const fpi::NodeEvent &in)
{
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

void PlatNetSvcHandler::getFlags(std::map<std::string, int64_t> & _return,
                                 const int32_t nullarg)  // NOLINT
{
}

void PlatNetSvcHandler::setConfigVal(const std::string& id, const int64_t val)
{
}

void PlatNetSvcHandler::setFlag(const std::string& id, const int64_t value)
{
}

int64_t PlatNetSvcHandler::getFlag(const std::string& id)
{
    return 0;
}
bool PlatNetSvcHandler::setFault(const std::string& command) {
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
    if (!g_fdsprocess)
        return fpi::SVC_STATUS_INVALID;
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
    if (!g_fdsprocess) return;

    if (*id == "*") {
        /* Request to return all counters */
        g_fdsprocess->get_cntrs_mgr()->toMap(_return);
        return;
    }

    auto cntrs = g_fdsprocess->get_cntrs_mgr()->get_counters(*id);
    if (cntrs == nullptr) {
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
    if (!g_fdsprocess) return;

    if (*id == "*") {
        /* Request to return all counters */
        g_fdsprocess->get_cntrs_mgr()->reset();
    }
}
/**
 * For setting a flag dynamically
 * @param id
 * @param val
 */
void PlatNetSvcHandler::
setConfigVal(boost::shared_ptr<std::string>& id,  // NOLINT
            boost::shared_ptr<int64_t>& val)
{
    if (!g_fdsprocess) return;

    try {
        g_fdsprocess->get_fds_config()->set(*id, *val);
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
    if (*id == "common_enter_gdb") {
        // TODO(Rao): This isn't working.  Needs to be debugged
        raise(SIGINT);
        return;
    }
    bool found = PlatformProcess::plf_manager()->\
                 plf_get_flags_map().setFlag(*id, *value);
    // TODO(Rao): Throw an exception if found is false;
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
    int64_t val = 0;
    bool found = PlatformProcess::plf_manager()->\
                 plf_get_flags_map().getFlag(*id, val);
    // TODO(Rao): Throw an exception if found is false;
    return val;
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
    _return = PlatformProcess::plf_manager()->plf_get_flags_map().toMap();
}

/**
* @brief
*
* @param command
*/
bool PlatNetSvcHandler::setFault(boost::shared_ptr<std::string>& cmdline)  // NOLINT
{
    boost::char_separator<char> sep(", ");
    /* Parse the cmd line */
    boost::tokenizer<boost::char_separator<char>> toknzr(*cmdline, sep);
    std::unordered_map<std::string, std::string> args;
    for (auto t : toknzr) {
        if (args.size() == 0) {
            args["cmd"] = t;
            continue;
        }
        std::vector<std::string> parts;
        boost::split(parts, t, boost::is_any_of("= "));
        if (parts.size() != 2) {
            return false;
        }
        args[parts[0]] = parts[1];
    }
    if (args.count("cmd") == 0 || args.count("name") == 0) {
        return false;
    }
    /* Execute based on the cmd */
    auto cmd = args["cmd"];
    auto name = args["name"];
    if (cmd == "enable") {
        fiu_enable(name.c_str(), 1, NULL, 0);
        LOGDEBUG << "Enable fault: " << name;
    } else if (cmd == "enable_random") {
        // TODO(Rao): add support for this
    } else if (cmd == "disable") {
        fiu_disable(name.c_str());
        LOGDEBUG << "Disable fault: " << name;
    }
    return true;
}

/*
 * notify_node_info
 * ----------------
 */
void
PlatNetSvcHandler::notify_node_info(fpi::AsyncHdrPtr &h, fpi::NodeInfoMsgPtr &m)
{
    NodeWorkFlow::nd_workflow_sgt()->wrk_recv_node_info(h, m);
}

/**
 * notify_node_qualify
 * -------------------
 */
void
PlatNetSvcHandler::notify_node_qualify(fpi::AsyncHdrPtr &h, fpi::NodeQualifyPtr &m)
{
    NodeWorkFlow::nd_workflow_sgt()->wrk_recv_node_qualify(h, m);
}

/**
 * notify_node_upgrade
 * -------------------
 */
void
PlatNetSvcHandler::notify_node_upgrade(fpi::AsyncHdrPtr &h, fpi::NodeUpgradePtr &m)
{
    NodeWorkFlow::nd_workflow_sgt()->wrk_recv_node_upgrade(h, m);
}

/**
 * notify_node_integrate
 * ---------------------
 */
void
PlatNetSvcHandler::notify_node_integrate(fpi::AsyncHdrPtr &h, fpi::NodeIntegratePtr &m)
{
    NodeWorkFlow::nd_workflow_sgt()->wrk_recv_node_integrate(h, m);
}

/**
 * notify_node_deploy
 * ------------------
 */
void
PlatNetSvcHandler::notify_node_deploy(fpi::AsyncHdrPtr &h, fpi::NodeDeployPtr &m)
{
    NodeWorkFlow::nd_workflow_sgt()->wrk_recv_node_deploy(h, m);
}

/**
 * notify_node_functional
 * ----------------------
 */
void
PlatNetSvcHandler::notify_node_functional(fpi::AsyncHdrPtr &h, fpi::NodeFunctionalPtr &m)
{
    NodeWorkFlow::nd_workflow_sgt()->wrk_recv_node_functional(h, m);
}

/**
 * notify_node_down
 * ----------------
 */
void
PlatNetSvcHandler::notify_node_down(fpi::AsyncHdrPtr &h, fpi::NodeDownPtr &m)
{
    NodeWorkFlow::nd_workflow_sgt()->wrk_recv_node_down(h, m);
}

/**
 * notify_node_event
 * -----------------
 */
void
PlatNetSvcHandler::notify_node_event(fpi::AsyncHdrPtr &h, fpi::NodeEventPtr &m)
{
    fpi::SvcUuid       svc;
    fpi::DomainID      domain;
    std::stringstream  stt;
    fpi::NodeEventPtr  res;

    NodeWorkFlow::nd_workflow_sgt()->wrk_dump_steps(NULL);
    return;

    NodeWorkFlow::nd_workflow_sgt()->wrk_dump_steps(&stt);
    res = bo::make_shared<fpi::NodeEvent>();
    res->nd_dom_id   = domain;
    res->nd_uuid     = svc;
    res->nd_evt      = "";
    res->nd_evt_text = stt.str();
    sendAsyncResp(*h, FDSP_MSG_TYPEID(fpi::NodeEvent), *res);
}

/**
 * getSvcEvent
 * -----------
 */
void
PlatNetSvcHandler::getSvcEvent(fpi::NodeEvent &ret, fpi::NodeEventPtr &in)
{
    fpi::SvcUuid       svc;
    fpi::DomainID      domain;
    std::stringstream  stt;

    NodeWorkFlow::nd_workflow_sgt()->wrk_dump_steps(&stt);
    ret.nd_dom_id   = domain;
    ret.nd_uuid     = svc;
    ret.nd_evt      = "";
    ret.nd_evt_text = stt.str();
}

}  // namespace fds
