/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <TokenCopyReceiver.h>

namespace fds {

TokenCopyReceiver::TokenCopyReceiver(fds_threadpoolPtr threadpool)
    : FdsRequestQueueActor(threadpool)
{
}

TokenCopyReceiver::~TokenCopyReceiver() {
}

Error TokenCopyReceiver::handle_actor_request(FdsActorRequestPtr req)
{
    return ERR_OK;
}
} /* namespace fds */
