/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <vector>
#include <DataMgr.h>
#include <timeline/timelinedb.h>
#include <util/stringutils.h>

std::ostream& operator<<(std::ostream& os, const fds::timeline::JournalFileInfo& fileinfo) {
    os << "[start:" << fileinfo.startTime << " file:" << fileinfo.journalFile << "]";
    return os;
}

#define DECLARE_DB_VARS(...)                    \
    char *zErrMsg = 0;                          \
    int  rc;                                    \
    std::string sql;

#define CHECK_SQL_CODE(msg)                                      \
    if (rc != SQLITE_OK) {                                       \
        LOGERROR << "rc:" << rc <<" sql: " << sql;               \
        LOGERROR << msg << ":" << zErrMsg;                       \
        sqlite3_free(zErrMsg);                                   \
        return ERR_INVALID;                                      \
    }

#define CHECK_DB_CODE(msg)                                              \
    if (rc != SQLITE_OK) {                                              \
        LOGERROR << "rc:" << rc << " " << msg << sqlite3_errmsg(db);    \
        return ERR_INVALID;                                             \
    }

namespace fds { namespace timeline {

TimelineDB::TimelineDB() {
}

Error TimelineDB::open(const FdsRootDir *root) {
    const std::string dmDir = root->dir_sys_repo_dm();
    root->fds_mkdir(dmDir.c_str());
    std::string dbFile = util::strformat("%s/timeline.db", dmDir.c_str());
    DECLARE_DB_VARS();

    // open the db
    rc = sqlite3_open(dbFile.c_str(), &db);
    CHECK_DB_CODE("unable to open db");

    /**
     * Create the Journal Table
     * The Journal Table has a list of journal files and its starttime
     * This is for the commit log retention
     */

    sql = "create table if not exists journaltbl"
            " (volid integer , starttime integer not null, filename text not null)";

    rc = sqlite3_exec(db, sql.c_str(), NULL, NULL,  &zErrMsg);
    CHECK_SQL_CODE("unable to create journal table");

    sql = "create index if not exists volid_starttime on journaltbl (volid, starttime)";
    rc = sqlite3_exec(db, sql.c_str(), NULL, NULL,  &zErrMsg);
    CHECK_SQL_CODE("unable to create index on journal table");

    /**
     * Create the Snapshot Table
     * Contains the list of snapshots and the time of creation
     */
    sql = "create table if not exists snapshottbl"
            " (volid integer, createtime integer, snapshotid integer)";
    rc = sqlite3_exec(db, sql.c_str(), NULL, NULL,  &zErrMsg);
    CHECK_SQL_CODE("unable to create snaphot table");

    sql = "create index if not exists volid_createtime on snapshottbl (volid, createtime)";
    rc = sqlite3_exec(db, sql.c_str(), NULL, NULL,  &zErrMsg);
    CHECK_SQL_CODE("unable to create index on snaphot table");

    LOGDEBUG << "timeline db setup successfully";
    return ERR_OK;
}

Error TimelineDB::addJournalFile(fds_volid_t volId,
                                 TimeStamp startTime, const std::string& journalFile) {
    DECLARE_DB_VARS();
    sql = util::strformat("insert into journaltbl (volid, starttime, filename) "
                                      "values (%ld, %ld, '%s')", volId.get(),
                                      startTime, journalFile.c_str());
    LOGDEBUG << "sql: " << sql;
    rc = sqlite3_exec(db, sql.c_str(), NULL, NULL,  &zErrMsg);
    CHECK_SQL_CODE("unable to add file to journaldb");
    return ERR_OK;
}

Error TimelineDB::removeJournalFile(fds_volid_t volId, const std::string& journalFile) {
    DECLARE_DB_VARS();
    sql = util::strformat("delete from journaltbl where volid=%ld and filename = '%s'",
                                      volId.get(),
                                      journalFile.c_str());
    LOGDEBUG << "sql: " << sql;
    rc = sqlite3_exec(db, sql.c_str(), NULL, NULL,  &zErrMsg);
    CHECK_SQL_CODE("unable to add file to journal tbl");

    return ERR_OK;
}

Error TimelineDB::removeOldJournalFiles(fds_volid_t volId, TimeStamp uptoTime,
                                        std::vector<JournalFileInfo>& vecJournalFiles) {
    DECLARE_DB_VARS();
    Error err(ERR_OK);
    // get the latest file lesser than the up to time
    sql = util::strformat("select starttime from journaltbl where "
                          "starttime <= %ld order by starttime desc limit 1",
                          uptoTime);
    TimeStamp startTime = 0;
    getInt(sql, startTime);
    if (startTime == 0) {
        LOGDEBUG << "no old files to be removed before : " << uptoTime;
        return err;
    }
    // dec the time by 1 so that we get the correct journal files.
    startTime -= 1;
    getJournalFiles(volId, 0, startTime, vecJournalFiles);
    sql = util::strformat(
        "delete from journaltbl where "
        "volid = %ld and starttime <= %ld",
        volId.get(), startTime);
    rc = sqlite3_exec(db, sql.c_str(), NULL, NULL,  &zErrMsg);
    CHECK_SQL_CODE("unable to delete records from db");
    return err;
}

Error TimelineDB::getJournalFiles(fds_volid_t volId, TimeStamp fromTime, TimeStamp toTime,
                                  std::vector<JournalFileInfo>& vecJournalFiles) {
    DECLARE_DB_VARS();
    Error err(ERR_OK);
    TimeStamp startTime = 0;

    // get the highest time <= fromTime
    sql = util::strformat("select starttime from journaltbl where volid=%ld and "
                          "starttime <= %ld order by starttime desc limit 1",
                          volId.get(), fromTime);
    getInt(sql, startTime);

    sql = util::strformat(
        "select starttime, filename from journaltbl where "
        "volid = %ld and starttime >= %ld and starttime <= %ld",
        volId.get(), startTime, toTime);

    sqlite3_stmt * stmt;
    const char * pzTail;
    rc = sqlite3_prepare(db, sql.c_str(), -1, &stmt, &pzTail);
    CHECK_DB_CODE("unable to prepare statement");

    JournalFileInfo fileinfo;
    do {
        rc = sqlite3_step(stmt);
        switch (rc) {
            case SQLITE_ROW:
                fileinfo.startTime = sqlite3_column_int64(stmt, 0);
                fileinfo.journalFile.assign(reinterpret_cast<const char*>(
                    sqlite3_column_text(stmt, 1)));
                vecJournalFiles.push_back(fileinfo);
                break;
            case SQLITE_DONE:
                break;
            default:
                err = ERR_INVALID;
                LOGERROR << "unknown return code : " << rc << " : " << sqlite3_errmsg(db);
            }
    } while (rc == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return ERR_OK;
}

Error TimelineDB::addSnapshot(fds_volid_t volId, fds_volid_t snapshotId, TimeStamp createTime) {
    DECLARE_DB_VARS();
    sql = util::strformat("insert into snapshottbl (volid, createtime, snapshotid) "
                          "values (%ld, %ld, %ld)",
                          volId.get(),
                          createTime,
                          snapshotId.get());
    LOGDEBUG << "sql: " << sql;
    rc = sqlite3_exec(db, sql.c_str(), NULL, NULL,  &zErrMsg);
    CHECK_SQL_CODE("unable to add snapshot info to snapshot table");
    return ERR_OK;
}

Error TimelineDB::removeSnapshot(fds_volid_t snapshotId) {
    DECLARE_DB_VARS();
    sql = util::strformat("delete from snapshottbl where snapshotid=%ld",
                          snapshotId.get());
    LOGDEBUG << "sql: " << sql;
    rc = sqlite3_exec(db, sql.c_str(), NULL, NULL,  &zErrMsg);
    CHECK_SQL_CODE("unable to remove snapshot from snapshot tbl");

    return ERR_OK;
}

Error TimelineDB::getLatestSnapshotAt(fds_volid_t volId,
                                      TimeStamp uptoTime,
                                      fds_volid_t& snapshotId) {
    DECLARE_DB_VARS();
    snapshotId = 0;
    sql = util::strformat(
        "select snapshotid from snapshottbl "
        "where volid=%ld and createtime <= %ld order by createtime desc LIMIT 1",
        volId.get(), uptoTime);
    return getInt(sql, *(reinterpret_cast<fds_uint64_t*>(&snapshotId) ));
}

Error TimelineDB::getSnapshotTime(fds_volid_t volId,
                                  fds_volid_t snapshotId, TimeStamp& createTime) {
    DECLARE_DB_VARS();
    createTime = 0;
    sql = util::strformat(
        "select createtime from snapshottbl "
        "where volid=%ld and snapshotid = %ld  LIMIT 1",
        volId.get(), snapshotId.get());
    return getInt(sql, createTime);
}

Error TimelineDB::getInt(const std::string& sql, fds_uint64_t& data) {
    char *zErrMsg = 0;
    int  rc;
    Error err(ERR_OK);
    sqlite3_stmt * stmt;
    const char * pzTail;

    rc = sqlite3_prepare(db, sql.c_str(), -1, &stmt, &pzTail);
    CHECK_DB_CODE("unable to prepare statement");
    do {
        rc = sqlite3_step(stmt);
        switch (rc) {
            case SQLITE_ROW:
                data = sqlite3_column_int64(stmt, 0);
                break;
            case SQLITE_DONE:
                break;
            default:
                err = ERR_INVALID;
                LOGERROR << "unknown return code : " << rc << " : " << sqlite3_errmsg(db);
        }
    } while (rc == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return err;
}

Error TimelineDB::removeVolume(fds_volid_t volId) {
    DECLARE_DB_VARS();
    sql = util::strformat("delete from snapshottbl where volid=%ld", volId.get());
    rc = sqlite3_exec(db, sql.c_str(), NULL, NULL,  &zErrMsg);
    CHECK_SQL_CODE("unable to remove volume from snapshot tbl");

    sql = util::strformat("delete from journaltbl where volid=%ld", volId.get());
    rc = sqlite3_exec(db, sql.c_str(), NULL, NULL,  &zErrMsg);
    CHECK_SQL_CODE("unable to remove volume from journal tbl");

    return ERR_OK;
}

Error TimelineDB::close() {
    if (db) {
        sqlite3_close(db);
        db = NULL;
    }
    return ERR_OK;
}

TimelineDB::~TimelineDB() {
    close();
}

}  // namespace timeline
}  // namespace fds
