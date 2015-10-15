/**
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <sqliteDB.h>

namespace fds {

SqliteDB::SqliteDB(const std::string &dbFilePath) {
    int errorCode = sqlite3_open(dbFilePath.c_str(), &db);
    checkAndLog(errorCode, "Unable to open db.");
}

SqliteDB::~SqliteDB() {
    if (db) {
        int errorCode = sqlite3_close(db);
        checkAndLog(errorCode, "Unable to close db");
        if (errorCode != SQLITE_BUSY) {
            db = nullptr;
        }
    }
}

int SqliteDB::execute(const std::string& query) {
    if (!db) {
        return -1;
    }

    char *errorMsg = nullptr;
    int errorCode = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errorMsg);
    checkAndLog("Statement execution failed.");
    return errorCode;
}

int SqliteDB::getValue(const std::string& query, fds_uint64_t& value) {
    if (!db) {
        return -1;
    }

    int  errorCode;
    Error err(ERR_OK);
    sqlite3_stmt * stmt;
    const char * pzTail;

    errorCode = sqlite3_prepare(db, query.c_str(), -1, &stmt, &pzTail);
    checkAndLog("Unable to prepare statement");
    do {
        errorCode = sqlite3_step(stmt);
        switch (errorCode) {
            case SQLITE_ROW:
                data = sqlite3_column_int64(stmt, 0);
                break;
            case SQLITE_DONE:
                break;
            default:
                LOGERROR << "Unknown error code: " << errorCode << " " << sqlite3_errmsg(db);
        }
    } while (errorCode == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return errorCode;
}

}
