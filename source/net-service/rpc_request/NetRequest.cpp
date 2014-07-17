/* Copyright 2014 Formation Data Systems, Inc. */
#include <string>
#include <net/NetRequest.h>
#include <net/net-service.h>
#include <net/net-service-tmpl.hpp>

namespace fds {
NetRequest::NetRequest()
{
}

NetRequest::~NetRequest()
{
}

void NetRequest::setPayloadBuf(const fpi::FDSPMsgTypeId &msgTypeId,
                               boost::shared_ptr<std::string> &buf)
{
    fds_assert(!payloadBuf_);
    msgTypeId_ = msgTypeId;
    payloadBuf_ = buf;
}

EPNetRequest::EPNetRequest(const AsyncRpcRequestId& id,
                           const fpi::SvcUuid &myEpId,
                           const fpi::SvcUuid &peerEpId)
: EPAsyncRpcRequest(id, myEpId, peerEpId)
{
}

void EPNetRequest::invoke()
{
    fds_assert(payloadBuf_);
    EPAsyncRpcRequest::setRpcFunc(CREATE_NET_REQUEST_RPC());
    EPAsyncRpcRequest::invoke();
}

FailoverNetRequest::FailoverNetRequest(const AsyncRpcRequestId& id,
                                             const fpi::SvcUuid &myEpId,
                                             const EpIdProviderPtr epProvider)
: FailoverRpcRequest(id, myEpId, epProvider)
{
}

void FailoverNetRequest::invoke()
{
    fds_assert(payloadBuf_);
    FailoverRpcRequest::setRpcFunc(CREATE_NET_REQUEST_RPC());
    FailoverRpcRequest::invoke();
}

QuorumNetRequest::QuorumNetRequest(const AsyncRpcRequestId& id,
                                   const fpi::SvcUuid &myEpId,
                                   const EpIdProviderPtr epProvider)
: QuorumRpcRequest(id, myEpId, epProvider)
{
}

void QuorumNetRequest::invoke()
{
    fds_assert(payloadBuf_);
    QuorumRpcRequest::setRpcFunc(CREATE_NET_REQUEST_RPC());
    QuorumRpcRequest::invoke();
}
}  // namespace fds
