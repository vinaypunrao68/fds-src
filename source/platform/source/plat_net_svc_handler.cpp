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

#include <fiu-control.h>
#include <fds_process.h>
#include <fdsp/fds_service_types.h>
#include <net/SvcMgr.h>
#include <net/PlatNetSvcHandler.h>

namespace fds
{
    PlatNetSvcHandler::PlatNetSvcHandler()
    {
        REGISTER_FDSP_MSG_HANDLER(fpi::UpdateSvcMapMsg, updateSvcMap);
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
        if (!g_fdsprocess)
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
        if (!g_fdsprocess)
        {
            return;
        }

        if (*id == "*")
        {
            /* Request to return all counters */
            g_fdsprocess->get_cntrs_mgr()->toMap(_return);
            return;
        }

        auto    cntrs = g_fdsprocess->get_cntrs_mgr()->get_counters(*id);

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
        if (!g_fdsprocess)
        {
            return;
        }

        if (*id == "*")
        {
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
        if (!g_fdsprocess)
        {
            return;
        }

        try
        {
            g_fdsprocess->get_fds_config()->set(*id, *val);
        } catch(...)
        {
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

    void PlatNetSvcHandler::updateSvcMap(fpi::AsyncHdrPtr &header,
                                            fpi::UpdateSvcMapMsgPtr &svcMapMsg)
    {
        MODULEPROVIDER()->getSvcMgr()->updateSvcMap(svcMapMsg->updates);
    }
}  // namespace fds
