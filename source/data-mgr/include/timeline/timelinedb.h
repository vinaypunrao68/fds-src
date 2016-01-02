/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_TIMELINE_TIMELINEDB_H_
#define SOURCE_DATA_MGR_INCLUDE_TIMELINE_TIMELINEDB_H_
#include <string>
#include <vector>
#include <util/Log.h>
#include <util/timeutils.h>
#include <sqlite3.h>
#include <fds_error.h>
#include <ostream>
namespace fds {
using util::TimeStamp;

namespace timeline {
struct JournalFileInfo {
    TimeStamp startTime;
    std::string journalFile;
};
/**
 * NOTE: All TimeStamps in micros
 */
class TimelineDB : public HasLogger {
  public:
    TimelineDB();
    Error open(const FdsRootDir *root);
    Error addJournalFile(fds_volid_t volId,
                         TimeStamp startTime, const std::string& journalFile);
    Error removeJournalFile(fds_volid_t volId, const std::string& journalFile);
    Error removeOldJournalFiles(fds_volid_t volId, TimeStamp until,
                                std::vector<JournalFileInfo>& vecJournalFiles);
    Error getJournalFiles(fds_volid_t volId, TimeStamp fromTime, TimeStamp toTime,
                   std::vector<JournalFileInfo>& vecJournalFiles);
    Error addSnapshot(fds_volid_t volId, fds_volid_t snapshotId, TimeStamp createTime);
    Error removeSnapshot(fds_volid_t snapshotId);
    Error getLatestSnapshotAt(fds_volid_t volId,
                              TimeStamp uptoTime,
                              fds_volid_t& snapshotId);
    Error getSnapshotTime(fds_volid_t volId, fds_volid_t snapshotId, TimeStamp& createTime);
    Error removeVolume(fds_volid_t volId);
    Error close();
    ~TimelineDB();

  protected:
    Error getInt(const std::string& sql, fds_uint64_t& data);

  private:
    sqlite3 *db = NULL;
};
}  // namespace timeline
}  // namespace fds
std::ostream& operator<<(std::ostream& os, const fds::timeline::JournalFileInfo& fileinfo);
#endif  // SOURCE_DATA_MGR_INCLUDE_TIMELINE_TIMELINEDB_H_
