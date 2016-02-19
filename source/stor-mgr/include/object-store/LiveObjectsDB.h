/**
 * Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_LIVEOBJECTSDB_H_
#define SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_LIVEOBJECTSDB_H_

#include <string>
#include <vector>

#include <util/sqliteDB.h>
#include <util/timeutils.h>
#include <util/stringutils.h>
#include <fds_error.h>
#include <concurrency/RwLock.h>

namespace fds {

using util::TimeStamp;
/**
 * An instance of this class will hold the information of all
 * object sets(bloom filters currently) sent by the DMs in the
 * fds domain. The database used to store the tables is sqliteDB
 * which is a wrapper over sqlite3 db. Each row of the db will
 * look like:
 * [< SM Token>, <Volume Id>, <TimeStamp>, <Filepath>]
 * Filepath: Absolute path to the object set file.
 * TimeStamp: Time when the Object Set Message for Object Set is
 *            received from the DM.
 */
class LiveObjectsDB {
  private:
    std::unique_ptr<SqliteDB> db = nullptr;
    fds_rwlock lock;
    
  public:
    typedef std::unique_ptr<LiveObjectsDB> unique_ptr;
    explicit LiveObjectsDB(const std::string& dbFilePath);
    Error createLiveObjectsTblAndIdx();
    Error addObjectSet(const fds_token_id &smToken,
                       const fds_volid_t &volId,
                       const TimeStamp &timeStamp,
                       const std::string &objectSetFilePath);

    Error cleansertObjectSet(const fds_token_id &smToken,
                             const fds_volid_t &volId,
                             const TimeStamp &timeStamp,
                             const std::string &objectSetFilePath);
    Error removeObjectSet(const fds_token_id &smToken,
                          const fds_volid_t &volId);
    Error removeObjectSet(const fds_token_id &smToken);
    Error removeObjectSet(const fds_volid_t &volId);
    Error findObjectSetsPerToken(const fds_token_id &smToken,
                                 std::set<std::string> &objSetFilenames);
    Error findAssociatedVols(const fds_token_id &smToken,
                             std::set<fds_volid_t> &volumes);
    Error findAllVols(std::set<fds_volid_t> &volumes);
    Error findMinTimeStamp(const fds_token_id &smToken,
                           TimeStamp &ts);
    bool haveAllObjectSets(const fds_token_id &smToken,
                           const std::set<fds_volid_t> &volumes,
                           TimeStamp ts=0);

    Error setTokenStartTime(const fds_token_id &smToken, fds_uint16_t diskid,TimeStamp &ts);
    TimeStamp getTokenStartTime(const fds_token_id &smToken, fds_uint16_t diskid);
    bool hasNewObjectSets(const fds_token_id &smToken, fds_uint16_t diskid);
    void dropDB();
    ~LiveObjectsDB() { }
};

} // end namespace fds

#endif // SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_LIVEOBJECTSDB_H_
