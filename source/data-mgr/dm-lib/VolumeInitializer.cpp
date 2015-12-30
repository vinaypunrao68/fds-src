/* Copyright 2015 Formation Data Systems, Inc.
 */
#include <VolumeInitializer.h>
#include <VolumeMeta.h>
#include <net/SvcMgr.h>
#include <net/SvcRequestPool.h>

namespace fds {

#define volSynchronized(func) func

template <class T>
ReplicaInitializer<T>::ReplicaInitializer(CommonModuleProviderIf *provider, T *replica)
: HasModuleProvider(provider),
  replica_(replica)
{
}

template <class T>
ReplicaInitializer<T>::~ReplicaInitializer()
{
}

template <class T>
void ReplicaInitializer<T>::run()
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
           enableWriteOpsBuffering_();
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

template <class T>
Error ReplicaInitializer<T>::tryAndBufferIo(const BufferReplay::Op &op)
{
    fds_assert(replica_->getState() == fpi::Syncing);

    return bufferReplay_->buffer(op);
}

template <class T>
void ReplicaInitializer<T>::enableWriteOpsBuffering_()
{
    bufferReplay_.reset(new BufferReplay("buffered_writes",
                                         512,  /* Replay batch size */
                                         MODULEPROVIDER()->proc_thrpool()));
#if 0
    bufferReplay_->setProgressCb(volSynchronized([this](BufferReplay::Progress progress) {
        // TODO(Rao): Exit checks
        LOGNOTIFY << replica->logString() << " BufferReplay progress: " << progress;
        if (progress == BufferReplay::COMPLETE) {
            complete(ERR_OK, "BufferReplay progress callback");
        } else if (progress == BufferReplay::ABORTED) {
            complete(ERR_ABORTED, "BufferReplay progress callback");
        }
    }));
#endif
    // TODO(Rao): op callbacks
}

void VolumeInitializer::notifyCoordinator(const EPSvcRequestRespCb &cb)
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

void VolumeInitializer::copyActiveStateFromPeer(const StatusCb &cb)
{
    // TODO(Neil): Please fill this
}

void VolumeInitializer::doQucikSyncWithPeer(const StatusCb &cb)
{
}

void VolumeInitializer::doStaticMigrationWithPeer(const StatusCb &cb)
{
    // TODO(Neil/James): Please fill this
}

void VolumeInitializer::replayBufferedIo(const StatusCb &cb)
{
    bufferReplay_->startReplay();
}

void VolumeInitializer::abort()
{
    // TODO(Rao):
}

void VolumeInitializer::complete(const Error &e, const std::string &context)
{
    // TODO(Rao):
}

}  // namespace fds
