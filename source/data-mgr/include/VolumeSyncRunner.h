/* Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_VOLUMESYNCRUNNER_H_
#define SOURCE_DATA_MGR_INCLUDE_VOLUMESYNCRUNNER_H_

#include <net/SvcRequest.h>

namespace fds {

/**
* @brief Run sync protocol.  Protocol is as follows
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
struct SyncProtocolRunner : HasModuleProvider {
    SyncProtocolRunner(CommonModuleProviderIf *provider, T *replica)
    : HasModuleProvider(provider),
    replica_(replica)
    {}
    virtual ~SyncProtocolRunner() {}
    virtual void notifyCoordinator(const EPSvcRequestRespCb &cb) = 0;
    virtual void copyActiveStateFromPeer(const StatusCb &cb) = 0; 
    virtual void doQucikSyncWithPeer(const StatusCb &cb) = 0;
    virtual void doStaticMigrationWithPeer(const StatusCb &cb) = 0;
    virtual void replayBufferedIo(const StatusCb &cb) = 0;
    virtual void abort() = 0;
    virtual void run();
    virtual void complete(const Error &e, const std::string &context) = 0;
 protected:
    T *replica_;
};


}  // namespace fds

#endif          // SOURCE_DATA_MGR_INCLUDE_VOLUMESYNCRUNNER_H_
