/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <platform/service-ep-lib.h>

namespace fds {

/**
 * SM Service EndPoint.
 */
SmSvcEp::SmSvcEp(const ResourceUUID &uuid, fds_uint32_t maj, fds_uint32_t min)
    : EpSvc(uuid, fpi::FDSP_STOR_MGR) {}

void
SmSvcEp::svc_receive_msg(const fpi::AsyncHdr &msg)
{
}

/**
 * DM Service EndPoint.
 */
DmSvcEp::DmSvcEp(const ResourceUUID &uuid, fds_uint32_t maj, fds_uint32_t min)
    : EpSvc(uuid, fpi::FDSP_DATA_MGR) {}

void
DmSvcEp::svc_receive_msg(const fpi::AsyncHdr &msg)
{
}

/**
 * AM Service EndPoint.
 */
AmSvcEp::AmSvcEp(const ResourceUUID &uuid, fds_uint32_t maj, fds_uint32_t min)
    : EpSvc(uuid, fpi::FDSP_STOR_HVISOR) {}

void
AmSvcEp::svc_receive_msg(const fpi::AsyncHdr &msg)
{
}

/**
 * OM Service EndPoint.
 */
OmSvcEp::OmSvcEp(const ResourceUUID &uuid, fds_uint32_t maj, fds_uint32_t min)
    : EpSvc(uuid, fpi::FDSP_ORCH_MGR) {}

void
OmSvcEp::svc_receive_msg(const fpi::AsyncHdr &msg)
{
}

/**
 * PM Service EndPoint.
 */
PmSvcEp::PmSvcEp(const ResourceUUID &uuid, fds_uint32_t maj, fds_uint32_t min)
    : EpSvc(uuid, fpi::FDSP_PLATFORM) {}

void
PmSvcEp::svc_receive_msg(const fpi::AsyncHdr &msg)
{
}

}  // namespace fds
