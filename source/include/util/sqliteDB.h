/**
 * Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_UTIL_SQLITEDB_H_
#define SOURCE_INCLUDE_UTIL_SQLITEDB_H_

#include <sqlite3.h>
#include <util/Log.h>
#include <fds_error.h>
#include <fds_types.h>

namespace fds {
/**
 * This class creates an instance of sqlite3 db and provides
 * a set of apis to execute queries and access data stored in
 * the tables.
 * TODO(Gurpreet): write some more data retrieval apis like
 *                 getIntValues, getTextValue etc.
 */
class SqliteDB {

    private:
        sqlite3 *db = nullptr;
        std::string dbFile;
    public:
        explicit SqliteDB(const std::string &dbFilePath);
        int execute(const std::string &query);
        int getIntValue(const std::string &query,
                        fds_uint64_t &value);
        int getTextValues(const std::string &query,
                          std::vector<std::string> &value);
        inline void logOnError(const int &errorCode,
                               const std::string &msg) {
            if (errorCode != SQLITE_OK) {
                LOGERROR << msg << " Error code: " << errorCode
                         << " " << sqlite3_errmsg(db);
            }
        }
        int dropDB();
        ~SqliteDB();
};

} // namespace fds

#endif // SOURCE_INCLUDE_UTIL_SQLITEDB_H_


