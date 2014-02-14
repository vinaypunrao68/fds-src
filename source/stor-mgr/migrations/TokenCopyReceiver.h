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
#include <ClusterCommMgr.h>
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
    TokenCopyReceiver(FdsMigrationSvc *migrationSvc,
            SmIoReqHandler *data_store,
            const std::string &mig_id,
            fds_threadpoolPtr threadpool,
            fds_log *log,
            const std::set<fds_token_id> &tokens,
            boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler,
            ClusterCommMgrPtr clust_comm_mgr);
    virtual ~TokenCopyReceiver();

    fds_log* get_log() {
        return log_;
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

    /* Table of receiver state machines.  Each is keyed by a migration stream id */
    std::unordered_map<std::string, std::unique_ptr<TokenCopyReceiverFSM> > rcvr_sms_;

    /* Migration service reference */
    FdsMigrationSvc *migrationSvc_;

    fds_log *log_;

    /* Cluster communication manager reference */
    ClusterCommMgrPtr clust_comm_mgr_;
};

} /* namespace fds */

#endif /* TOKENCOPYRECEIVER_H_*/
