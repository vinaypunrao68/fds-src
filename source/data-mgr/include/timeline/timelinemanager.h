/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_DATA_MGR_INCLUDE_TIMELINE_TIMELINEMANAGER_H_
#define SOURCE_DATA_MGR_INCLUDE_TIMELINE_TIMELINEMANAGER_H_

#include <map>

#include <timeline/timelinedb.h>
#include <timeline/journalmanager.h>
namespace fds {
struct DataMgr;
namespace timeline {
struct TimelineManager {
    TimelineManager(fds::DataMgr* dm);

    Error deleteSnapshot(fds_volid_t volid, fds_volid_t snapshotid = invalid_vol_id);
    Error loadSnapshot(fds_volid_t volid, fds_volid_t snapshotid = invalid_vol_id);
    Error unloadSnapshot(fds_volid_t volid, fds_volid_t snapshotid);
    Error createSnapshot(VolumeDesc *vdesc);
    Error createClone(VolumeDesc *vdesc);
    Error removeVolume(fds_volid_t volid);
    Error getSnapshotsForVolume(fds_volid_t volId, std::vector<fds_volid_t>& vecVolIds);
    SHPTR<TimelineDB> getDB();
    float_t getUsedCapacityAsPct();

  private:
    fds::DataMgr* dm;
    SHPTR<TimelineDB> timelineDB;
    SHPTR<JournalManager> journalMgr;

};
}  // namespace timeline
}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_TIMELINE_TIMELINEMANAGER_H_
