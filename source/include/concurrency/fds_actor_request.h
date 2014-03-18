/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_UTIL_ACTOR_REQUEST_H_
#define SOURCE_UTIL_ACTOR_REQUEST_H_

#include <queue>
#include <boost/shared_ptr.hpp>

/* DO NOT use this macro outside.  Use FAR_ID() instead */
#define FAR_ENUM(type) type##_far_enum

/* Maps the input class/struct type to FdsActorRequestType.
 * Externally use this macro
 */
#define FAR_ID(type) FAR_ENUM(type)

namespace fds {

/* Enum ids for identifying FdsActorRequest payload types.
 * NOTE: The ids are named such that they match the payload type.  When
 * you rename the payload type don't forget to update here
 */
enum FdsActorRequestType {
    /*------------------ Actor specific request -------------------------------*/
    /* For shutting down fds actor */
    FAR_ENUM(FdsActorShutdown),
    /* Notification from FdsActor to it's parent that shutdown is complete */
    FAR_ENUM(FdsActorShutdownComplete),

    /*----------------- Migration request range [1000-2000) -------------------*/
    /* Migration service message to initiate token copy */
    FAR_ENUM(MigSvcCopyTokensReq) = 1000,

    /* TokenCopySender message that token data has been read */
    FAR_ENUM(TcsDataReadDone),

    /* TokenCopyReceiver message that token data has been written */
    FAR_ENUM(TcrWrittenEvt),

    /* TokenCopyReceiver message that a copy stream is finished */
    FAR_ENUM(TcrDestroyEvt),

    /* Token sync snapshot is complete notification event */
    FAR_ENUM(TSSnapDnEvt),

    /* Token sync transfer is complete */
    FAR_ENUM(TSXferDnEvt),

    /* Token sync resolve is complete */
    FAR_ENUM(TSResolveDnEvt),

    /* Token sync meta data is applied to Object store */
    FAR_ENUM(TSMdAppldEvt),
    /*----------------- Migration RPC -----------------------------------------*/
    /* RPC from receiver->sender to start token copy */
    FAR_ENUM(FDSP_CopyTokenReq),

    /* RPC token copy response ack from sender->receiver */
    FAR_ENUM(FDSP_CopyTokenResp),

    /* RPC from sender->receiver with token object data */
    FAR_ENUM(FDSP_PushTokenObjectsReq),

    /* RPC ack response from receiver->sender of token object data */
    FAR_ENUM(FDSP_PushTokenObjectsResp),

    /* RPC ack response from receiver->sender of token metadata */
    FAR_ENUM(FDSP_PushTokenMetadataResp),
    /*----------------- Last Request ------------------------------------------*/
    FAR_ENUM(Max)
};

/* Forward declarations */
class FdsActor;
class FdsRequestQueueActor;

class FdsActorRequest {
public:
    FdsActorRequest()
    : FdsActorRequest(FAR_ID(Max), nullptr)
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

/**
 * Notification FdsActor to its parent that shutdown is complete
 */
class FdsActorShutdownComplete {
public:
    FdsActorShutdownComplete(const std::string &id)
    : far_id(id)
    {}

    std::string far_id;
};
typedef boost::shared_ptr<FdsActorShutdownComplete> FdsActorShutdownCompletePtr;

}  // namespace fds

#endif  // SOURCE_UTIL_ACTOR_REQUEST_H_
