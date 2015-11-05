/* Copyright 2015 Formation Data Systems, Inc.
 */
#include <net/replica/Replica.h>
namespace fds {
ReplicaGroupBroadcastRequest::ReplicaGroupBroadcastRequest(ReplicaGroupHandle *groupHandle)
    : groupHandle_(groupHandle)
{
}

void ReplicaGroupBroadcastRequest::invoke()
{
    auto &functionalReplicas = groupHandle_->getFunciontalReplicas();    
    for (const auto& replicaId : functionalReplicas) {
        fpi::SvcUuid peerSvcId;
        peerSvcId.svc_uuid = replicaId;
        sendPayload_(peerSvcId);
    }
}

void ReplicaGroupBroadcastRequest::handleResponse(SHPTR<fpi::AsyncHdr>& header,
                                                  SHPTR<std::string>& payload)
{
    groupHandle_->handleReplicaResponse(this, header, payload);
}

ReplicaGroupHandle::ReplicaGroupHandle(CommonModuleProviderIf* provider)
: HasModuleProvider(provider) 
{
}

void ReplicaGroupHandle::handleReplicaResponse(ReplicaGroupBroadcastRequest* req,
                                               SHPTR<fpi::AsyncHdr>& header,
                                               SHPTR<std::string>& payload)
{
}
}  // namespace fds
