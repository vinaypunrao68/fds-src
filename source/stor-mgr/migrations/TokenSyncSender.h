/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef TOKENSYNC_SENDER_H_
#define TOKENSYNC_SENDER_H_

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

struct SyncReqEvt {};

/* Snapshot is complete notification event */
struct TSSnapDnEvt {
    TSSnapDnEvt(const leveldb::ReadOptions& options, leveldb::DB* db)
    {
        this->options = options;
        this->db = db;
    }
    leveldb::ReadOptions options;
    leveldb::DB* db;
};
typedef boost::shared_ptr<TSSnapDnEvt> TSSnapDnEvtPtr;

struct BldSyncLogDnEvt {};
struct SendDnEvt {};
struct IoClosedEvt {};
struct SyncDnAckEvt {};

struct TokenSyncSenderFSM_;
/* back-end */
typedef boost::msm::back::state_machine<TokenSyncSenderFSM_> TokenSyncSenderFSM;
struct TokenSyncSender {
    void init(const std::string &mig_stream_id,
            FdsMigrationSvc *migrationSvc,
            TokenCopySender *parent,
            SmIoReqHandler *data_store,
            const std::string &rcvr_ip,
            const int &rcvr_port,
            fds_token_id token_id,
            boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler);
private:
    std::unique_ptr<TokenSyncSenderFSM> fsm_;
};
}  // namespace fds

#endif  /* TOKENSYNC_SENDER_H_*/
