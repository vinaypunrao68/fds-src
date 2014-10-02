/*
 * Copyright 2014 Formation Data Systems, Inc. 
 */

#include <string>
#include <PerfTrace.h>
#include <object-store/ObjectPersistData.h>

namespace fds {

ObjectPersistData::ObjectPersistData(const std::string &modName)
        : Module(modName.c_str()),
          tokFileContainer(new diskio::tokenFileDb()) {
}

ObjectPersistData::~ObjectPersistData() {
}

Error
ObjectPersistData::openPersistDataStore(const SmDiskMap::const_ptr& diskMap) {
    Error err(ERR_OK);
    smDiskMap = diskMap;
    // TODO(Anna) we need to revisit tokenFileDb class; we should open
    // all files we need here and do not try to open on data path
    // also need to remove dependency tokenFileDb class on dataDiscoveryMod
    // TODO(ANNA) open data files on both tiers here!

    LOGDEBUG << "Will open token data files";
    return err;
}

Error
ObjectPersistData::writeObjectData(const ObjectID& objId,
                                   diskio::DiskRequest* req) {
    fds_token_id smTokId = smDiskMap->smTokenId(objId);
    fds_uint32_t diskId = smDiskMap->getDiskId(objId, req->getTier());
    diskio::PersisDataIO *iop = tokFileContainer->openTokenFile(req->getTier(),
                                                        diskId,
                                                        smTokId,
                                                        WRITE_FILE_ID);

    fds_verify(iop);
    return iop->disk_write(req);
}

Error
ObjectPersistData::readObjectData(const ObjectID& objId,
                                  diskio::DiskRequest* req) {
    diskio::PersisDataIO  *iop = NULL;
    obj_phy_loc_t *loc = req->req_get_phy_loc();
    fds_token_id smTokId = smDiskMap->smTokenId(objId);

    iop = tokFileContainer->openTokenFile(req->getTier(),
                                          loc->obj_stor_loc_id,
                                          smTokId,
                                          loc->obj_file_id);
    fds_verify(iop);
    return iop->disk_read(req);
}

void
ObjectPersistData::notifyDataDeleted(const ObjectID& objId,
                                     fds_uint32_t objSize,
                                     const obj_phy_loc_t* loc) {
    diskio::PersisDataIO *iop = NULL;
    fds_token_id smTokId = smDiskMap->smTokenId(objId);
    iop = tokFileContainer->openTokenFile((diskio::DataTier)loc->obj_tier,
                                          loc->obj_stor_loc_id,
                                          smTokId,
                                          loc->obj_file_id);
    iop->disk_delete(objSize);
}

void
ObjectPersistData::notifyStartGc(fds_token_id smTokId,
                                 fds_uint16_t diskId,
                                 diskio::DataTier tier) {
    Error err = tokFileContainer->notifyStartGC(diskId, smTokId, tier);
    fds_verify(err.ok());
}

Error
ObjectPersistData::notifyEndGc(fds_token_id smTokId,
                               fds_uint16_t diskId,
                               diskio::DataTier tier) {
    Error err = tokFileContainer->notifyEndGC(diskId, smTokId, tier);
    fds_verify(err.ok());  // until we know how to handle those errors
    return err;
}

fds_bool_t
ObjectPersistData::isShadowLocation(obj_phy_loc_t* loc,
                                    fds_token_id smTokId) {
    return tokFileContainer->isShadowFileId(loc->obj_file_id,
                                       loc->obj_stor_loc_id,
                                       smTokId,
                                       static_cast<diskio::DataTier>(loc->obj_tier));
}

/**
 * Module initialization
 */
int
ObjectPersistData::mod_init(SysParams const *const p) {
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
