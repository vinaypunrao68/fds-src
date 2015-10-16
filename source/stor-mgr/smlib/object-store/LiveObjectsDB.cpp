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
                        " (smToken integer, volId integer,"
                        " timeStamp integer not null, filename text not null)";

    if (db->execute(query.c_str())) {
        LOGERROR << "Failed creating live objects table";
        return ERR_INVALID;
    }

    query = "create index if not exists smTokenIdx on liveObjTbl (smToken, volId)";
    if (db->execute(query.c_str())) {
        LOGERROR << "Failed creating smToken index on live object table";
        return ERR_INVALID;
    }
    return ERR_OK;
}

Error LiveObjectsDB::addObjectSet(const fds_token_id &smToken,
                                  const fds_volid_t &volId,
                                  const TimeStamp &timeStamp,
                                  const std::string &objectSetFilePath) {
    if (!db) { return ERR_INVALID; }

    std::string query = util::strformat("insert into liveObjTbl "
                                        "(smToken, volId, timeStamp, filename) "
                                        "values (%ld, %ld, %ld, '%s')",
                                        smToken, volId.get(), timeStamp,
                                        objectSetFilePath.c_str());
    if (db->execute(query.c_str())) {
        LOGERROR << "Failed adding object set to live object table";
        return ERR_INVALID;
    }
    return ERR_OK;
}

Error LiveObjectsDB::removeObjectSet(const fds_token_id &smToken,
                                     const fds_volid_t &volId) {
    if (!db) { return ERR_INVALID; }

    std::string query = util::strformat("delete from liveObjTbl "
                                        "where smToken=%ld and volId=%ld",
                                        smToken, volId.get());
    if (db->execute(query.c_str())) {
        LOGERROR << "Failed deleting object set from live object table"
                 << " of smToken = " << smToken << " and volId = " << volId.get();;
        return ERR_INVALID;
    }
    return ERR_OK;
}

Error LiveObjectsDB::findObjectSetsPerToken(const fds_token_id &smToken,
                                            std::vector<std::string> &objSetFilenames) {
    if (!db) { return ERR_INVALID; }

    std::string query = util::strformat("select filename from liveObjTbl where smToken=%ld", smToken);
    if (db->getTextValues(query.c_str(), objSetFilenames)) {
        LOGERROR << "Failed getting filenames from live object table"
                 << " for smToken = " << smToken;
        return ERR_INVALID;
    }
    return ERR_OK;
}

} // end namespace fds
