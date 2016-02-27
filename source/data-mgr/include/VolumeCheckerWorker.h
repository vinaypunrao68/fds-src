/*
 * Copyright 2016 Formation Data Systems, Inc.
 */

#ifndef SOURCE_DATA_MGR_INCLUDE_VOLUMECHECKERWORKER_H_
#define SOURCE_DATA_MGR_INCLUDE_VOLUMECHECKERWORKER_H_

#include <fds_volume.h>

namespace fds {

// Forward Declaration
class DataMgr;

// Each worker stores information about certain volumes and does the computational
// work that's needed by the VolumeChecker class.
class VolumeCheckerWorker {
public:
    explicit VolumeCheckerWorker(fds_volid_t _volId, DataMgr &_dm);
    ~VolumeCheckerWorker() = default;

    /**
     * Starts the worker in a QoS feedback loop based on System Queue
     * Goal is to quickly schedule something and let go of this current thread
     */
    void scheduleToRun();

    // Mark the volume offline prior to doing work
    void markVolumeOffline();

private:
    std::string     logString();
    fds_volid_t     volId;     // local cached id
    dm::Handler     qosHelper;  // Used for feedback loop into QoS sys queue

};

} // namespace fds

#endif // SOURCE_DATA_MGR_INCLUDE_VOLUMECHECKERWORKER_H_
