/**
 * Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_UTIL_SQLITEDB_H_
#define SOURCE_INCLUDE_UTIL_SQLITEDB_H_

#include <sqlite3.h>
#include <util/Log.h>
#include <fds_error.h>
#include <fds_types.h>
#include <concurrency/Mutex.h>

namespace fds {
/**
 * This class creates an instance of sqlite3 db and provides
 * a set of apis to execute queries and access data stored in
 * the tables.
 */
class SqliteDB {

    private:
        sqlite3 *db = nullptr;
        std::string dbFile;
        fds_mutex mtx;
    public:
        explicit SqliteDB(const std::string &dbFilePath);
        int execute(const std::string &query);
        bool getIntValue(const std::string &query,
                        fds_uint64_t &value);
        bool getIntValues(const std::string &query,
                          std::set<fds_uint64_t> &valueSet);
        bool getTextValue(const std::string &query,
                          std::string &value);
        bool getTextValues(const std::string &query,
                           std::set<std::string> &valueSet);
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


