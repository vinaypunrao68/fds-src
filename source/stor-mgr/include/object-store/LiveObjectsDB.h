/**
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <util/sqliteDB.h>
#include <util/timeutils.h>
#include <util/stringutils.h>

namespace fds {

using util::TimeStamp;

class LiveObjectsDB {

    private:
        std::unique_ptr<SqliteDB> db = nullptr;

    public:
        LiveObjectDB(const std::string& dbFilePath);
        Error createLiveObjectsTblAndIdx();
        Error addObjectSet(fds_token_id& smToken,
                           fds_vol_id& volId,
                           TimeStamp& timeStamp,
                           std::string objectSetFilePath);
        Error removeObjectSet(fds_token_id& smToken,
                              fds_vol_id& volId);
        Error findObjectSetsPerToken(fds_toke_id& smToken,
                                     std::vector<std::string> &objSetFilenames);
        ~LiveObjectDB() { }
};

} // end namespace fds
