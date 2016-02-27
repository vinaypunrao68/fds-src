/*
 * Copyright 2016 Formation Data Systems, Inc.
 */

#ifndef SOURCE_DATA_MGR_INCLUDE_VOLUMECHECKERMGR_H_
#define SOURCE_DATA_MGR_INCLUDE_VOLUMECHECKERMGR_H_
#include <VolumeCheckerWorker.h>

namespace fds {

// Forward Declarations
class DataMgr;

/**
 * This class takes care of handling the messages coming from the volume
 * checker, as well as the QoS loopback needed to perform data checking.
 */
class VolumeCheckerMgr {
public:
    typedef std::unique_ptr<VolumeCheckerMgr> unique_ptr;
    typedef std::shared_ptr<VolumeCheckerMgr> shared_ptr;

    explicit VolumeCheckerMgr(DataMgr &_dataMgr);
    ~VolumeCheckerMgr() = default;

    /**
     * Registers the volume to be checked
     */
    Error registerRequest(DmRequest *req);

    /**
     * Unit test functions only
     */
    size_t testGetvcWorkerMapSize();

private:
    // Local reference of dm
    DataMgr &dataMgr;

    // The node that the volumeChecker command was issued
    fpi::SvcUuid checkerUuid;

    using vcWorkerPtr = std::unique_ptr<VolumeCheckerWorker>;
    using workerMapType = std::unordered_map<fds_volid_t, vcWorkerPtr>;
    workerMapType vcWorkerMap;


};


} // namespace fds

#endif // SOURCE_DATA_MGR_INCLUDE_VOLUMECHECKERMGR_H_
