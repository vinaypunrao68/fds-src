/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef TOKENCOPYRECEIVER_H_
#define TOKENCOPYRECEIVER_H_

#include <string>
#include <vector>
#include <memory>
#include <boost/msm/back/state_machine.hpp>

#include <fds_assert.h>
#include <concurrency/fds_actor.h>
#include <fdsp/FDSP_types.h>
#include <fds_base_migrators.h>
#include <util/Log.h>
#include <NetSession.h>


namespace fds {

using namespace  ::FDS_ProtocolInterface;

struct TokenCopyReceiverFSM_;
/* back-end */
typedef boost::msm::back::state_machine<TokenCopyReceiverFSM_> TokenCopyReceiverFSM;

class TokenCopyReceiver : public MigrationReceiver,
                          public FdsRequestQueueActor {
public:
    TokenCopyReceiver(const std::string &migration_id,
            fds_threadpoolPtr threadpool,
            fds_logPtr log,
            const std::string &sender_ip,
            const std::set<fds_token_id> &tokens,
            boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler);
    virtual ~TokenCopyReceiver();

    fds_log* get_log() {
        return log_.get();
    }

    /* For logging */
    std::string log_string() {
        return "TokenCopyReceiver";
    }

    virtual void start();

    /* Overrides from FdsRequestQueueActor */
    virtual Error handle_actor_request(FdsActorRequestPtr req) override;

private:
    std::unique_ptr<TokenCopyReceiverFSM> sm_;
    fds_logPtr log_;
};

} /* namespace fds */

#endif /* TOKENCOPYRECEIVER_H_*/
