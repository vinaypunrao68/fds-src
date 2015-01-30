/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <fds_assert.h>
#include <fds_process.h>
#include <sm_ut_utils.h>
#include <object-store/SmDiskMap.h>
#include <object-store/ObjectMetaDb.h>
#include <object-store/ObjectMetadataStore.h>
#include <object-store/ObjectStore.h>
#include <vector>
#include <string>
#include <boost/program_options.hpp>

#include "smchk.h"

namespace fds {
SMChk::SMChk(int sm_count, SmDiskMap::ptr smDiskMap,
        ObjectDataStore::ptr smObjStore, ObjectMetadataDb::ptr smMdDb):
        sm_count(sm_count),
        smDiskMap(smDiskMap),
        smObjStore(smObjStore),
        smMdDb(smMdDb) {
    fds_uint32_t cols = (sm_count < 4) ? sm_count : 4;
    DLT* dlt = new DLT(8, cols, 1, true);
    SmUtUtils::populateDlt(dlt, sm_count);
    GLOGDEBUG << "Using DLT: " << *dlt;
    Error err = smDiskMap->handleNewDlt(dlt);
    fds_verify(err.ok() || (err == ERR_SM_NOERR_PRISTINE_STATE));

    // Open the data store
    smObjStore->openDataStore(smDiskMap,
                              (err == ERR_SM_NOERR_PRISTINE_STATE));

    // Meta db
    smMdDb->openMetadataDb(smDiskMap);

    // we don't need dlt anymore
    delete dlt;
}

SMChk::SMChk(ObjectDataStore::ptr smObjStore, ObjectMetadataDb::ptr smMdDb)
        : smDiskMap(nullptr), sm_count(0),
        smObjStore(smObjStore),
        smMdDb(smMdDb) {}

SmTokenSet SMChk::getSmTokens() {
    return smDiskMap->getSmTokens();
}

void SMChk::list_path_by_token() {
    SmTokenSet smToks = getSmTokens();
    for (SmTokenSet::const_iterator cit = smToks.cbegin();
         cit != smToks.cend();
         ++cit) {
        std::cout << "Token " << *cit << "\ndisk path: "
                << smDiskMap->getDiskPath(*cit, diskio::diskTier)
                << "\n";
    }
}

void SMChk::list_token_by_path() {
    std::unordered_map<std::string, std::vector<fds_token_id>> rev_map;

    SmTokenSet smToks = getSmTokens();
    for (SmTokenSet::const_iterator cit = smToks.cbegin();
         cit != smToks.cend(); ++cit) {
        std::string path(smDiskMap->getDiskPath(*cit, diskio::diskTier));
        rev_map[path].push_back(*cit);
    }
    for (auto entry : rev_map) {
        std::cout << "PATH: " << entry.first << "\n";
        for (auto val : entry.second) {
            std::cout << "SM Token: " << val << "\n";
        }
    }
}

void SMChk::list_metadata() {
    MetadataIterator md_iter(this);
    for (md_iter.start(); !md_iter.end(); md_iter.next()) {
        std::cout << *(md_iter.value()) << "\n";
    }
}

void SMChk::list_active_metadata() {
    MetadataIterator md_iter(this);
    for (md_iter.start(); !md_iter.end(); md_iter.next()) {
        boost::shared_ptr<ObjMetaData> omd = md_iter.value();
        if (omd->getRefCnt() > 0L) {
            std::cout << *(md_iter.value()) << "\n";
        }
    }
}

int SMChk::bytes_reclaimable() {
    int bytes = 0;
    MetadataIterator md_it(this);
    for (md_it.start(); !md_it.end(); md_it.next()) {
        boost::shared_ptr<ObjMetaData> omd = md_it.value();
        if (omd->getRefCnt() == 0L) {
            bytes += omd->getObjSize();
        }
    }
    GLOGNORMAL << "Found " << bytes << " bytes reclaimable.\n";
    return bytes;
}

ObjectID SMChk::hash_data(boost::shared_ptr<const std::string> dataPtr, fds_uint32_t obj_size) {
    return ObjIdGen::genObjectId((*dataPtr).c_str(),
            static_cast<size_t>(obj_size));
}

bool SMChk::consistency_check(fds_token_id token) {
    leveldb::DB *ldb;
    leveldb::Iterator *it;
    leveldb::ReadOptions options;

    int error_count = 0;

    smMdDb->snapshot(token, ldb, options);
    it = ldb->NewIterator(options);
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        const ObjectID id(it->key().ToString());


        boost::shared_ptr<ObjMetaData> omd =
                boost::shared_ptr<ObjMetaData>(new ObjMetaData());
        omd->deserializeFrom(it->value());

        fds_uint32_t obj_size = omd->getObjSize();

        GLOGDEBUG << *omd << "\n" << "Disk Path: "
                    << smDiskMap->getDiskPath(token, diskio::diskTier);

        // Get object
        Error err(ERR_OK);
        boost::shared_ptr<const std::string> dataPtr
                = smObjStore->getObjectData(invalid_vol_id, id, omd, err);

        ObjectID hashId = hash_data(dataPtr, obj_size);

        GLOGDEBUG << "Hasher found: " << hashId
                    << " and objId was: " << id << "\n";

        // Compare hash to objID
        if (hashId != id) {
            GLOGNORMAL << "An error was found with " << id << "\n" << *omd;
            error_count++;
        }
        // omd will auto delete when it goes out of scope
    }
    ldb->ReleaseSnapshot(options.snapshot);
    // Delete ldb and it
    delete it;
    delete ldb;

    if (error_count > 0) {
        GLOGNORMAL << "WARNING: " << error_count << " errors were found.\n";
        return false;
    }
    GLOGNORMAL << "SUCCESS! No errors found.\n";
    return true;
}

bool SMChk::consistency_check(ObjectID obj_id) {
    return true;
}

bool SMChk::full_consistency_check() {
    int error_count = 0;
    int objs_count = 0;
    MetadataIterator md_it(this);
    for (md_it.start(); !md_it.end(); md_it.next()) {
        const ObjectID id(md_it.key());
        boost::shared_ptr<ObjMetaData> omd = md_it.value();
        fds_uint32_t obj_size = omd->getObjSize();

        // Get object
        Error err(ERR_OK);
        boost::shared_ptr<const std::string> dataPtr
                = smObjStore->getObjectData(invalid_vol_id, id, omd, err);

        // Get hash of data
        ObjectID hashId = hash_data(dataPtr, obj_size);

        GLOGDEBUG << "Hasher found: " << hashId
                    << " and objId was: " << id << "\n";

        // Compare hash to objID
        if (hashId != id) {
            GLOGNORMAL << "An error was found with " << id << "\n";
            GLOGNORMAL << id << " corrupt!\n" << *omd;
            error_count++;
        }
        // Increment the number of objects we've checked
        objs_count++;
        // omd will auto delete when it goes out of scope
    }

    GLOGNORMAL << objs_count << " objects checked, " << error_count << " errors were found.\n";

    // For convenience, tests will look at stdout.
    std::cout << objs_count << " objects checked, " << error_count << " errors were found." << std::endl;

    if (error_count > 0) {
        GLOGNORMAL << "WARNING: " << error_count << " errors were found. "
                << objs_count << " objects were checked. \n";
        return false;
    }
    GLOGNORMAL << "SUCCESS! " << objs_count << " objects checked; no errors found.\n";
    return true;
}

// ---------------------- MetadataIterator -------------------------------- //

SMChk::MetadataIterator::MetadataIterator(SMChk * instance) {
    smchk = instance;
    ldb = nullptr;
    ldb_it = nullptr;
    all_toks = smchk->getSmTokens();
}

void SMChk::MetadataIterator::start() {
    token_it = all_toks.begin();
    GLOGNORMAL << "Reading metadata db for SM token " << *token_it << "\n";
    smchk->smMdDb->snapshot(*token_it, ldb, options);
    ldb_it = ldb->NewIterator(options);
    ldb_it->SeekToFirst();
    if (!ldb_it->Valid()) {
        next();
    }
}

bool SMChk::MetadataIterator::end() {
    if (token_it == all_toks.end()) {
        if (!ldb_it->Valid()) {
            return true;
        }
    }
    return false;
}

void SMChk::MetadataIterator::next() {
    // If we're currently valid, lets try to call next
    if (ldb_it->Valid()) {
        ldb_it->Next();
    }
    // If we're no longer valid (or never were, keep moving until we are)
    while (!ldb_it->Valid()) {
        // Moving to next was invalid so we must increase token iterator
        ++token_it;
        if (end()) { return; }

        delete ldb_it;
        delete ldb;

        // Create new from new token
        GLOGNORMAL << "Reading metadata db for SM token " << *token_it << "\n";
        smchk->smMdDb->snapshot(*token_it, ldb, options);
        ldb_it = ldb->NewIterator(options);
        ldb_it->SeekToFirst();
    }
}

std::string SMChk::MetadataIterator::key() {
    return ldb_it->key().ToString();
}

boost::shared_ptr<ObjMetaData> SMChk::MetadataIterator::value() {
    if (!end()) {
        // We should have a valid levelDB / ldb_it
        omd = boost::shared_ptr<ObjMetaData>(new ObjMetaData());
        omd->deserializeFrom(ldb_it->value());
    }
    return omd;
}
}  // namespace fds
