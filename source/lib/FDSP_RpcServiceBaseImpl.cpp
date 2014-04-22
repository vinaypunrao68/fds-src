/*
 * Copyright 2014 By Formation Data Systems, Inc.
 */
#include <map>
#include <string>

#include <FDSP_RpcServiceBaseImpl.h>
#include <fds_assert.h>
#include <fds_counters.h>
#include <fds_config.hpp>

namespace FDS_ProtocolInterface {

FDSP_RpcServiceStatus FDSP_RpcServiceBaseImpl::GetStatus(const int32_t nullarg)
{
    return SERVICE_ACTIVE;
}
void FDSP_RpcServiceBaseImpl::GetStats(std::map<std::string, int64_t> & _return,
        const std::string& id)
{
    fds_verify(!"Shouldn't be implemented");
}
void FDSP_RpcServiceBaseImpl::SetConfigVal(const std::string& id, const int64_t val)
{
    fds_verify(!"Shouldn't be implemented");
}

/**
 * Returns the status of the service
 * @param nullarg
 * @return
 */
FDSP_RpcServiceStatus FDSP_RpcServiceBaseImpl::
GetStatus(boost::shared_ptr<int32_t>& nullarg)  // NOLINT
{
    return SERVICE_ACTIVE;
}

/**
 * Returns the stats associated with counters identified by id
 * @param _return
 * @param id
 */
void FDSP_RpcServiceBaseImpl::GetStats(std::map<std::string, int64_t> & _return,
        boost::shared_ptr<std::string>& id)
{
    auto cntrs = cntrsMgr_->get_counters(*id);
    if (cntrs == nullptr) {
        return;
    }
    cntrs->toMap(_return);
}

/**
 * For setting a flag dynamically
 * @param id
 * @param val
 */
void FDSP_RpcServiceBaseImpl::
SetConfigVal(boost::shared_ptr<std::string>& id,  // NOLINT
            boost::shared_ptr<int64_t>& val)
{
    try {
        config_->set(*id, *val);
    } catch(...) {
        // TODO(Rao): Only ignore SettingNotFound exception
        /* Ignore the error */
    }
}

/**
 * Registers counter manager
 * @param cntrsMgr
 */
void FDSP_RpcServiceBaseImpl::RegisterCountersMgr(
        const boost::shared_ptr<fds::FdsCountersMgr> &cntrsMgr)
{
    cntrsMgr_ = cntrsMgr;
}

/**
 * Registers config
 * @param config
 */
void FDSP_RpcServiceBaseImpl::
RegisterFdsConfig(const boost::shared_ptr<fds::FdsConfig> &config)
{
    config_ = config;
}
}  // namespace FDS_ProtocolInterface
