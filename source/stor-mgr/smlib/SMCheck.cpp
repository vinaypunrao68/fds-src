/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <fds_assert.h>
#include <fds_process.h>
#include "platform/platform.h"
#include <StorMgr.h>

#include <ObjectId.h>
#include <boost/program_options.hpp>
#include <net/SvcMgr.h>

#include <SMCheck.h>

namespace fds {

SMCheckOffline::SMCheckOffline(SmDiskMap::ptr smDiskMap,
                 ObjectDataStore::ptr smObjStore,
                 ObjectMetadataDb::ptr smMdDb,
                 bool verboseMsg = false)
    : smDiskMap(smDiskMap),
      smObjStore(smObjStore),
      smMdDb(smMdDb),
      verbose(verboseMsg)
{

    std::string uuidPath = g_fdsprocess->proc_fdsroot()->dir_fds_logs() + UUIDFileName;
    if (access(uuidPath.c_str(), F_OK) == -1) {
       std::cout << "uuid file (" << uuidPath << ") doesn't exists.  "
                 << "Cannot properly map DLT tokens.  Exiting..."
                 << std::endl;
       exit(0);
    }

    uint64_t tmpUuid;
    std::ifstream uuidFile(uuidPath);
    uuidFile >> tmpUuid;
    smUuid.uuid_set_val(tmpUuid);

    if (verbose) {
        std::cout << "Uuid=" << smUuid << std::endl;
    }

    // Load current DLT from the file.
    curDLT = new DLT(0, 0, 0, false);
    std::string dltPath = g_fdsprocess->proc_fdsroot()->dir_fds_logs() + DLTFileName;
    if (access(dltPath.c_str(), F_OK) == -1) {
       std::cout << "DLT file (" << dltPath << ") doesn't exists.  "
                 << "Cannot properly map DLT tokens.  Exiting..."
                 << std::endl;
       exit(0);
    }
    curDLT->loadFromFile(dltPath);
    curDLT->generateNodeTokenMap();
    if (verbose) {
        std::cout << "DLT Table:" << std::endl << *curDLT << std::endl;
    }

    Error err = smDiskMap->loadPersistentState();
    fds_verify(err.ok() || (err == ERR_SM_NOERR_PRISTINE_STATE));

    Error dltErr = smDiskMap->handleNewDlt(curDLT, smUuid);
    fds_verify(dltErr.ok() ||
               (dltErr == ERR_SM_NOERR_GAINED_SM_TOKENS) ||
               (dltErr == ERR_SM_NOERR_NEED_RESYNC));

    // Open the data store
    smObjStore->openDataStore(smDiskMap,
                              (err == ERR_SM_NOERR_PRISTINE_STATE));

    // Meta db
    smMdDb->openMetadataDb(smDiskMap);

}

void SMCheckOffline::list_path_by_token() {
    SmTokenSet smToks = getSmTokens();
    for (SmTokenSet::const_iterator cit = smToks.cbegin();
         cit != smToks.cend();
         ++cit) {
        std::cout << "Token " << *cit << "\ndisk path: "
                  << smDiskMap->getDiskPath(*cit, diskio::diskTier)
                  << "\n";
    }
}

SmTokenSet SMCheckOffline::getSmTokens() {
    return smDiskMap->getSmTokens();
}

void SMCheckOffline::list_token_by_path() {
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

void SMCheckOffline::list_metadata(bool checkOnlyActive) {
    MetadataIterator md_iter(this);
    for (md_iter.start(); !md_iter.end(); md_iter.next()) {
        boost::shared_ptr<ObjMetaData> omd = md_iter.value();
        if (checkOnlyActive && omd->getRefCnt() == 0UL) {
            continue;
        }
        std::cout << *(md_iter.value()) << "\n";
    }
}

int SMCheckOffline::bytes_reclaimable() {
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

ObjectID SMCheckOffline::hash_data(boost::shared_ptr<const std::string> dataPtr, fds_uint32_t obj_size) {
    return ObjIdGen::genObjectId((*dataPtr).c_str(),
            static_cast<size_t>(obj_size));
}

bool SMCheckOffline::consistency_check(fds_token_id token) {
    std::shared_ptr<leveldb::DB> ldb;
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

    if (error_count > 0) {
        GLOGNORMAL << "WARNING: " << error_count << " errors were found.\n";
        return false;
    }
    GLOGNORMAL << "SUCCESS! No errors found.\n";
    return true;
}

bool SMCheckOffline::checkObjectOwnership(const ObjectID& objId)
{
    bool found = false;

    DltTokenGroupPtr nodes = curDLT->getNodes(objId);
    for (uint i = 0; i < nodes->getLength(); ++i) {
        if (nodes->get(i) == smUuid) {
            found = true;
            break;
        }
    }

    return found;
}

bool SMCheckOffline::full_consistency_check(bool checkOwnership, bool checkOnlyActive) {
    int corruptionCount = 0;
    int ownershipMismatch = 0;
    int objs_count = 0;

    if (verbose) {
        std::cout << "Full Consistency Check with options: "
                  << " checkOwnership=" << checkOwnership
                  << " checkOnlyActive=" << checkOnlyActive
                  << std::endl;
    }

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
                    << " and objId was: " << id
                    << "Metadata for obj: " << *omd;

        // Only report active object metadata
        if (checkOnlyActive && (omd->getRefCnt() == 0)) {
            continue;
        }

        // Compare hash to objID
        if (hashId != id) {
            GLOGNORMAL << "An error was found with " << id << "\n";
            GLOGNORMAL << id << " corrupt!\n" << *omd;
            ++corruptionCount;
        }

        if (checkOwnership) {
            bool tokenOwned = checkObjectOwnership(id);
            if (!tokenOwned) {
                ++ownershipMismatch;
            }
        }

        // Increment the number of objects we've checked
        ++objs_count;
        // omd will auto delete when it goes out of scope
    }

    GLOGNORMAL << "Total Objects=" << objs_count
               << ", Corrupted Objects=" << corruptionCount
               << ", Token Ownership Mismatch=" << ownershipMismatch;

    // For convenience, tests will look at stdout.
    // NOTE:  This output is necessary for testing, so do not conditionalize
    //        or remove it.
    std::cout << "Total Objects=" << objs_count
              << ", Corrupted Objects=" << corruptionCount
              << ", Token Ownership Mismatch=" << ownershipMismatch
              << std::endl;

    if (corruptionCount > 0) {
        GLOGERROR << "ERROR: "
                  << "Total Objects=" << objs_count
                  << ", Corruption=" << corruptionCount;
        return false;
    }

    if (ownershipMismatch > 0) {
        GLOGERROR << "Total Objects=" << objs_count
                  << ", Token Ownership Mismatch=" << ownershipMismatch;
        return false;
    }

    GLOGNORMAL << "SUCCESS! " << objs_count << " objects checked; no errors found.\n";
    return true;
}

// ---------------------- MetadataIterator -------------------------------- //

SMCheckOffline::MetadataIterator::MetadataIterator(SMCheckOffline * instance) {
    smchk = instance;
    ldb = nullptr;
    ldb_it = nullptr;
    all_toks = smchk->getSmTokens();
}

void SMCheckOffline::MetadataIterator::start() {
    token_it = all_toks.begin();
    GLOGNORMAL << "Reading metadata db for SM token " << *token_it << "\n";
    smchk->smMdDb->snapshot(*token_it, ldb, options);
    ldb_it = ldb->NewIterator(options);
    ldb_it->SeekToFirst();
    if (!ldb_it->Valid()) {
        next();
    }
}

bool SMCheckOffline::MetadataIterator::end() {
    if (token_it == all_toks.end()) {
        if (!ldb_it->Valid()) {
            return true;
        }
    }
    return false;
}

void SMCheckOffline::MetadataIterator::next() {
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

        // Create new from new token
        GLOGNORMAL << "Reading metadata db for SM token " << *token_it << "\n";
        smchk->smMdDb->snapshot(*token_it, ldb, options);
        ldb_it = ldb->NewIterator(options);
        ldb_it->SeekToFirst();
    }
}

std::string SMCheckOffline::MetadataIterator::key() {
    return ldb_it->key().ToString();
}

boost::shared_ptr<ObjMetaData> SMCheckOffline::MetadataIterator::value() {
    if (!end()) {
        // We should have a valid levelDB / ldb_it
        omd = boost::shared_ptr<ObjMetaData>(new ObjMetaData());
        omd->deserializeFrom(ldb_it->value());
    }
    return omd;
}



// --------------------- online smcheck ---------------------------- //

// TODO(Sean):
// Unfortunately, I think SMCheck is cursed.  Always running into some sort of
// compilation or linker issues.  So, decided to decouple offline vs. online
// smcheck.  This can be refactored, but it will take some effort to do it.
// For now, live with having two flavors for SMCheck.
SMCheckOnline::SMCheckOnline(SmIoReqHandler *datastore, SmDiskMap::ptr diskmap)
    : dataStore(datastore),
      diskMap(diskmap),
      latestClosedDLT(nullptr)

{
    SMChkActive = ATOMIC_VAR_INIT(false);
    resetStats();

    snapRequest.io_type = FDS_SM_SNAPSHOT_TOKEN;
    snapRequest.smio_snap_resp_cb = std::bind(&SMCheckOnline::SMCheckSnapshotCB,
                                              this,
                                              std::placeholders::_1,
                                              std::placeholders::_2,
                                              std::placeholders::_3,
                                              std::placeholders::_4);
}

SMCheckOnline::~SMCheckOnline()
{
    if (nullptr != latestClosedDLT) {
        delete latestClosedDLT;
        latestClosedDLT = nullptr;
    }
}

// Reset all stats.
void
SMCheckOnline::resetStats()
{
    std::atomic_store(&numCorruptions, 0L);
    std::atomic_store(&numOwnershipMismatches, 0L);
    std::atomic_store(&numActiveObjects, 0L);
    totalNumTokens = 0;
    std::atomic_store(&totalNumTokensVerified, 0L);
}

// Get stats
void
SMCheckOnline::getStats(fpi::CtrlNotifySMCheckStatusRespPtr resp)
{
    if (getActiveStatus() == true) {
        resp->SmCheckStatus = fpi::SMCHECK_STATUS_ACTIVE;
    } else {
        resp->SmCheckStatus = fpi::SMCHECK_STATUS_INACTIVE;
    }
    resp->SmCheckCorruption = std::atomic_load(&numCorruptions);
    resp->SmCheckOwnershipMismatch = std::atomic_load(&numOwnershipMismatches);
    resp->SmCheckActiveObjects = std::atomic_load(&numActiveObjects);
    resp->SmCheckTotalNumTokens = totalNumTokens;
    resp->SmCheckTotalNumTokensVerified = std::atomic_load(&totalNumTokensVerified);
}

// Start integrity check.
// For each token owned by SM, it will enqueue a snapshot request for a token.
// After snapshot callback is called, it will process then the call back will
// enqueue snapshot request for the next token in the set...  and so on...
Error
SMCheckOnline::startIntegrityCheck(std::set<fds_token_id> tgtDltTokens)
{
    Error err(ERR_OK);

    // TODO(Sean):
    // Need to disable SM Token migration.  While token migration is going on,
    // the objects may land on the SM different DLT version, since we don't update
    // DLT until it is closed.

    bool success = setActive();
    if (!success) {
        err = ERR_NOT_READY;
        return err;
    }

    // Reset all stats.
    resetStats();

    latestClosedDLT = const_cast<DLT *>(objStorMgr->getDLT());

    // Get UUID of the SM.
    SMCheckUuid = MODULEPROVIDER()->getSvcMgr()->getSelfSvcUuid().svc_uuid;

    GLOGNORMAL << "Starting SM Integrity Check: active=" << getActiveStatus();

    GLOGDEBUG << "Starting SM Integrity Check:"
             << " UUID=" << SMCheckUuid
             << " DLT=" << latestClosedDLT;

    targetDLTTokens = tgtDltTokens;
    // Clear any tokens that were stored from previous runs
    allTokens.clear();
    if (!targetDLTTokens.empty()) {
        for (auto token : targetDLTTokens) {
            fds_token_id smToken;
            smToken = diskMap->smTokenId(token);
            GLOGDEBUG << "Token " << token << " found in sm token: " << smToken;
            allTokens.insert(smToken);
        }
        GLOGDEBUG << "Received target tokens list, verifying " << allTokens.size() << " SM tokens"
                 << " and " << tgtDltTokens.size() << " dlt tokens.\n";
    } else {
        GLOGDEBUG << "No target tokens found, verifying ALL SM tokens.\n";
        // get all the SM tokens in the system.  Now
        allTokens = diskMap->getSmTokens();
    }
    // update stats on number of tokens to be checked
    totalNumTokens = allTokens.size();
    // assert in debug mode.
    fds_assert(totalNumTokens > 0);

    // Set the iterator
    iterTokens = allTokens.begin();

    // This is not possible (I think -- where there is no token owned by SM, but
    // if the token set is not empty, then start checking.
    if (allTokens.end() != iterTokens) {
        snapRequest.token_id = *iterTokens;
        GLOGNORMAL << "Integrity check starting on token=" << *iterTokens;
        err = dataStore->enqueueMsg(FdsSysTaskQueueId, &snapRequest);
    }

    // If the request has failed, then set it inactive.
    if (!err.ok()) {
        setInactive();
        GLOGERROR << "Failed to enqueue snapshot request: active=" << getActiveStatus()
                 << ", err=" << err;
    }

    return err;
}

// check the ownership of an object against latest "cloned" DLT and
// service UUID.
bool
SMCheckOnline::checkObjectOwnership(const ObjectID& objId)
{
    bool found = false;

    DltTokenGroupPtr nodes = latestClosedDLT->getNodes(objId);
    for (uint i = 0; i < nodes->getLength(); ++i) {
        if (nodes->get(i) == SMCheckUuid) {
            found = true;
            break;
        }
    }

    return found;
}

void
SMCheckOnline::SMCheckSnapshotCB(const Error& error,
                                SmIoSnapshotObjectDB* snapReq,
                                leveldb::ReadOptions& options,
                                std::shared_ptr<leveldb::DB> db)
{
    Error err(ERR_OK);

    // This will maintain true, if all metadata belonging to this
    // SM token is completely checked.
    bool checkSuccess = true;

    leveldb::Iterator* ldbIter = db->NewIterator(options);
    for (ldbIter->SeekToFirst(); ldbIter->Valid(); ldbIter->Next()) {
        ObjectID id(ldbIter->key().ToString());

        if (!targetDLTTokens.empty()) {
            // If this object isn't in the DLT token we're checking
            if (targetDLTTokens.count(latestClosedDLT->getToken(id)) == 0) {
                continue;
            }
        }

        ObjMetaData::ptr objMetaDataPtr = ObjMetaData::ptr(new ObjMetaData());
        objMetaDataPtr->deserializeFrom(ldbIter->value());

        // This is set by scrubber functionality of GC.
        if (objMetaDataPtr->isObjCorrupted()) {
            GLOGNORMAL << "Corruption found with object metadata: " << objMetaDataPtr->logString();
            ++numCorruptions;
            continue;
        }

        // Only check the active metadata
        if (objMetaDataPtr->getRefCnt() == 0UL) {
            continue;
        }
        ++numActiveObjects;

        // check the SM token ownership.
        if (!checkObjectOwnership(id)) {
            GLOGNORMAL << "Ownership mismatch found with Object ID: " << id
                        << " Object metadata: " << objMetaDataPtr->logString();
            ++numOwnershipMismatches;
        }

#ifdef OBJECT_STORE_READ_ENABLE

        // TODO(Sean):
        // Can't get this to compile for some reason.  Spent some time on it, but
        // giving up for now.  This online smcheck will be mainly used for the
        // SM Token migration testing, so no doing this is ok.  Data verification
        // is also done by GC, and this is just a reporting mechanism.
        fds_uint32_t objSize = objMetaDataPtr->getObjSize();

        boost::shared_ptr<const std::string> objData =
            objStorMgr->objectStore->getObjectData(invalid_vol_id, id, objMetaDataPtr, err);
        if (!err.ok()) {
            LOGERROR << "Cannot read object: " << objMetaDataPtr->logString();
            LOGERROR << "Incomplete verification on token=" << *iterTokens;
            checkSuccess = false;
            break;
        }

        ObjectID objId;
        objId = ObjIdGen::getObjectId(objData->c_str(), objData->size());

        if (id != objId) {
            ++numCorruptions;
            LOGCRITICAL << "CORRUPTION: metadata ID and ondiskID do not match: "
                        << "metadata=" << id.ToHex().c_str()
                        << "ondisk=" << objId.ToHex().c_str();
        }
#endif
    }

    // delete snapshot related objects.
    delete ldbIter;
    db->ReleaseSnapshot(options.snapshot);

    // if all objects are checked, then mark count it as verified.
    if (checkSuccess) {
        ++totalNumTokensVerified;
    }

    // Advance to the next token
    ++iterTokens;

    // If the iterator has not reached end, then enqueue another QoS request
    // to snapshot the next token to verify.
    if (allTokens.end() != iterTokens) {
        snapRequest.token_id = *iterTokens;
        GLOGNORMAL << "Integrity check starting on token=" << *iterTokens;
        err = dataStore->enqueueMsg(FdsSysTaskQueueId, &snapRequest);

        // If the request has failed, then set it inactive.
        if (!err.ok()) {
            setInactive();
            GLOGERROR << "Failed to enqueue snapshot request: active=" << getActiveStatus()
                     << ", err=" << err;
        }
    } else {
        // If we have checked all tokens, then set the state to inactive.
        setInactive();
    }

    // No longer doing the verification.  So, output the status to the log file.
    if (!getActiveStatus()) {

        GLOGNORMAL << "Completed SM Integrity Check: active=" << getActiveStatus()
                  << " totalNumTokens=" << totalNumTokens
                  << " totalNumTokensVerified=" << std::atomic_load(&totalNumTokensVerified)
                  << " numCorrupted=" << std::atomic_load(&numCorruptions)
                  << " numOwnershipMismatches=" << std::atomic_load(&numOwnershipMismatches)
                  << " numActiveObjects=" << std::atomic_load(&numActiveObjects);
    }

    // TODO(Sean):
    // If SM token migration is disabled, need to re-enable it again.

}

// Clone the latest close DLT.
void
SMCheckOnline::updateDLT(const DLT *latestDLT)
{
    // Delete the previous cloned DLT, if one exists.
    if (nullptr != latestClosedDLT) {
        delete latestClosedDLT;
        latestClosedDLT = nullptr;
    }

    // Clone the DLT.
    latestClosedDLT = latestDLT->clone();

    // Regenerate the node to token map.
    latestClosedDLT->generateNodeTokenMap();

}

// set state of checker as active.
bool
SMCheckOnline::setActive()
{
    // TODO(Sean):
    // Need to disable token migration.  There can be a race with DLT information we currently
    // have.

    // Check if the checker is currently running or not.
    bool curState;
    curState = std::atomic_load(&SMChkActive);
    if (true == curState) {
        GLOGNOTIFY << "SM Checker is already running";
        return false;
    }

    // With current implementation, this is not possible, but check just in case if multiple
    // requests is received by this SM.
    bool oldState = false;
    if (!std::atomic_compare_exchange_strong(&SMChkActive, &oldState, true)) {
        GLOGNOTIFY << "SM Checker is already running";
        return false;
    }
    return true;
}

// set the state of the checker as inactive.
void
SMCheckOnline::setInactive()
{
    // right now, there is no reason to check the current state.  Just mark it as
    // inactive.
    std::atomic_store(&SMChkActive, false);
}

}  // namespace fds
