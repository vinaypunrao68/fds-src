/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_UTIL_ACTOR_REQUEST_H_
#define SOURCE_UTIL_ACTOR_REQUEST_H_

#include <queue>
#include <boost/shared_ptr.hpp>

namespace fds {

/* Types.  These types are divided into ranges.  DO NOT change the order */
enum FdsActorRequestType {
    /*----------------- Migration request range [1000-2000) -------------------*/
    /* Migration service message to initiate token copy */
    FAR_MIGSVC_COPY_TOKEN = 1000,
    /* Migration service message that a migration is complete */
    FAR_MIGSVC_MIGRATION_COMPLETE,
    /* TockenCopySender message that token data has been read */
    FAR_MIG_TCS_DATA_READ_DONE,
    /* TockenCopyReceiver message that token data has been written */
    FAR_MIG_TCR_DATA_WRITE_DONE,
    /*----------------- Migration RPC -----------------------------------------*/
    /* RPC from receiver->sender to start token copy */
    FAR_MIG_COPY_TOKEN_RPC,
    /* RPC token copy response ack from sender->receiver */
    FAR_MIG_COPY_TOKEN_RESP_RPC,
    /* RPC from sender->receiver with token object data */
    FAR_MIG_PUSH_TOKEN_OBJECTS_RPC,
    /* RPC ack response from receiver->sender of token object data */
    FAR_MIG_PUSH_TOKEN_OBJECTS_RESP_RPC,

    /*----------------- Last Request ------------------------------------------*/
    FAR_MAX
};

/* Forward declarations */
class FdsActor;
class FdsRequestQueueActor;

class FdsActorRequest {
public:
    FdsActorRequest()
    : FdsActorRequest(FAR_MAX, nullptr)
    {
    }

    FdsActorRequest(FdsActorRequestType type, boost::shared_ptr<void> payload) {
        this->type = type;
        owner_ = nullptr;
        prev_owner_ = nullptr;
        this->payload = payload;
    }

    template <class PayloadT>
    boost::shared_ptr<PayloadT> get_payload() {
        return boost::static_pointer_cast<PayloadT>(payload);
    }

public:
    /* Request payload */
    boost::shared_ptr<void> payload;
    /* Request type */
    FdsActorRequestType type;

protected:
    /* Current owner of the request */
    FdsActor *owner_;
    /* Previous owner */
    FdsActor *prev_owner_;

    friend class FdsRequestQueueActor;
};

typedef boost::shared_ptr<FdsActorRequest> FdsActorRequestPtr;
typedef std::queue<FdsActorRequestPtr> FdsActorRequestQueue;

}  // namespace fds

#endif  // SOURCE_UTIL_ACTOR_REQUEST_H_
