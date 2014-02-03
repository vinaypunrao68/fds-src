/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <TokenCopySender.h>

namespace fds {

TokenCopySender::TokenCopySender(fds_threadpoolPtr threadpool)
    : FdsRequestQueueActor(threadpool)
{
}

TokenCopySender::~TokenCopySender() {
}

Error TokenCopySender::handle_actor_request(FdsActorRequestPtr req)
{
    return ERR_OK;
}
} /* namespace fds */
