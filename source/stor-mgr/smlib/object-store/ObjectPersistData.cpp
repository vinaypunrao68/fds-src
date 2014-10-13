/*
 * Copyright 2014 Formation Data Systems, Inc. 
 */

#include <string>
#include <PerfTrace.h>
#include <object-store/ObjectPersistData.h>

namespace fds {

ObjectPersistData::ObjectPersistData(const std::string &modName,
                                     SmIoReqHandler *data_store)
        : Module(modName.c_str()),
          scavenger(new ScavControl("SM Disk Scavenger", data_store, this)) {
}

ObjectPersistData::~ObjectPersistData() {
    diskio::FilePersisDataIO* delFdesc = NULL;
    for (TokFileMap::iterator it = tokFileTbl.begin();
         it != tokFileTbl.end();
         ++it) {
        delFdesc = it->second;
        if (delFdesc) {
            // file is closed in FilePersisDataIO destructor
            delete delFdesc;
            it->second = NULL;
        }
    }
}

Error
ObjectPersistData::openPersistDataStore(const SmDiskMap::const_ptr& diskMap) {
    Error err(ERR_OK);
    smDiskMap = diskMap;
    LOGDEBUG << "Will open token data files";

    SmTokenSet smToks = diskMap->getSmTokens();
    for (SmTokenSet::const_iterator cit = smToks.cbegin();
         cit != smToks.cend();
         ++cit) {
        // TODO(Anna) for now opening on disk tier only -- revisit when porting
        // back tiering
        // TODO(Anna) we are not opening files we are going to write to, which is ok
        // if SM always comes up from clean state and no GC. When porting back GC, will need to
        // open all files for an SM token. We will know from GC persistent state

        // so.. set file to 'initial file id', have to revisit right away
        // and make it a file id where are we writing to
        fds_uint64_t wkey = getWriteFileIdKey(diskio::diskTier, *cit);
        fds_uint16_t fileId = SM_INIT_FILE_ID;
        write_synchronized(mapLock) {
            fds_verify(writeFileIdMap.count(wkey) == 0);
            writeFileIdMap[wkey] = fileId;
        }

        // actually open SM token file
        err = openTokenFile(diskio::diskTier, *cit, fileId);
        if (!err.ok()) {
            LOGERROR << "Failed to open File for SM token " << *cit << " " << err;
            break;
        }
    }

    // we enable scavenger by default
    // TODO(Anna) need a bit more testing before enabling GC
    /*
    err = scavenger->enableScavenger(diskMap);
    if (!err.ok()) {
        LOGERROR << "Failed to start Scavenger " << err;
    }
    */
    return err;
}

Error
ObjectPersistData::writeObjectData(const ObjectID& objId,
                                   diskio::DiskRequest* req) {
    fds_token_id smTokId = smDiskMap->smTokenId(objId);
    fds_uint16_t fileId = getWriteFileId(req->getTier(), smTokId);
    diskio::PersisDataIO *iop = getTokenFile(req->getTier(),
                                             smTokId,
                                             fileId);

    fds_verify(iop);
    return iop->disk_write(req);
}

Error
ObjectPersistData::readObjectData(const ObjectID& objId,
                                  diskio::DiskRequest* req) {
    obj_phy_loc_t *loc = req->req_get_phy_loc();
    fds_token_id smTokId = smDiskMap->smTokenId(objId);
    fds_uint16_t fileId = loc->obj_file_id;
    diskio::PersisDataIO *iop = getTokenFile(req->getTier(),
                                             smTokId,
                                             fileId);
    fds_verify(iop);
    return iop->disk_read(req);
}

void
ObjectPersistData::notifyDataDeleted(const ObjectID& objId,
                                     fds_uint32_t objSize,
                                     const obj_phy_loc_t* loc) {
    fds_token_id smTokId = smDiskMap->smTokenId(objId);
    fds_uint16_t fileId = loc->obj_file_id;
    diskio::PersisDataIO *iop = getTokenFile((diskio::DataTier)loc->obj_tier,
                                             smTokId,
                                             fileId);

    iop->disk_delete(objSize);
}

void
ObjectPersistData::notifyStartGc(fds_token_id smTokId,
                                 diskio::DataTier tier) {
    Error err(ERR_OK);
    fds_uint16_t curFileId = SM_INVALID_FILE_ID;
    fds_uint16_t newFileId = SM_INVALID_FILE_ID;
    fds_uint64_t wkey = getWriteFileIdKey(tier, smTokId);

    read_synchronized(mapLock) {
        fds_verify(writeFileIdMap.count(wkey) > 0);
        curFileId = writeFileIdMap[wkey];
    }

    // calculate new file id (file we will start writing
    // and to which non-garbage data will be copied)
    // but do not set until we open token file
    newFileId = getShadowFileId(curFileId);

    // we shouldn't have a file with newFileId open; if it is open,
    // most likely we did not finish last GC properly
    // openTokenFile() method will verify this
    // open file we are going to write to
    err = openTokenFile(tier, smTokId, newFileId);
    fds_verify(err.ok());

    // set new file id
    write_synchronized(mapLock) {
        fds_verify(writeFileIdMap.count(wkey) > 0);
        writeFileIdMap[wkey] = newFileId;
    }
}

Error
ObjectPersistData::notifyEndGc(fds_token_id smTokId,
                               diskio::DataTier tier) {
    Error err(ERR_OK);
    fds_uint64_t wkey = getWriteFileIdKey(tier, smTokId);
    fds_uint16_t shadowFileId = SM_INVALID_FILE_ID;
    fds_uint16_t oldFileId = SM_INVALID_FILE_ID;
    diskio::FilePersisDataIO *delFdesc = NULL;

    SCOPEDWRITE(mapLock);
    fds_verify(writeFileIdMap.count(wkey) > 0);
    shadowFileId = writeFileIdMap[wkey];

    // old file id is the same as next shadow file id
    oldFileId = getShadowFileId(shadowFileId);

    // remove the token file with old file id (garbage collect)
    // it is up to the TokenCompactor to check that it copied all
    // non-garbage data to the shadow file
    // TODO(Anna) update below code when supporting multiple files
    fds_uint64_t fkey = getFileKey(tier, smTokId, oldFileId);
    fds_verify(tokFileTbl.count(fkey) > 0);
    delFdesc = tokFileTbl[fkey];
    if (delFdesc) {
        err = delFdesc->delete_file();
        delete delFdesc;
        delFdesc = NULL;
    }
    tokFileTbl.erase(fkey);
    return err;
}

fds_bool_t
ObjectPersistData::isShadowLocation(obj_phy_loc_t* loc,
                                    fds_token_id smTokId) {
    fds_uint16_t curFileId = getWriteFileId(static_cast<diskio::DataTier>(loc->obj_tier),
                                            smTokId);
    return (curFileId == loc->obj_file_id);
}

Error
ObjectPersistData::openTokenFile(diskio::DataTier tier,
                                 fds_token_id smTokId,
                                 fds_uint16_t fileId) {
    Error err(ERR_OK);
    diskio::FilePersisDataIO *fdesc = NULL;
    fds_uint16_t diskId = smDiskMap->getDiskId(smTokId, tier);

    // this method expects explicit file ids
    fds_verify(fileId != SM_WRITE_FILE_ID);
    fds_verify(fileId != SM_INVALID_FILE_ID);

    // check if we got a valid disk ID
    if (!ObjectLocationTable::isDiskIdValid(diskId)) {
        // means there are no disks on this tier
        LOGWARN << "Couldn't open SM token file for SM token "
                << smTokId << " tier " << tier << ", most likely no disks on requested tier";
        return ERR_NOT_FOUND;  // TODO(Anna) do we want separate error?
    }

    // if we are here, params are valid
    fds_uint64_t fkey = getFileKey(tier, smTokId, fileId);  // key to actual persist io datastruct
    std::string filename = smDiskMap->getDiskPath(smTokId, tier) + "/tokenFile_"
            + std::to_string(smTokId) + "_" + std::to_string(fileId);

    SCOPEDWRITE(mapLock);
    fds_verify(tokFileTbl.count(fkey) == 0);
    fdesc = new(std::nothrow) diskio::FilePersisDataIO(filename.c_str(), fileId, diskId);
    if (!fdesc) {
        LOGERROR << "Failed to create FilePersisDataIO for " << filename;
        return ERR_OUT_OF_MEMORY;
    }
    tokFileTbl[fkey] = fdesc;
    return err;
}

void
ObjectPersistData::closeTokenFile(diskio::DataTier tier,
                                  fds_token_id smTokId,
                                  fds_uint16_t fileId) {
    diskio::FilePersisDataIO *delFdesc = NULL;
    fds_uint64_t fkey = getFileKey(tier, smTokId, fileId);

    // if need to close current file ID, must specify file id
    // explicitly, instead of using SM_WRITE_FILE_ID
    fds_verify(fileId != SM_WRITE_FILE_ID);
    fds_verify(fileId != SM_INVALID_FILE_ID);

    SCOPEDWRITE(mapLock);
    if (tokFileTbl.count(fkey) > 0) {
        delFdesc = tokFileTbl[fkey];
        if (delFdesc) {
            delete delFdesc;
            delFdesc = NULL;
        }
        tokFileTbl.erase(fkey);
    }
}

diskio::FilePersisDataIO*
ObjectPersistData::getTokenFile(diskio::DataTier tier,
                                fds_token_id smTokId,
                                fds_uint16_t fileId) {
    diskio::FilePersisDataIO *fdesc = NULL;
    fds_uint64_t fkey = getFileKey(tier, smTokId, fileId);

    SCOPEDREAD(mapLock);
    fds_verify(tokFileTbl.count(fkey) > 0);
    return tokFileTbl[fkey];
}

fds_uint16_t
ObjectPersistData::getWriteFileId(diskio::DataTier tier,
                                  fds_token_id smTokId) {
    fds_uint64_t key = getWriteFileIdKey(tier, smTokId);
    SCOPEDREAD(mapLock);
    fds_verify(writeFileIdMap.count(key) > 0);
    return writeFileIdMap[key];
}

void
ObjectPersistData::getSmTokenStats(fds_token_id smTokId,
                                   diskio::DataTier tier,
                                   diskio::TokenStat* retStat) {
    fds_uint16_t fileId = getWriteFileId(tier, smTokId);
    diskio::FilePersisDataIO *fdesc = getTokenFile(tier, smTokId, fileId);
    fds_verify(fdesc);

    // TODO(Anna) if we do multiple files per SM token, this method should
    // change; current assumption there is only one file per SM token
    // (or two we are garbage collecting)

    // fill in token stat that we return
    (*retStat).tkn_id = smTokId;
    (*retStat).tkn_tot_size = fdesc->get_total_bytes();
    (*retStat).tkn_reclaim_size = fdesc->get_deleted_bytes();
}

/**
 * Module initialization
 */
int
ObjectPersistData::mod_init(SysParams const *const p) {
    static Module *objPersistDepMods[] = {
        scavenger.get(),
        NULL
    };
    mod_intern = objPersistDepMods;

    Module::mod_init(p);
    return 0;
}

/**
 * Module startup
 */
void
ObjectPersistData::mod_startup() {
}

/**
 * Module shutdown
 */
void
ObjectPersistData::mod_shutdown() {
}

}  // namespace fds
