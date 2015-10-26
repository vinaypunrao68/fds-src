/**
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <object-store/LiveObjectsDB.h>

namespace fds {

LiveObjectsDB::LiveObjectsDB(const std::string& dbFilePath) {
    db.reset(new SqliteDB(dbFilePath));
}

Error LiveObjectsDB::createLiveObjectsTblAndIdx() {
    if (!db) { return ERR_INVALID; }

    std::string query = "create table if not exists liveObjTbl"
                        " (smtoken integer, volid integer, dmuuid integer,"
                        " timestamp integer not null, filename text not null)";

    if (db->execute(query.c_str())) {
        LOGERROR << "Failed to create live objects table";
        return ERR_INVALID;
    }

    query = "create index if not exists smTokenIdx on liveObjTbl (smtoken, volid)";
    if (db->execute(query.c_str())) {
        LOGERROR << "Failed to create smtoken index on live object table";
        return ERR_INVALID;
    }
    return ERR_OK;
}

Error LiveObjectsDB::addObjectSet(const fds_token_id &smToken,
                                  const fds_volid_t &volId,
                                  const fds_uint64_t &dmUUID,
                                  const util::TimeStamp &timeStamp,
                                  const std::string &objectSetFilePath) {
    if (!db) { return ERR_INVALID; }

    std::string query = util::strformat("insert into liveObjTbl "
                                        "(smtoken, volid, dmuuid, timestamp, filename) "
                                        "values (%ld, %ld, %ld, %ld, '%s')",
                                        smToken, volId.get(), dmUUID, timeStamp,
                                        objectSetFilePath.c_str());
    if (db->execute(query.c_str())) {
        LOGERROR << "Failed to add object set to live object table";
        return ERR_INVALID;
    }
    return ERR_OK;
}

Error LiveObjectsDB::removeObjectSet(const fds_token_id &smToken,
                                     const fds_volid_t &volId) {
    if (!db) { return ERR_INVALID; }

    std::string query = util::strformat("delete from liveObjTbl "
                                        "where smtoken=%ld and volid=%ld",
                                        smToken, volId.get());
    if (db->execute(query.c_str())) {
        LOGERROR << "Failed deleting object set from live object table"
                 << " of smtoken = " << smToken << " and volid = " << volId.get();;
        return ERR_INVALID;
    }
    return ERR_OK;
}

Error LiveObjectsDB::findObjectSetsPerToken(const fds_token_id &smToken,
                                            std::set<std::string> &objSetFilenames) {
    if (!db) { return ERR_INVALID; }

    std::string query = util::strformat("select filename from liveObjTbl where smtoken=%ld", smToken);
    if (db->getTextValues(query.c_str(), objSetFilenames)) {
        LOGERROR << "Failed getting filenames from live object table"
                 << " for smtoken = " << smToken;
        return ERR_INVALID;
    }
    return ERR_OK;
}

Error LiveObjectsDB::findAssociatedVols(const fds_token_id &smToken,
                                        std::set<fds_volid_t> &volumes) {
    if (!db) { return ERR_INVALID; }

    std::set<fds_uint64_t> volumeSet;
    std::string query = util::strformat("select volid from liveObjTbl where smtoken=%ld", smToken);
    if (db->getIntValues(query.c_str(), volumeSet)) {
        LOGERROR << "Failed getting volume associations from live object table"
                 << " for objects in smtoken = " << smToken;
        return ERR_INVALID;
    }

    for (auto volId : volumeSet) {
        volumes.insert(fds_volid_t(volId));
    }
    return ERR_OK;
}

} // end namespace fds
