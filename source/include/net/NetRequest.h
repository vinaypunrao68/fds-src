/* Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_NETREQUEST_H_
#define SOURCE_INCLUDE_NET_NETREQUEST_H_

#include <string>
#include <net/RpcRequest.h>
#include <fdsp_utils.h>

namespace fds {

/**
* @brief Base class for message based network requests
*/
struct NetRequest
{
    NetRequest();
    virtual ~NetRequest();

    template<class PayloadT>
    void setPayload(const fpi::FDSPMsgTypeId &msgTypeId,
                    const boost::shared_ptr<PayloadT> &payload)
    {
        boost::shared_ptr<std::string>buf;
        /* NOTE: Doing the serialization on calling thread */
        fds::serializeFdspMsg(*payload, buf);
        setPayloadBuf(msgTypeId, buf);
    }

    void setPayloadBuf(const fpi::FDSPMsgTypeId &msgTypeId,
                       boost::shared_ptr<std::string> &buf);

 protected:
    /* Message type id */
    fpi::FDSPMsgTypeId msgTypeId_;
    /* Payload buffer */
    boost::shared_ptr<std::string> payloadBuf_;
};

/**
* @brief EP style message based network request
*/
struct EPNetRequest : EPAsyncRpcRequest, NetRequest
{
    EPNetRequest(const AsyncRpcRequestId& id,
                       const fpi::SvcUuid &myEpId,
                       const fpi::SvcUuid &peerEpId);
    void setRpcFunc(RpcFunc rpc) = delete;
    virtual void invoke() override;
};
typedef boost::shared_ptr<EPNetRequest> EPNetRequestPtr;

/**
* @brief Failover style message based network request
*/
struct FailoverNetRequest : FailoverRpcRequest, NetRequest
{
    FailoverNetRequest(const AsyncRpcRequestId& id,
                       const fpi::SvcUuid &myEpId,
                       const EpIdProviderPtr epProvider);
    void setRpcFunc(RpcFunc rpc) = delete;
    virtual void invoke() override;
};
typedef boost::shared_ptr<FailoverNetRequest> FailoverNetRequestPtr;

/**
* @brief Quorum style message based network request
*/
struct QuorumNetRequest : QuorumRpcRequest, NetRequest
{
    QuorumNetRequest(const AsyncRpcRequestId& id,
                       const fpi::SvcUuid &myEpId,
                       const EpIdProviderPtr epProvider);
    void setRpcFunc(RpcFunc rpc) = delete;
    virtual void invoke() override;
};
typedef boost::shared_ptr<QuorumNetRequest> QuorumNetRequestPtr;
}  // namespace fds

#endif  // SOURCE_INCLUDE_NET_NETREQUEST_H_
