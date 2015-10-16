/**
 * Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_LIVEOBJECTSDB_H_
#define SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_LIVEOBJECTSDB_H_

#include <string>
#include <vector>

#include <util/sqliteDB/sqliteDB.h>
#include <util/timeutils.h>
#include <util/stringutils.h>

#include <fds_error.h>

namespace fds {

using util::TimeStamp;

class LiveObjectsDB {

    private:
        std::unique_ptr<SqliteDB> db = nullptr;

    public:
        explicit LiveObjectsDB(const std::string& dbFilePath);
        Error createLiveObjectsTblAndIdx();
        Error addObjectSet(const fds_token_id &smToken,
                           const fds_volid_t &volId,
                           const TimeStamp &timeStamp,
                           const std::string &objectSetFilePath);
        Error removeObjectSet(const fds_token_id &smToken,
                              const fds_volid_t &volId);
        Error findObjectSetsPerToken(const fds_token_id &smToken,
                                     std::vector<std::string> &objSetFilenames);
        ~LiveObjectsDB() { }
};

} // end namespace fds

#endif // SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_LIVEOBJECTSDB_H_
