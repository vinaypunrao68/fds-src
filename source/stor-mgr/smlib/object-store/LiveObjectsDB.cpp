/**
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <LiveObjectDB.h>

namespace fds {


LiveObjectsDB::LiveObjectsDB(const std::string& dbFilePath) {
    db.reset(new SqliteDB(dbFilePath));
}

Error LiveObjectsDB::createLiveObjectsTblAndIdx() {
    if (!db) { return ERR_INVALID; }

    sql = "create table if not exists liveObjTbl"
          " (smToken integer, volId integer,"
          " timeStamp integer not null, filename text not null)";

    if (db->execute(sql.c_str())) {
        LOGERROR << "Failed creating live objects table";
        return ERR_INVALID;
    }

    sql = "create index if not exists smTokenIdx on liveObjTbl (smToken, volId)";
    if (db->execute(sql.c_str())) {
        LOGERROR << "Failed creating smToken index on live object table";
        return ERR_INVALID;
    }
    return ERR_OK;
}

Error LiveObjectsDB::addObjectSet(fds_token_id& smToken,
                                  fds_volid_t& volId,
                                  TimeStamp& timeStamp,
                                  std::string objectSetFilePath) {
    if (!db) { return ERR_INVALID; }

    query = util::strformat("insert into liveObjTbl "
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

Error LiveObjectsDB::removeObjectSet(fds_token_id& smToken,
                                     fds_volid_t& volId);
    if (!db) { return ERR_INVALID; }

    query = util::strformat("delete from liveObjTbl "
                            "where smToken=%ld and volId=%ld",
                            smToken, volId.get());
    if (db->execute(query.c_str())) {
        LOGERROR << "Failed deleting object set from live object table"
                 << " of smToken = " << smToken << " and volId = " << volId.get();;
        return ERR_INVALID;
    }
    return ERR_OK;
}

Error LiveObjectsDB::findObjectSetsPerToken(const fds_toke_id &smToken,
                                            std::vector<std::string> &objSetFilenames) {
    if (!db) { return ERR_INVALID; }

    query = util::strformat("select filename from liveObjTbl where smToken=%ld", smToken);
    if (db->getTextValues(query.c_str(), objsetFilenames)) {
        LOGERROR << "Failed getting filenames from live object table"
                 << " for smToken = " << smToken;
        return ERR_INVALID;
    }
}

} // end namespace fds
