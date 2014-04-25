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

FdsActorRequest::FdsActorRequest(const FdsActorRequestType &t, boost::shared_ptr<void> p)
{
    recycle(t, p);
}

void FdsActorRequest::recycle(const FdsActorRequestType &t, boost::shared_ptr<void> p)
{
    this->type = t;
    owner_ = nullptr;
    prev_owner_ = nullptr;
    this->payload = p;
}
FdsActorShutdownComplete::FdsActorShutdownComplete(const std::string &id)
    : far_id(id)
{
}
}  // namespace fds
