/**
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <sqlite3.h>

namespace fds {

class SqliteDB {

    private:
        sqlite3 *db = nullptr;

    public:
        SqliteDB(const std::string& dbFilePath);
        int execute(const std::string& stmt);
        int getValue(const std::string& stmt,
                     fds_uint64_t& value);
        inline void checkAndLog(const int& errorCode,
                                const std::string& msg) {
            if (errorCode != SQLITE_OK) {
                LOGERROR << msg << " Error code: " << errorCode
                         << " " << sqlite3_errmsg(db);
            }
        }
        ~SqliteDB();
};
