/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_PLATFORM_SERVICE_EP_LIB_H_
#define SOURCE_INCLUDE_PLATFORM_SERVICE_EP_LIB_H_

#include <net/net-service.h>
#include <fdsp/fds_service_types.h>

/**
 * Common shared library to provide SM/DM/AM endpoint services.  These services are
 * embeded in NodeAgent objects.
 */
namespace fds {

/**
 * SM Service EndPoint
 */
class SmSvcEp : public EpSvc
{
  public:
    typedef bo::intrusive_ptr<SmSvcEp> pointer;

    virtual ~SmSvcEp() {}
    SmSvcEp(const ResourceUUID &uuid, fds_uint32_t maj, fds_uint32_t min);

    void svc_receive_msg(const fpi::AsyncHdr &msg);
};

/**
 * DM Service EndPoint
 */
class DmSvcEp : public EpSvc
{
  public:
    typedef bo::intrusive_ptr<DmSvcEp> pointer;

    virtual ~DmSvcEp() {}
    DmSvcEp(const ResourceUUID &uuid, fds_uint32_t maj, fds_uint32_t min);

    void svc_receive_msg(const fpi::AsyncHdr &msg);
};

/**
 * AM Service EndPoint
 */
class AmSvcEp : public EpSvc
{
  public:
    typedef bo::intrusive_ptr<AmSvcEp> pointer;

    virtual ~AmSvcEp() {}
    AmSvcEp(const ResourceUUID &uuid, fds_uint32_t maj, fds_uint32_t min);

    void svc_receive_msg(const fpi::AsyncHdr &msg);
};

/**
 * OM Service EndPoint
 */
class OmSvcEp : public EpSvc
{
  public:
    typedef bo::intrusive_ptr<OmSvcEp> pointer;

    virtual ~OmSvcEp() {}
    OmSvcEp(const ResourceUUID &uuid, fds_uint32_t maj, fds_uint32_t min);

    void svc_receive_msg(const fpi::AsyncHdr &msg);
};

/**
 * PM Service EndPoint
 */
class PmSvcEp : public EpSvc
{
  public:
    typedef bo::intrusive_ptr<PmSvcEp> pointer;

    virtual ~PmSvcEp() {}
    PmSvcEp(const ResourceUUID &uuid, fds_uint32_t maj, fds_uint32_t min);

    void svc_receive_msg(const fpi::AsyncHdr &msg);
};

}  // namespace fds
#endif  // SOURCE_INCLUDE_PLATFORM_SERVICE_EP_LIB_H_
