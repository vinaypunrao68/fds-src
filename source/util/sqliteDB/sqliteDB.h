/**
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <sqlite3.h>

namespace fds {

class SqliteDB {

    private:
        sqlite3 *db = nullptr;

    public:
        SqliteDB(const std::string &dbFilePath);
        int execute(const std::string &query);
        int getIntValue(const std::string &query,
                        fds_uint64_t &value);
        int getIntValues(const std::string &query,
                         std::vector<fds_uint64_t> &value);
        int getTextValue(const std::string &query,
                         std::string &value);
        int getTextValues(const std::string &query,
                          std::vector<std::string> &value);
        inline void checkAndLog(const int& errorCode,
                                const std::string& msg) {
            if (errorCode != SQLITE_OK) {
                LOGERROR << msg << " Error code: " << errorCode
                         << " " << sqlite3_errmsg(db);
            }
        }
        ~SqliteDB();
};

}
