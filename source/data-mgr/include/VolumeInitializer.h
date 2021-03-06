/* Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_VOLUMEINITIALIZER_H_
#define SOURCE_DATA_MGR_INCLUDE_VOLUMEINITIALIZER_H_

#include <net/SvcRequest.h>
#include <net/SvcRequestPool.h>
#include <util/stringutils.h>
#include <BufferReplay.h>

#define ABORT_CHECK() \
if (completionError_ != ERR_OK) { \
    complete_(completionError_, "Abort check"); \
    return; \
}
#define STUBSTATEMENT(code) code

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
        CONTACT_COORDINATOR,        // Initial contact with coordinator
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
    inline Progress getProgress() const { return progress_; }
    bool syncPreemptionChecks(const fpi::AddToVolumeGroupRespCtrlMsgPtr &responseMsg);

#if 0
    template <class F>
    F makeSynchronized(F &&f) {
        return replica_->makeSynchronized(std::forward<F>(f));
    }
#endif
    std::string logString() const;

 protected:
    /**
     * Sends AddToVolumeGroupCtrlMsg to the coordinator to indicate current replica
     * state.
     * If cb is null response cb isn't set i.e fire/forget message to the coordinator
     */
    void notifyCoordinator_(const EPSvcRequestRespCb &cb = nullptr);
    void copyActiveStateFromPeer_(const std::vector<fpi::SvcUuid> &syncPeers,
                                  const EPSvcRequestRespCb &cb); 
    void startBuffering_();
    void doQucikSyncWithPeer_(const StatusCb &cb);
    void doStaticMigrationWithPeer_(const StatusCb &cb);
    void startReplay_();
    void setProgress_(Progress progress, const std::string& logCtx="");
    bool isSynchronized_() const;
    void complete_(const Error &e, const std::string &context);

    Progress                                progress_;
    T                                       *replica_;
    fpi::SvcUuid                            syncPeer_;
    std::unique_ptr<BufferReplay>           bufferReplay_;
    uint32_t                                bufferReplayIoTimeout_;
    Error                                   completionError_;
    std::string                             logPrefix_;
};
template <class T>
constexpr const char* const ReplicaInitializer<T>::progressStr[];

struct VolumeMeta;
using VolumeInitializer = ReplicaInitializer<VolumeMeta>;
using VolumeInitializerPtr = SHPTR<VolumeInitializer>;

template <class T>
ReplicaInitializer<T>::ReplicaInitializer(CommonModuleProviderIf *provider, T *replica)
: HasModuleProvider(provider),
  progress_(UNINIT),
  replica_(replica)
{
    /* TODO(Rao): Don't hardcode string volid here */
    logPrefix_ = util::strformat("volid: %ld", replica_->getId());
    bufferReplayIoTimeout_ = provider->get_fds_config()->get<fds_uint32_t>(
        "fds.am.svc.timeout.io_message");
}

template <class T>
ReplicaInitializer<T>::~ReplicaInitializer()
{
}

// 02/03/16 Neil - currently only used in: VolumeMeta::startInitializer()
template <class T>
void ReplicaInitializer<T>::run()
{
    setProgress_(CONTACT_COORDINATOR);

    /* NOTE: Notify coordinator can happen outside volume synchronization context */
    notifyCoordinator_(replica_->makeSynchronized([this](EPSvcRequest*,
                                                    const Error &e_,
                                                    StringPtr payload) {
        ABORT_CHECK();
        /* After Coordinator has been notifed volume is loading */
        Error e = e_;
        auto responseMsg = fds::deserializeFdspMsg<fpi::AddToVolumeGroupRespCtrlMsg>(
            e, payload);
        if (e != ERR_OK) {
            complete_(e, "Coordinator rejected at loading state");
            return;
        }

        auto &syncPeers = responseMsg->group.functionalReplicas;
        auto selfUuid = MODULEPROVIDER()->getSvcMgr()->getSelfSvcUuid();
        fds_assert(std::find(syncPeers.begin(), syncPeers.end(), selfUuid) == syncPeers.end());

        /* Based on the first response from coordinator sync can be preempted */
        if (syncPreemptionChecks(responseMsg)) {
            return;
        }

        /* Try and copy active state from peers in a failover manner.  The first successful
         * responder is the peer we will do quick sync/static migration with.
         * NOTE: In case we fail while doing quick sync/static migration we try the
         * initilization sequence from the beginning
         */
        copyActiveStateFromPeer_(syncPeers,
                                 replica_->makeSynchronized([this](EPSvcRequest*,
                                                        const Error &e_,
                                                        StringPtr payload) {
           ABORT_CHECK();
           Error e = e_;
           auto activeState = fds::deserializeFdspMsg<fpi::CtrlNotifyRequestTxStateRspMsg>(
               e, payload);
           if (e != ERR_OK) {
               complete_(e, "Failed to get active state");
               return;
           }
           LOGNORMAL << logString() << ". Recieved active state."
               << " sync peer: " << SvcMgr::mapToSvcUuidAndName(syncPeer_)
               << " latest opid: " << activeState->highest_op_id
               << " # txs: " << activeState->transactions.size();
           Error applyErr = replica_->applyActiveTxState(activeState->highest_op_id,
                                                         activeState->transactions);
           if (applyErr != ERR_OK) {
               complete_(e, "Failed to apply active state");
               return;
           }

           /* Active tx state is copied.  Enable buffering of active io */
           startBuffering_();
           replica_->setState(fpi::ResourceState::Syncing,
                              " - VolumeInitializer:Active state copied");
           /* Notify coordinator we are in syncing state.  From this point
            * we will receive actio io.  Active io will be buffered to disk
            */
           notifyCoordinator_();
           doQucikSyncWithPeer_(replica_->makeSynchronized([this](const Error &e) {
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
                   doStaticMigrationWithPeer_(replica_->makeSynchronized(replayBufferedIoWork));
                   return;
               }
               replayBufferedIoWork(e);
            }));
        }));
    }));
}

template <class T>
bool ReplicaInitializer<T>::
syncPreemptionChecks(const fpi::AddToVolumeGroupRespCtrlMsgPtr &responseMsg)
{
    if (responseMsg->group.lastCommitId == replica_->getSequenceId() && 
        responseMsg->group.lastOpId == VolumeGroupConstants::OPSTARTID) {
        /* Short-circuit migration if we match state with coordinator and
         * no writes have happened on the coordinator
         */
        replica_->setState(fpi::ResourceState::Syncing,
                           "- Sync prevented. No writes since last open. About to become functional");
        notifyCoordinator_();
        complete_(ERR_OK, "- Sync prevented. No writes since last open. Become functional");
        return true;
    } else if (responseMsg->group.functionalReplicas.size() == 0) {
        complete_(ERR_SYNCPEER_UNAVAILABLE, " - no sync peer is available");
        return true;
    }

    return false;
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
    auto bufferfilePrefix = replica_->getBufferfilePrefix();
    auto bufferfilePath = util::strformat("%s%d", bufferfilePrefix.c_str(),
                                          replica_->getVersion());
    /* Remove any existing buffer files */
    auto cmdRet = std::system(util::strformat("rm %s* >/dev/null 2>&1",
                                              bufferfilePrefix.c_str()).c_str());
    /* Create buffer replay instance */
    bufferReplay_.reset(new BufferReplay(bufferfilePath,
                                         512,  /* Replay batch size */
                                         replica_->getId(),
                                         MODULEPROVIDER()->proc_thrpool()));
    auto err = bufferReplay_->init();
    if (!err.ok()) {
        complete_(err, "BufferReplay init() failed");
        return;
    }

    /* Callback to get the progress of replay */
    bufferReplay_->setProgressCb(replica_->makeSynchronized([this](BufferReplay::Progress progress) {
        // TODO(Rao): Current state validity checks
        LOGNOTIFY << "BufferReplay progressCb: " << replica_->logString()
            << bufferReplay_->logString();
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
        /* We replay ops as if client is sending the ops over service layer */
        auto requestMgr = MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr();
        auto selfUuid = MODULEPROVIDER()->getSvcMgr()->getSelfSvcUuid();
        for (const auto &op : ops) {
            if (op.opId <= replica_->getOpId()) {
                /**
                 * Static migration will transfer the activeTx's to pair with whatever
                 * blobs and blobs desc that has already been committed in the levelDB
                 * Static migration will apply those activeTx's, and volumeMeta will
                 * have its opId updated to whatever snapshot that the source had at the time.
                 * The on disk buffer started buffering before the static migration source
                 * snapshot and continued throughout the migration. We need to skip
                 * entries that are migrated from the source and applied as part of migration
                 * so we don't duplicate replay and cause error.
                 */
                bufferReplay_->notifyOpsReplayed();
                continue;
            }
            auto req = requestMgr->newEPSvcRequest(selfUuid);
            req->setTimeoutMs(bufferReplayIoTimeout_);
            req->setTaskExecutorId(replica_->getId());
            req->setPayloadBuf(static_cast<fpi::FDSPMsgTypeId>(op.type), op.payload);
            req->onResponseCb([this](EPSvcRequest*,
                                     const Error &e,
                                     StringPtr) {
                if (e != ERR_OK) {
                    if (!isVolumeGroupError(e)) {
                        LOGNORMAL << replica_->logString() << bufferReplay_->logString()
                            << " buffer replay encountered "
                            << e << " - ignoring the error";
                    } else {
                        LOGWARN << replica_->logString() << bufferReplay_->logString()
                            << " buffer replay encountered volume group error: "
                            << e << " - aborting buffer replay";
                        /* calling abort mutiple times should be ok */
                        bufferReplay_->abort();
                    }
                }
                /* Op accounting.  NOTE: Op accounting needs to be done even after abort */
                bufferReplay_->notifyOpsReplayed();
            });
            /* invokeDirect() because we want to ensure IO is enqueued to qos on this thread */
            req->invokeDirect();
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
    req->setTaskExecutorId(replica_->getId());
    if (cb) {
        req->onResponseCb(cb);
    }
    req->invoke();
}

template <class T>
void ReplicaInitializer<T>::copyActiveStateFromPeer_(const std::vector<fpi::SvcUuid> &syncPeers,
                                                     const EPSvcRequestRespCb &cb)
{
    fds_assert(isSynchronized_());
    setProgress_(COPY_ACTIVE_STATE);

    auto msg = fpi::CtrlNotifyRequestTxStateMsgPtr(new fpi::CtrlNotifyRequestTxStateMsg);
    msg->volume_id = replica_->getId();
    msg->version = replica_->getVersion();
    auto requestMgr = MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr();
    auto req = requestMgr->newFailoverSvcRequest(syncPeers);
    req->setPayload(FDSP_MSG_TYPEID(fpi::CtrlNotifyRequestTxStateMsg), msg);
    req->onResponseCb([this, cb](FailoverSvcRequest* req,
                                 const Error &e,
                                 StringPtr payload) {
        syncPeer_ = req->getLastRespondedSvcUuid();
        cb(nullptr, e, payload);
    });
    req->invoke();
}

template <class T>
void ReplicaInitializer<T>::doQucikSyncWithPeer_(const StatusCb &cb)
{
    fds_assert(isSynchronized_());
    // TODO(Rao): 
    setProgress_(QUICK_SYNCING);

    MODULEPROVIDER()->proc_thrpool()->schedule(cb, ERR_QUICKSYNC_NOT_POSSIBLE);
}

template <class T>
void ReplicaInitializer<T>::doStaticMigrationWithPeer_(const StatusCb &cb)
{
    fds_assert(isSynchronized_());
    setProgress_(STATIC_MIGRATION);

    // Helps trace: startMigration should call volumeMeta's startMigration
    if (replica_->startMigration(syncPeer_, replica_->getId(), cb) != ERR_OK) {
        abort();
    }
}

template <class T>
void ReplicaInitializer<T>::startReplay_()
{
    fds_assert(isSynchronized_());
    setProgress_(REPLAY_ACTIVEIO, bufferReplay_->logString());
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
        // startMigration's cleanup should take care of the cleanups
    } else if (progress_ == REPLAY_ACTIVEIO) {
        bufferReplay_->abort();
    }
}

template <class T>
std::string ReplicaInitializer<T>::logString() const
{
    std::stringstream ss;
    ss << logPrefix_
        << " progress: " << progressStr[static_cast<int>(progress_)];
    return ss.str();
}


template <class T>
void ReplicaInitializer<T>::setProgress_(Progress progress, const std::string &logCtx)
{
    progress_ = progress;
    LOGNORMAL << logString() << replica_->logString() << logCtx;
}

template <class T>
bool ReplicaInitializer<T>::isSynchronized_() const
{
    return replica_->isSynchronized();
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

    completionError_ = e;
    setProgress_(COMPLETE);

    if (completionError_ == ERR_OK) {
        replica_->setState(fpi::ResourceState::Active, context);
    } else {
        replica_->setState(fpi::ResourceState::Offline, context);
    }
    notifyCoordinator_();

    /* This must be the last thing we do on ReplicaInitializer object.  After this call, 
     * ReplicaInitializer will be deleted
     */
    replica_->notifyInitializerComplete(completionError_);
}

}  // namespace fds

#endif          // SOURCE_DATA_MGR_INCLUDE_VOLUMEINITIALIZER_H_
