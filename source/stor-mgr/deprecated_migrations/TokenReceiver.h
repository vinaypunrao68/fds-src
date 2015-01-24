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
// #include <NetSession.h>
#include <ClusterCommMgr.h>
#include <StorMgrVolumes.h>

namespace fds {

using namespace  ::FDS_ProtocolInterface;

/* Forward declarations */
class FdsMigrationSvc;
struct TokenSyncReceiver;
struct TokenPullReceiver;

struct TokenCopyReceiverFSM_;
typedef boost::msm::back::state_machine<TokenCopyReceiverFSM_> TokenCopyReceiverFSM;

class TokenReceiver : public MigrationReceiver,
                          public FdsRequestQueueActor {
public:
    TokenReceiver(FdsMigrationSvc *migrationSvc,
            SmIoReqHandler *data_store,
            const std::string &mig_id,
            fds_threadpoolPtr threadpool,
            fds_log *log,
            const fds_token_id &token,
            boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler,
            ClusterCommMgrPtr clust_comm_mgr);
    virtual ~TokenReceiver();

    fds_log* get_log() {
        return log_;
    }

    /* For logging */
    std::string log_string() {
        return "TokenReceiver";
    }

    virtual void start();

    /* Overrides from FdsRequestQueueActor */
    virtual Error handle_actor_request(FdsActorRequestPtr req) override;

protected:
    void destroy_migration_stream(const std::string &mig_stream_id);

    /* Token we are copying */
    fds_token_id token_id_;

    std::unique_ptr<TokenCopyReceiverFSM> copy_fsm_;
    /* sync state machine.  Couldn't make it unique_ptr.  I was getting compiler
     * errors that I couldn't figure out
     */
    TokenSyncReceiver *sync_fsm_;

    TokenPullReceiver *pull_fsm_;

    /* Migration service reference */
    FdsMigrationSvc *migrationSvc_;

    fds_log *log_;

    /* Cluster communication manager reference */
    ClusterCommMgrPtr clust_comm_mgr_;
    DBG(bool destroy_sent_);
};

} /* namespace fds */

#endif /* TOKENCOPYRECEIVER_H_*/
