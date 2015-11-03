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

namespace fds {

using util::TimeStamp;
/**
 * An instance of this class will hold the information of all
 * object sets(bloom filters currently) sent by the DMs in the
 * fds domain. The database used to store the tables is sqliteDB
 * which is a wrapper over sqlite3 db. Each row of the db will
 * look like:
 * [< SM Token>, <Volume Id>, <DM UUID>, <TimeStamp>, <Filepath>]
 * Filepath: Absolute path to the object set file.
 * TimeStamp: Time when the Object Set Message for Object Set is
 *            received from the DM.
 */
class LiveObjectsDB {
    private:
        std::unique_ptr<SqliteDB> db = nullptr;

    public:
        typedef std::unique_ptr<LiveObjectsDB> unique_ptr;
        explicit LiveObjectsDB(const std::string& dbFilePath);
        Error createLiveObjectsTblAndIdx();
        Error addObjectSet(const fds_token_id &smToken,
                           const fds_volid_t &volId,
                           const fds_uint64_t &dmUUID,
                           const TimeStamp &timeStamp,
                           const std::string &objectSetFilePath);
        Error removeObjectSet(const fds_token_id &smToken,
                              const fds_volid_t &volId);
        Error findObjectSetsPerToken(const fds_token_id &smToken,
                                     std::set<std::string> &objSetFilenames);
        Error findAssociatedVols(const fds_token_id &smToken,
                                 std::set<fds_volid_t> &volumes);
        Error findMinTimeStamp(const fds_token_id &smToken,
                               TimeStamp &ts);
        void dropDB();
        ~LiveObjectsDB() { }
};

} // end namespace fds

#endif // SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_LIVEOBJECTSDB_H_
