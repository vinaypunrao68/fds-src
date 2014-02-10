/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef TOKENCOPYSENDER_H_
#define TOKENCOPYSENDER_H_

#include <string>
#include <boost/msm/back/state_machine.hpp>

#include <fds_assert.h>
#include <concurrency/fds_actor.h>
#include <fdsp/FDSP_types.h>
#include <fds_base_migrators.h>
#include <util/Log.h>
#include <NetSession.h>
#include <StorMgrVolumes.h>

namespace fds {

using namespace  ::FDS_ProtocolInterface;

/* Forward declarations */
class FdsMigrationSvc;
struct TokenCopySenderFSM_;
typedef boost::msm::back::state_machine<TokenCopySenderFSM_> TokenCopySenderFSM;

class TokenCopySender : public MigrationSender,
                        public FdsRequestQueueActor {
public:
    TokenCopySender(FdsMigrationSvc *migrationSvc,
            SmIoReqHandler *data_store,
            const std::string &mig_id,
            fds_threadpoolPtr threadpool,
            fds_logPtr log,
            const std::string &rcvr_ip,
            const int &rcvr_port,
            const std::set<fds_token_id> &tokens,
            boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler);
    virtual ~TokenCopySender();

    fds_log* get_log() {
        return log_.get();
    }

    /* For logging */
    std::string log_string() {
        return "TokenCopySender";
    }

    void start();

    /* Overrides from FdsRequestQueueActor */
    virtual Error handle_actor_request(FdsActorRequestPtr req) override;

private:
    std::unique_ptr<TokenCopySenderFSM> sm_;
    fds_logPtr log_;
};

} /* namespace fds */

#endif /* TOKENCOPYSENDER_H_ */
