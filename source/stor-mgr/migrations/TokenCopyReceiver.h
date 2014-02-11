/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef TOKENCOPYRECEIVER_H_
#define TOKENCOPYRECEIVER_H_

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
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

struct TokenCopyReceiverFSM_;
/* back-end */
typedef boost::msm::back::state_machine<TokenCopyReceiverFSM_> TokenCopyReceiverFSM;

class TokenCopyReceiver : public MigrationReceiver,
                          public FdsRequestQueueActor {
public:
    typedef std::unordered_map<std::string, std::set<fds_token_id> > IpTokenTable;
public:
    TokenCopyReceiver(FdsMigrationSvc *migrationSvc,
            SmIoReqHandler *data_store,
            const std::string &mig_id,
            fds_threadpoolPtr threadpool,
            fds_logPtr log,
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

protected:
    template<class EventT>
    void route_to_mig_stream(const std::string &mig_stream_id,
            const EventT &event);
    void destroy_migration_stream(const std::string &mig_stream_id);
    IpTokenTable get_ip_token_tbl(const std::set<fds_token_id>& tokens);

    /* Table of receiver state machines.  Each is keyed by a migration stream id */
    std::unordered_map<std::string, std::unique_ptr<TokenCopyReceiverFSM> > rcvr_sms_;
    FdsMigrationSvc *migrationSvc_;
    fds_logPtr log_;
};

} /* namespace fds */

#endif /* TOKENCOPYRECEIVER_H_*/
