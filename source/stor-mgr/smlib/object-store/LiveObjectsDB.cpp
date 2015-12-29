/**
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <object-store/LiveObjectsDB.h>

namespace fds {

LiveObjectsDB::LiveObjectsDB(const std::string& dbFilePath) {
    db.reset(new SqliteDB(dbFilePath));
}

void LiveObjectsDB::dropDB() {
    db->dropDB();
}

Error LiveObjectsDB::createLiveObjectsTblAndIdx() {
    SCOPEDWRITE(lock);
    if (!db) { return ERR_INVALID; }

    std::string query = "create table if not exists liveobjectstbl"
                        " (smtoken integer, volid integer, "
                        " timestamp integer not null, filename text,"
                        " primary key(smtoken, volid))";

    if (db->execute(query)) {
        LOGERROR << "Failed to create live objects table";
        return ERR_INVALID;
    }

    query = "create index if not exists idxvolid on liveobjectstbl (volid)";
    if (db->execute(query)) {
        LOGERROR << "Failed to create volid index on live object table";
        return ERR_INVALID;
    }

    query = "create index if not exists idxtimestamp on liveobjectstbl (timestamp)";
    if (db->execute(query)) {
        LOGERROR << "Failed to create timestamp index on live object table";
        return ERR_INVALID;
    }

    query = "create table if not exists tokentbl"
            " (smtoken integer, timestamp integer"
            " primary key(smtoken))";

    if (db->execute(query)) {
        LOGERROR << "Failed to create tokenlbl";
        return ERR_INVALID;
    }

    query = "create index if not exists idxtimestamp on tokentbl (timestamp)";
    if (db->execute(query)) {
        LOGERROR << "Failed to create timestamp index on tokentbl";
        return ERR_INVALID;
    }

    return ERR_OK;
}

Error LiveObjectsDB::addObjectSet(const fds_token_id &smToken,
                                  const fds_volid_t &volId,
                                  const util::TimeStamp &timeStamp,
                                  const std::string &objectSetFilePath ) {
    SCOPEDWRITE(lock);
    if (!db) { return ERR_INVALID; }

    std::string query = util::strformat("insert or replace into liveobjectstbl "
                                        "(smtoken, volid, timestamp, filename) "
                                        "values (%ld, %ld, %ld, '%s')",
                                        smToken, volId.get(), timeStamp,
                                        objectSetFilePath.c_str());
    if (db->execute(query.c_str())) {
        LOGERROR << "Failed to add object set to live object table";
        return ERR_INVALID;
    }
    return ERR_OK;
}

Error LiveObjectsDB::cleansertObjectSet(const fds_token_id &smToken,
                                        const fds_volid_t &volId,
                                        const util::TimeStamp &timeStamp,
                                        const std::string &objectSetFilePath) {
    SCOPEDWRITE(lock);
    if (!db) { return ERR_INVALID; }

    std::string query = util::strformat("delete from liveobjectstbl "
                                        "where smtoken=%ld and volid=%ld",
                                        smToken, volId.get());
    if (db->execute(query.c_str())) {
        LOGERROR << "Failed deleting object set from live object table"
                 << " of smtoken = " << smToken
                 << " and volid = " << volId.get();
        return ERR_INVALID;
    }

    query = util::strformat("insert or replace into liveobjectstbl "
                            "(smtoken, volid, timestamp, filename) "
                            "values (%ld, %ld, %ld, '%s')",
                            smToken, volId.get(), timeStamp,
                            objectSetFilePath.c_str());
    if (db->execute(query.c_str())) {
        LOGERROR << "Failed to add object set to live object table";
        return ERR_INVALID;
    }
    return ERR_OK;
}

Error LiveObjectsDB::removeObjectSet(const fds_token_id &smToken,
                                     const fds_volid_t &volId) {
    SCOPEDWRITE(lock);
    if (!db) { return ERR_INVALID; }

    std::string query = util::strformat("delete from liveobjectstbl "
                                        "where smtoken=%ld and volid=%ld",
                                        smToken, volId.get());
    if (db->execute(query.c_str())) {
        LOGERROR << "Failed deleting object set from live object table"
                 << " of smtoken = " << smToken << " and volid = " << volId.get();
        return ERR_INVALID;
    }
    return ERR_OK;
}

Error LiveObjectsDB::removeObjectSet(const fds_token_id &smToken) {
    SCOPEDWRITE(lock);
    if (!db) { return ERR_INVALID; }

    std::string query = util::strformat("delete from liveobjectstbl "
                                        "where smtoken=%ld",
                                        smToken);
    if (db->execute(query.c_str())) {
        LOGERROR << "Failed deleting object set from live object table"
                 << " of smtoken = " << smToken;
        return ERR_INVALID;
    }
    return ERR_OK;
}

Error LiveObjectsDB::removeObjectSet(const fds_volid_t &volId) {
    SCOPEDWRITE(lock);
    if (!db) { return ERR_INVALID; }

    std::string query = util::strformat("delete from liveobjectstbl "
                                        "where volid=%ld", volId.get());
    if (db->execute(query.c_str())) {
        LOGERROR << "Failed deleting object set(s) from live object table"
                 << " for volid = " << volId.get();
        return ERR_INVALID;
    }
    return ERR_OK;
}

Error LiveObjectsDB::findObjectSetsPerToken(const fds_token_id &smToken,
                                            std::set<std::string> &objSetFilenames) {
    SCOPEDREAD(lock);
    if (!db) { return ERR_INVALID; }

    std::string query = util::strformat("select filename from liveobjectstbl where smtoken=%ld", smToken);
    if (!(db->getTextValues(query.c_str(), objSetFilenames))) {
        LOGERROR << "Failed getting filenames from live object table"
                 << " for smtoken = " << smToken;
        return ERR_INVALID;
    }
    return ERR_OK;
}

Error LiveObjectsDB::findAssociatedVols(const fds_token_id &smToken,
                                        std::set<fds_volid_t> &volumes) {
    SCOPEDREAD(lock);
    if (!db) { return ERR_INVALID; }

    std::set<fds_uint64_t> volumeSet;
    std::string query = util::strformat("select volid from liveobjectstbl where smtoken=%ld", smToken);
    if (!(db->getIntValues(query.c_str(), volumeSet))) {
        LOGERROR << "Failed getting volume associations from live object table"
                 << " for objects in smtoken = " << smToken;
        return ERR_INVALID;
    }

    for (const auto& volId : volumeSet) {
        volumes.insert(fds_volid_t(volId));
    }
    return ERR_OK;
}

Error LiveObjectsDB::findAllVols(std::set<fds_volid_t> &volumes) {
    SCOPEDREAD(lock);
    if (!db) { return ERR_INVALID; }

    std::set<fds_uint64_t> volumeSet;
    std::string query = "select volid from liveobjectstbl";
    if (!(db->getIntValues(query.c_str(), volumeSet))) {
        LOGERROR << "Failed getting all volume associations from live object table";
        return ERR_INVALID;
    }

    for (const auto& volId : volumeSet) {
        volumes.insert(fds_volid_t(volId));
    }
    return ERR_OK;
}

bool LiveObjectsDB::haveAllObjectSets(const fds_token_id &smToken,const std::set<fds_volid_t> &volumes, TimeStamp ts) {
    SCOPEDREAD(lock);
    if (!db) { return false; }

    std::ostringstream oss;
    oss << "select count(smtoken) from liveobjectstbl where smtoken=" << smToken << " and volid in (";
    auto volcount = volumes.size();
    for (const auto& volid : volumes) {
        volcount--;
        oss << volid;
        if (volcount > 0) oss << ",";
    }
    oss<<") and timestamp>" << ts;

    fds_uint64_t count = 0;
    if (!(db->getIntValue(oss.str(), count))) {
        LOGERROR << "Failed in query : " << oss.str();
        return false;
    }

    return count == volumes.size();
}

Error LiveObjectsDB::findMinTimeStamp(const fds_token_id &smToken, TimeStamp &ts) {
    SCOPEDREAD(lock);
    if (!db) { return ERR_INVALID; }

    std::string query = util::strformat("select MIN(timestamp) from liveobjectstbl where smtoken=%ld", smToken);
    if (!(db->getIntValue(query.c_str(), ts))) {
        LOGERROR << "Failed getting timestamp information from live object table"
                 << " for objects in smtoken = " << smToken;
        return ERR_INVALID;
    }
    return ERR_OK;
}

Error LiveObjectsDB::setTokenStartTime(const fds_token_id &smToken, TimeStamp &ts) {
    SCOPEDREAD(lock);
    if (!db) { return ERR_INVALID; }
    std::string query = util::strformat("insert or replace into tokentbl "
                                        "(smtoken, timestamp) "
                                        "values (%ld, %ld)",
                                        smToken, ts);

    if (db->execute(query.c_str())) {
        LOGERROR << "failed: " << query;
        return ERR_INVALID;
    }
    return ERR_OK;
}

TimeStamp LiveObjectsDB::getTokenStartTime(const fds_token_id &smToken) {
    SCOPEDREAD(lock);
    if (!db) { return 0; }
    TimeStamp ts;
    std::string query = util::strformat("select timestamp from tokentbl where smtoken=%ld", smToken);
    if (!(db->getIntValue(query.c_str(), ts))) {
        LOGERROR << "failed: " << query;
        return 0;
    }
    return ts;
}

} // end namespace fds
