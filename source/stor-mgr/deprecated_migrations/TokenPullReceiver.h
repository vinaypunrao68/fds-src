/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef TOKENPULLRECEIVER_H_
#define TOKENPULLRECEIVER_H_

#include <string>
#include <list>
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
#include <TokenSyncBaseEvents.h>


namespace fds {

using namespace  ::FDS_ProtocolInterface;

/* Forward declarations */
class TokenReceiver;
struct TokenPullReceiverFSM_;
typedef boost::msm::back::state_machine<TokenPullReceiverFSM_> TokenPullReceiverFSM;

/* Statemachine Events */

/* Last pull request event.  After this no more TRPullReqEvt will be sent*/
struct TRLastPullReqEvt {};

typedef FDSP_PushObjectsReq TRDataPulledEvt;

typedef SmIoApplyObjectdata TRPullDataWrittenEvt;
typedef boost::shared_ptr<TRPullDataWrittenEvt> TRPullDataWrittenEvtPtr;

struct TokenPullReceiver {
public:
    TokenPullReceiver();
    virtual ~TokenPullReceiver();

    void init(const std::string &mig_stream_id,
            FdsMigrationSvc *migrationSvc,
            TokenReceiver *parent,
            SmIoReqHandler *data_store,
            const std::string &rcvr_ip,
            const int &rcvr_port,
            const fds_token_id &token_id,
            netMigrationPathClientSession *sender_session,
            boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler);
    void start();
    void process_event(const TRPullReqEvt& evt);
    void process_event(const TRDataPulledEvt& evt);
    void process_event(const TRPullDataWrittenEvt& evt);
    void process_event(const TRLastPullReqEvt& evt);

 private:
    TokenPullReceiverFSM *fsm_;
};
}  // namespace fds

#endif  /* TOKENPULLRECEIVER_H_*/
