/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_PLAT_NET_EP_PTR_H_
#define SOURCE_INCLUDE_PLATFORM_PLAT_NET_EP_PTR_H_

// This a transitional file

#include <fdsp/PlatNetSvc.h>

namespace fds
{
    template <class SendIf, class RecvIf>class EndPoint;

    typedef EndPoint<fpi::PlatNetSvcClient, fpi::PlatNetSvcProcessor> PlatNetEp;
    typedef bo::intrusive_ptr<PlatNetEp> PlatNetEpPtr;

}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_PLAT_NET_EP_PTR_H_
