/**
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <util/sqliteDB.h>

namespace fds {

SqliteDB::SqliteDB(const std::string &dbFilePath):
                   dbFile(dbFilePath) {
    int errorCode = sqlite3_open(dbFilePath.c_str(), &db);
    logOnError(errorCode, "Unable to open db.");
}

SqliteDB::~SqliteDB() {
    if (db) {
        int errorCode = sqlite3_close(db);
        logOnError(errorCode, "Unable to close db");
        if (errorCode != SQLITE_BUSY) {
            db = nullptr;
        }
    }
}

int SqliteDB::dropDB() {
    fds_scoped_lock lock(mtx);
    int errorCode = 0;
    if (db) {
        errorCode = sqlite3_close(db);
        logOnError(errorCode, "Unable to close db");
        if (errorCode != SQLITE_BUSY) {
            db = nullptr;
            if (unlink(dbFile.c_str()) < 0) {
                LOGERROR << "Can't unlink file " << dbFile;
            }
        }
    }
    return errorCode;
}

int SqliteDB::execute(const std::string &query) {
    fds_scoped_lock lock(mtx);
    if (!db) {  return -1;  }

    int errorCode = sqlite3_exec(db, query.c_str(), nullptr, nullptr, nullptr);
    logOnError(errorCode, "Statement execution failed.");
    return errorCode;
}

bool SqliteDB::getIntValue(const std::string &query, fds_uint64_t &value) {
    fds_scoped_lock lock(mtx);
    if (!db) {  return -1;  }

    int  errorCode;
    sqlite3_stmt *stmt;

    errorCode = sqlite3_prepare(db, query.c_str(), -1, &stmt, nullptr);
    logOnError(errorCode, "Unable to prepare statement");
    do {
        errorCode = sqlite3_step(stmt);
        switch (errorCode) {
            case SQLITE_ROW:
                value = sqlite3_column_int64(stmt, 0);
                break;
            case SQLITE_DONE:
                break;
            default:
                LOGERROR << "Unknown error code: " << errorCode << " " << sqlite3_errmsg(db);
        }
    } while (errorCode == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return (errorCode == SQLITE_DONE);
}

bool SqliteDB::getIntValues(const std::string &query, std::set<fds_uint64_t> &valueSet) {
    fds_scoped_lock lock(mtx);
    if (!db) {  return -1;  }

    int  errorCode;
    sqlite3_stmt *stmt;

    errorCode = sqlite3_prepare(db, query.c_str(), -1, &stmt, nullptr);
    logOnError(errorCode, "Unable to prepare statement");
    do {
        errorCode = sqlite3_step(stmt);
        switch (errorCode) {
            case SQLITE_ROW:
                valueSet.insert(sqlite3_column_int64(stmt, 0));
                break;
            case SQLITE_DONE:
                break;
            default:
                LOGERROR << "Unknown error code: " << errorCode << " " << sqlite3_errmsg(db);
        }
    } while (errorCode == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return (errorCode == SQLITE_DONE);
}

bool SqliteDB::getTextValue(const std::string &query, std::string &value) {
    fds_scoped_lock lock(mtx);
    if (!db) {  return -1;  }

    int  errorCode;
    sqlite3_stmt * stmt;

    errorCode = sqlite3_prepare(db, query.c_str(), -1, &stmt, nullptr);
    logOnError(errorCode, "Unable to prepare statement");
    do {
        errorCode = sqlite3_step(stmt);
        switch (errorCode) {
            case SQLITE_ROW:
                value.assign(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)));
                break;
            case SQLITE_DONE:
                break;
            default:
                LOGERROR << "Unknown error code: " << errorCode << " " << sqlite3_errmsg(db);
        }
    } while (errorCode == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return (errorCode == SQLITE_DONE);
}

bool SqliteDB::getTextValues(const std::string &query, std::set<std::string> &valueSet) {
    fds_scoped_lock lock(mtx);
    if (!db) {  return -1;  }

    int  errorCode;
    sqlite3_stmt * stmt;

    errorCode = sqlite3_prepare(db, query.c_str(), -1, &stmt, nullptr);
    logOnError(errorCode, "Unable to prepare statement");
    do {
        errorCode = sqlite3_step(stmt);
        std::string data;
        switch (errorCode) {
            case SQLITE_ROW:
                data.assign(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)));
                valueSet.insert(data);
                break;
            case SQLITE_DONE:
                break;
            default:
                LOGERROR << "Unknown error code: " << errorCode << " " << sqlite3_errmsg(db);
        }
    } while (errorCode == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return (errorCode == SQLITE_DONE);
}

} // end namespace fds
