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

    /* Migration service message to initiate token copy */
    FAR_ENUM(MigSvcBulkCopyTokensReq),

    /* Migration service message to close syncs */
    FAR_ENUM(MigSvcSyncCloseReq),

    /* For starting Token receiver statemachine */
    FAR_ENUM(TRStart),

    /* TokenSender message that token data has been read */
    FAR_ENUM(TcsDataReadDone),

    /* TokenReceiver message that token data has been written */
    FAR_ENUM(TcrWrittenEvt),

    /* Token copy done event */
    FAR_ENUM(TRCopyDnEvt),

    /* Token receiver sync start event */
    FAR_ENUM(TRSyncStartEvt),

    /* Token sync snapshot is complete notification event */
    FAR_ENUM(TSnapDnEvt),

    /* Token receiver sync meta data is applied to Object store */
    FAR_ENUM(TRMdAppldEvt),

    /* Token receiver sync transfer is complete */
    FAR_ENUM(TRMdXferDnEvt),

    /* Token receiver sync resolve is complete */
    FAR_ENUM(TRResolveDnEvt),

    /* Token receiver sync process is complete */
    FAR_ENUM(TRSyncDnEvt),

    /* Token receiver pull request to pull statemachine */
    FAR_ENUM(TRPullReqEvt),

    /* Pull receiver object data is applied to Object store */
    FAR_ENUM(TRPullDataWrittenEvt),

    /* Token receiver pull is complete */
    FAR_ENUM(TRPullDnEvt),

    /* For starting Token sender statemachine */
    FAR_ENUM(TSStart),

    /* Token sender pull data has been read */
    FAR_ENUM(TSDataReadEvt),

    /*----------------- Last Request ------------------------------------------*/
    FAR_ENUM(Max)
};

/* Forward declarations */
class FdsActor;
class FdsRequestQueueActor;

class FdsActorRequest {
public:
    FdsActorRequest();

    FdsActorRequest(const FdsActorRequestType &type,
            boost::shared_ptr<void> payload);

    void recycle(const FdsActorRequestType &type,
            boost::shared_ptr<void> payload);

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
    FdsActorShutdownComplete(const std::string &id);

    std::string far_id;
};
typedef boost::shared_ptr<FdsActorShutdownComplete> FdsActorShutdownCompletePtr;

}  // namespace fds

#endif  // SOURCE_UTIL_ACTOR_REQUEST_H_
