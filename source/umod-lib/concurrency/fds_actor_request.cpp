/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <string>

#include <concurrency/fds_actor_request.h>

namespace fds {

FdsActorRequest::FdsActorRequest()
    : FdsActorRequest(FAR_ID(Max), nullptr)
{
}

FdsActorRequest::FdsActorRequest(const FdsActorRequestType &type,
        boost::shared_ptr<void> payload)
{
    recycle(type, payload);
}

void FdsActorRequest::recycle(const FdsActorRequestType &type,
        boost::shared_ptr<void> payload)
{
    this->type = type;
    owner_ = nullptr;
    prev_owner_ = nullptr;
    this->payload = payload;
}
FdsActorShutdownComplete::FdsActorShutdownComplete(const std::string &id)
    : far_id(id)
{
}
}  // namespace fds
