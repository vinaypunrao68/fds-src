/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_DATA_MGR_INCLUDE_TIMELINE_TIMELINEMANAGER_H_
#define SOURCE_DATA_MGR_INCLUDE_TIMELINE_TIMELINEMANAGER_H_

#include <map>

#include <timeline/timelinedb.h>
#include <timeline/journalmanager.h>
#include <util/bloomfilter.h>
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
    SHPTR<TimelineDB> getDB();

    bool isObjectInSnapshot(const ObjectID& objId, fds_volid_t volId);
  private:
    fds::DataMgr* dm;
    SHPTR<TimelineDB> timelineDB;
    SHPTR<JournalManager> journalMgr;

    std::string getSnapshotBloomFile(fds_volid_t snapshotId);
    Error markObjectsInSnapshot(fds_volid_t volId, fds_volid_t snapshotId);

    // map of volid -> [map of snapshotid->bloom filter]
    std::map<fds_volid_t , std::map<fds_volid_t,SHPTR<util::BloomFilter> > > blooms;
};
}  // namespace timeline
}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_TIMELINE_TIMELINEMANAGER_H_
