/* Copyright 2015 Formation Data Systems, Inc.
 */
#include <VolumeSyncRunner.h>
#include <VolumeMeta.h>
#include <net/SvcMgr.h>
#include <net/SvcRequestPool.h>

namespace fds {

template <class T>
void SyncProtocolRunner<T>::run()
{
    // TODO(Rao): Invoking these callbacks in the context of qos
    notifyCoordinator([this](EPSvcRequest*,
                             const Error &e,
                             StringPtr) {
        if (e != ERR_OK) {
            complete(e, "Coordinator rejected at loading state");
            return;
        }
        copyActiveStateFromPeer([this](const Error &e) {
           if (e != ERR_OK) {
               complete(e, "Failed to copy active state");
               return;
           }
           replica_->setState(fpi::ResourceState::Syncing);
           /* Notify coordinator we are in syncing state.  From this point
            * we will receive actio io.  Active io will be buffered to disk
            */
           notifyCoordinator(nullptr);
           doQucikSyncWithPeer([this](const Error &e) {
               auto replayBufferedIoWork = [this](const Error &e) {
                   if (e != ERR_OK) {
                       complete(e, "Failed to do sync");
                       return;
                   }
                   replayBufferedIo([this](const Error &e) {
                       if (e != ERR_OK) {
                           complete(e, "Failed to replay buffered io");
                           return;
                       }
                       replica_->setState(fpi::ResourceState::Active);
                       notifyCoordinator(nullptr);
                   });
               };
               if (e == ERR_QUICKSYNC_NOT_POSSIBLE) {
                   doStaticMigrationWithPeer(replayBufferedIoWork);
                   return;
               }
               replayBufferedIoWork(e);
            });
        });
    });
}

struct VolumeSyncRunner : SyncProtocolRunner<VolumeMeta> {
    using SyncProtocolRunner<VolumeMeta>::SyncProtocolRunner;

    void notifyCoordinator(const EPSvcRequestRespCb &cb) override;
    void copyActiveStateFromPeer(const StatusCb &cb) override; 
    void doQucikSyncWithPeer(const StatusCb &cb) override;
    void doStaticMigrationWithPeer(const StatusCb &cb) override;
    void replayBufferedIo(const StatusCb &cb) override;
    void abort() override;
    void complete(const Error &e, const std::string &context) override;
};

void VolumeSyncRunner::notifyCoordinator(const EPSvcRequestRespCb &cb)
{
    auto msg = fpi::AddToVolumeGroupCtrlMsgPtr(new fpi::AddToVolumeGroupCtrlMsg);
    msg->targetState = replica_->getState();
    msg->groupId = replica_->getId();
    msg->replicaVersion = replica_->getVersion();
    msg->svcUuid = MODULEPROVIDER()->getSvcMgr()->getSelfSvcUuid();
    msg->lastOpId = replica_->getOpId();
    msg->lastCommitId = replica_->getSequenceId();

    auto requestMgr = MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr();
    auto req = requestMgr->newEPSvcRequest(replica_->getCoordinatorId());
    req->setPayload(FDSP_MSG_TYPEID(fpi::AddToVolumeGroupCtrlMsg), msg);
    if (cb) {
        // TODO(Rao): Ensure cb is invoked on qos context
        req->onResponseCb(cb);
    }
    req->invoke();
}

void VolumeSyncRunner::copyActiveStateFromPeer(const StatusCb &cb)
{
    // TODO(Neil): Please fill this
}

void VolumeSyncRunner::doQucikSyncWithPeer(const StatusCb &cb)
{
}

void VolumeSyncRunner::doStaticMigrationWithPeer(const StatusCb &cb)
{
    // TODO(Neil/James): Please fill this
}

void VolumeSyncRunner::replayBufferedIo(const StatusCb &cb)
{
    // TODO(Rao):
}

void VolumeSyncRunner::abort()
{
    // TODO(Rao):
}

void VolumeSyncRunner::complete(const Error &e, const std::string &context)
{
    // TODO(Rao):
}

}  // namespace fds
