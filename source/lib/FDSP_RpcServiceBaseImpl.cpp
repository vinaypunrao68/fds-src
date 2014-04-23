/*
 * Copyright 2014 By Formation Data Systems, Inc.
 */
#include <map>
#include <string>

#include <FDSP_RpcServiceBaseImpl.h>
#include <fds_assert.h>
#include <fds_process.h>

namespace FDS_ProtocolInterface {

FDSP_RpcServiceStatus FDSP_RpcServiceBaseImpl::GetStatus(const int32_t nullarg)
{
    fds_verify(!"Shouldn't be implemented");
    return SVC_STATUS_INVALID;
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
    if (!process_)
        return SVC_STATUS_INVALID;
    return SVC_STATUS_ACTIVE;
}

/**
 * Returns the stats associated with counters identified by id
 * @param _return
 * @param id
 */
void FDSP_RpcServiceBaseImpl::GetStats(std::map<std::string, int64_t> & _return,
        boost::shared_ptr<std::string>& id)
{
    if (!process_) return;

    auto cntrs = process_->get_cntrs_mgr()->get_counters(*id);
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
    if (!process_) return;

    try {
        process_->get_fds_config()->set(*id, *val);
    } catch(...) {
        // TODO(Rao): Only ignore SettingNotFound exception
        /* Ignore the error */
    }
}

/**
 * For setting process
 * @param p
 */
void FDSP_RpcServiceBaseImpl::
SetFdsProcess(fds::FdsProcess *p)
{
    fds_assert(process_ == nullptr);
    process_ = p;
}
}  // namespace FDS_ProtocolInterface
