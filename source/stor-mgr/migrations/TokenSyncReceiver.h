/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef TOKENSYNCRECEIVER_H_
#define TOKENSYNCRECEIVER_H_

#include <string>
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
#include <TokenSyncBaseEvents.h>


namespace fds {

using namespace  ::FDS_ProtocolInterface;

/* Forward declarations */
class TokenCopyReceiver;

/* Statemachine Events */
struct TokMdEvt {};
struct SyncAckdEvt{};
struct TSMdAppldEvt {};
struct NeedPullEvt {};
struct TSXferDnEvt {};
struct PullDnEvt {};
struct ResolveEvt {};
struct TSResolveDnEvt {};

struct TokenSyncReceiverFSM_;
/* back-end */
typedef boost::msm::back::state_machine<TokenSyncReceiverFSM_> TokenSyncReceiverFSM;

/**
 * Token sync start event
 */
struct TSStartEvt {
    TSStartEvt(const uint64_t &start, const uint64_t &end) {
        start_time = start;
        end_time = end;
    }
    /* start time */
    uint64_t start_time;
    /* End time.  zero means end time isn't specified */
    uint64_t end_time;
};

struct TokenSyncReceiver {
public:
    TokenSyncReceiver();
    virtual ~TokenSyncReceiver();

    void init(const std::string &mig_stream_id,
            FdsMigrationSvc *migrationSvc,
            TokenCopyReceiver *parent,
            SmIoReqHandler *data_store,
            const std::string &rcvr_ip,
            const int &rcvr_port,
            const fds_token_id &token_id,
            netMigrationPathClientSession *sender_session,
            boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler);
    void process_event(const TSStartEvt& evt);
 private:
    TokenSyncReceiverFSM *fsm_;
};
}  // namespace fds

#endif  /* TOKENSYNCRECEIVER_H_*/
