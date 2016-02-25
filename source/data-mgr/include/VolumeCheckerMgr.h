/*
 * Copyright 2016 Formation Data Systems, Inc.
 */

#ifndef SOURCE_DATA_MGR_INCLUDE_VOLUMECHECKERMGR_H_
#define SOURCE_DATA_MGR_INCLUDE_VOLUMECHECKERMGR_H_

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

private:
    // Local reference of dm
    DataMgr &dataMgr;
};


} // namespace fds

#endif // SOURCE_DATA_MGR_INCLUDE_VOLUMECHECKERMGR_H_
