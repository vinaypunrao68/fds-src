/* Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_VOLUMEINITIALIZER_H_
#define SOURCE_DATA_MGR_INCLUDE_VOLUMEINITIALIZER_H_

#include <net/SvcRequest.h>
#include <net/SvcRequestPool.h>
#include <BufferReplay.h>

#define ABORT_CHECK() \
if (completionError_ != ERR_OK) { \
    complete_(completionError_, "Abort check"); \
    return; \
}

namespace fds {

/**
* @brief Run protocol for initializing replica.  Protocol is as follows
* 1. Notify coordinator replica is in loading state.  This allows coordinator to buffer
*   io while we fetch active state from peer.  Coordinator responds back with information
*   about active replicas we can sync from
* 2. Copy active state (transactions etc.) from sync peer and apply locally.
* 3. Go into sync state.  Notify coordinator we are ready for active io.  From this point
*    coordinator will send active io.  Active Io will be buffered to disk.
* 4. Try and attempt quick sync.  After quick sync, apply buffered io and notify coordinator
*   we are active.
* 5. If quick sync is not possible, do static migration followed by applying buffered io and
*    notifying coordinator we are active.
*
* @tparam T
*/
template <class T>
struct ReplicaInitializer : HasModuleProvider,
                            boost::enable_shared_from_this<ReplicaInitializer<T>>
{
    enum Progress {
        UNINIT,
        // replica state = loading
        CONTACT_COORDINATOR,
        COPY_ACTIVE_STATE,
        // replica state = syncing
        ENABLE_BUFFERING,
        QUICK_SYNCING,
        STATIC_MIGRATION,
        REPLAY_ACTIVEIO,
        // replica state = active|offline based on completionError_
        COMPLETE
    };
    static const constexpr char* const progressStr[] =  {
        "UNINIT",
        "CONTACT_COORDINATOR",
        "COPY_ACTIVE_STATE",
        "ENABLE_BUFFERING",
        "QUICK_SYNCING",
        "STATIC_MIGRATION",
        "REPLAY_ACTIVEIO",
        "COMPLETE"
    };

    ReplicaInitializer(CommonModuleProviderIf *provider, T *replica);
    virtual ~ReplicaInitializer();
    void run();
    void abort();
    Error tryAndBufferIo(const BufferReplay::Op &op);

    template <class F>
    F makeSynchronized(F &&f) {
        return replica_->makeSynchronized(std::forward<F>(f));
    }
    std::string logString() const;

 protected:
    void notifyCoordinator_(const EPSvcRequestRespCb &cb = nullptr);
    void copyActiveStateFromPeer_(const StatusCb &cb); 
    void startBuffering_();
    void doQucikSyncWithPeer_(const StatusCb &cb);
    void doStaticMigrationWithPeer_(const StatusCb &cb);
    void startReplay_();
    void setProgress_(Progress progress);
    bool isSynchronized_();
    void complete_(const Error &e, const std::string &context);

    Progress                                progress_;
    T                                       *replica_;
    fpi::SvcUuid                            syncPeer_;
    std::unique_ptr<BufferReplay>           bufferReplay_;
    Error                                   completionError_;
};

struct VolumeMeta;
using VolumeInitializer = ReplicaInitializer<VolumeMeta>;
using VolumeInitializerPtr = SHPTR<VolumeInitializer>;

template <class T>
ReplicaInitializer<T>::ReplicaInitializer(CommonModuleProviderIf *provider, T *replica)
: HasModuleProvider(provider),
  progress_(UNINIT),
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
    setProgress_(CONTACT_COORDINATOR);
    notifyCoordinator_(makeSynchronized([this](EPSvcRequest*,
                                                    const Error &e,
                                                    StringPtr) {
        ABORT_CHECK();
        /* After Coordinator has been notifed volume is loading */
        if (e != ERR_OK) {
            complete_(e, "Coordinator rejected at loading state");
            return;
        }
        copyActiveStateFromPeer_(makeSynchronized([this](const Error &e) {
           ABORT_CHECK();
           /* After active tx state is copied from the peer */
           if (e != ERR_OK) {
               complete_(e, "Failed to copy active state");
               return;
           }
           startBuffering_();
           replica_->setState(fpi::ResourceState::Syncing);
           /* Notify coordinator we are in syncing state.  From this point
            * we will receive actio io.  Active io will be buffered to disk
            */
           notifyCoordinator_();
           doQucikSyncWithPeer_(makeSynchronized([this](const Error &e) {
               ABORT_CHECK();
               auto replayBufferedIoWork = [this](const Error &e) {
                   ABORT_CHECK();
                   /* After quick sync/static migiration with the peer */
                   if (e != ERR_OK) {
                       complete_(e, "Failed to do sync");
                       return;
                   }
                   /* Completing initialization sequence is done based on result from
                    * completion of replay.  see startBuffering_()
                    */
                   startReplay_();
               };
               if (e == ERR_QUICKSYNC_NOT_POSSIBLE) {
                   doStaticMigrationWithPeer_(makeSynchronized(replayBufferedIoWork));
                   return;
               }
               replayBufferedIoWork(e);
            }));
        }));
    }));
}

template <class T>
Error ReplicaInitializer<T>::tryAndBufferIo(const BufferReplay::Op &op)
{
    fds_assert(replica_->getState() == fpi::Syncing);

    return bufferReplay_->buffer(op);
}

template <class T>
void ReplicaInitializer<T>::startBuffering_()
{
    setProgress_(ENABLE_BUFFERING);
    // TODO(Rao): Set the directory
    bufferReplay_.reset(new BufferReplay("buffered_writes",
                                         512,  /* Replay batch size */
                                         MODULEPROVIDER()->proc_thrpool()));
    /* Callback to get the progress of replay */
    bufferReplay_->setProgressCb(makeSynchronized([this](BufferReplay::Progress progress) {
        // TODO(Rao): Current state validity checks
        LOGNOTIFY << replica_->logString() << " BufferReplay progress: " << progress;
        if (progress == BufferReplay::REPLAY_CAUGHTUP) {
            // TODO(Rao): Set disable buffering on replica
        } else if (progress == BufferReplay::COMPLETE) {
            complete_(ERR_OK, "BufferReplay progress callback");
        } else if (progress == BufferReplay::ABORTED) {
            if (bufferReplay_->getOutstandingReplayOpsCnt() == 0) {
                complete_(ERR_ABORTED, "BufferReplay progress callback");
            }
        }
    }));

    /* Callback to replay a batch of ops.
     * NOTE: This doesn't need to be done on a volume synchronized context
     */
    bufferReplay_->setReplayOpsCb([this](int64_t opCntr, std::list<BufferReplay::Op> &ops) {
        auto requestMgr = MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr();
        auto selfUuid = MODULEPROVIDER()->getSvcMgr()->getSelfSvcUuid();
        for (const auto &op : ops) {
            auto req = requestMgr->newEPSvcRequest(selfUuid);
            req->setPayloadBuf(op.first, op.second);
            req->onResponseCb([this](EPSvcRequest*,
                                     const Error &e,
                                     StringPtr) {
                if (e != ERR_OK) {
                    /* calling abort mutiple times should be ok */
                    bufferReplay_->abort();
                }
                bufferReplay_->notifyOpsReplayed();
            });
            req->invoke();
        }
    });
}

template <class T>
void ReplicaInitializer<T>::notifyCoordinator_(const EPSvcRequestRespCb &cb)
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

template <class T>
void ReplicaInitializer<T>::copyActiveStateFromPeer_(const StatusCb &cb)
{
    fds_assert(isSynchronized_());
    // TODO(Neil): Please fill this
    setProgress_(COPY_ACTIVE_STATE);
}

template <class T>
void ReplicaInitializer<T>::doQucikSyncWithPeer_(const StatusCb &cb)
{
    fds_assert(isSynchronized_());
    // TODO(Rao): 
    setProgress_(QUICK_SYNCING);
}

template <class T>
void ReplicaInitializer<T>::doStaticMigrationWithPeer_(const StatusCb &cb)
{
    fds_assert(isSynchronized_());
    // TODO(Neil/James): Please fill this
    setProgress_(STATIC_MIGRATION);
}

template <class T>
void ReplicaInitializer<T>::startReplay_()
{
    fds_assert(isSynchronized_());
    setProgress_(REPLAY_ACTIVEIO);
    bufferReplay_->startReplay();
}

template <class T>
void ReplicaInitializer<T>::abort()
{
    fds_assert(isSynchronized_());

    if (progress_ == COMPLETE) {
        return;
    }

    fds_assert(completionError_ == ERR_OK);
    completionError_ = ERR_ABORTED;

    /* Issue aborts on any outstanding operations */
    if (progress_ == STATIC_MIGRATION) {
        // TODO(Rao): Abort static migration
    } else if (progress_ == REPLAY_ACTIVEIO) {
        bufferReplay_->abort();
    }
}

template <class T>
std::string ReplicaInitializer<T>::logString() const
{
    std::stringstream ss;
    ss << " Initializer id: " << replica_->getId()
        << " progress: " << progressStr[static_cast<int>(progress_)];
    return ss.str();
}


template <class T>
void ReplicaInitializer<T>::setProgress_(Progress progress)
{
    progress_ = progress;
    LOGNORMAL << logString();
}

template <class T>
bool ReplicaInitializer<T>::isSynchronized_()
{
    return false;
}

template <class T>
void ReplicaInitializer<T>::complete_(const Error &e, const std::string &context)
{
    fds_assert(isSynchronized_());

    /* At this point all operations related to initialization must be complete.
     * We should know whether initialization finished with success or failure
     */
    fds_assert(progress_ != COMPLETE);
    // TODO(Rao): Assert static migration, buffer replay aren't in progress

    if (completionError_ != ERR_OK) {
        completionError_ = e;
    }
    setProgress_(COMPLETE);

    if (completionError_ == ERR_OK) {
        replica_->setState(fpi::ResourceState::Active);
    } else {
        replica_->setState(fpi::ResourceState::Offline);
    }
    notifyCoordinator_();
    // TODO(Rao): Notify replica so that cleanup can be done
}

}  // namespace fds

#endif          // SOURCE_DATA_MGR_INCLUDE_VOLUMEINITIALIZER_H_
