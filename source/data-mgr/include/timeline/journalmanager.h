/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_DATA_MGR_INCLUDE_TIMELINE_JOURNALMANAGER_H_
#define SOURCE_DATA_MGR_INCLUDE_TIMELINE_JOURNALMANAGER_H_
#include <util/timeutils.h>
#include <string>
namespace fds {
struct DataMgr;
namespace timeline {
struct JournalManager {
    JournalManager(fds::DataMgr* dm);
    virtual ~JournalManager();
    void stopMonitoring();
    Error replayTransactions(Catalog& destCat,
                             const std::vector<std::string> &files,
                             util::TimeStamp fromTime,
                             util::TimeStamp toTime);

    Error replayTransactions(fds_volid_t srcVolId,
                             fds_volid_t destVolId,
                             util::TimeStamp fromTime,
                             util::TimeStamp toTime);

    Error getJournalStartTime(const std::string &logfile,
                              util::TimeStamp& startTime);

  protected:
    SHPTR<std::thread> logMonitor;
    void monitorLogs();
    void removeExpiredJournals();
  private:
    fds::DataMgr* dm;
    bool fStopLogMonitoring;
};
}  // namespace timeline
}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_TIMELINE_JOURNALMANAGER_H_
