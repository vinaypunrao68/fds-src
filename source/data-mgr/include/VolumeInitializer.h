/* Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_VOLUMEINITIALIZER_H_
#define SOURCE_DATA_MGR_INCLUDE_VOLUMEINITIALIZER_H_

#include <net/SvcRequest.h>
#include <BufferReplay.h>

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
struct ReplicaInitializer : HasModuleProvider {
    enum Progress {
        // state = loading
        CONTACT_COORDINATOR,
        COPY_ACTIVE_STATE,
        // state = syncing
        QUICK_SYNCING,
        STATIC_MIGRATION,
        REPLAY_ACTIVEIO,
        // state = active
        COMPLETE
    };

    ReplicaInitializer(CommonModuleProviderIf *provider, T *replica);
    virtual ~ReplicaInitializer();
    virtual void notifyCoordinator(const EPSvcRequestRespCb &cb) = 0;
    virtual void copyActiveStateFromPeer(const StatusCb &cb) = 0; 
    virtual void doQucikSyncWithPeer(const StatusCb &cb) = 0;
    virtual void doStaticMigrationWithPeer(const StatusCb &cb) = 0;
    virtual void replayBufferedIo(const StatusCb &cb) = 0;
    virtual void abort() = 0;
    virtual void run();
    virtual void complete(const Error &e, const std::string &context) = 0;

    virtual Error tryAndBufferIo(const BufferReplay::Op &op);
 protected:
    void enableWriteOpsBuffering_();

    Progress                                progress_;
    T                                       *replica_;
    fpi::SvcUuid                            syncPeer_;
    std::unique_ptr<BufferReplay>           bufferReplay_;
};

struct VolumeMeta;

/**
* @brief Handles volume specific initialization
*/
struct VolumeInitializer : ReplicaInitializer<VolumeMeta> {
    using ReplicaInitializer<VolumeMeta>::ReplicaInitializer;

    void notifyCoordinator(const EPSvcRequestRespCb &cb) override;
    void copyActiveStateFromPeer(const StatusCb &cb) override; 
    void doQucikSyncWithPeer(const StatusCb &cb) override;
    void doStaticMigrationWithPeer(const StatusCb &cb) override;
    void replayBufferedIo(const StatusCb &cb) override;
    void abort() override;
    void complete(const Error &e, const std::string &context) override;
};
using VolumeInitializerPtr = SHPTR<VolumeInitializer>;

}  // namespace fds

#endif          // SOURCE_DATA_MGR_INCLUDE_VOLUMEINITIALIZER_H_
