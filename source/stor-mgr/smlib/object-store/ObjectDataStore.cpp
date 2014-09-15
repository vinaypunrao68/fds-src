/*
 * Copyright 2014 Formation Data Systems, Inc. 
 */

#include <string>
#include <object-store/ObjectDataStore.h>

namespace fds {

ObjectDataStore::ObjectDataStore(const std::string &modName)
        : Module(modName.c_str()),
          diskMgr(&(diskio::DataIO::disk_singleton())) {
}

ObjectDataStore::~ObjectDataStore() {
}

Error
ObjectDataStore::putObjectData(fds_volid_t volId,
                               const ObjectID &objId,
                               boost::shared_ptr<const std::string> objData) {
    Error err(ERR_OK);

    // Construct persistent layer request
    meta_vol_io_t    vio;
    meta_obj_id_t    oid;
    fds_bool_t       sync = true;
    // TODO(Andrew): Actually select tier
    diskio::DataTier tier = diskio::diskTier;
    // TODO(Andrew): Should take a shared_ptr not a raw object buf
    ObjectBuf objBuf(*objData);
    SmPlReq *plReq =
            new SmPlReq(vio, oid,
                        const_cast<ObjectBuf *>(&objBuf),
                        sync, tier);

    err = diskMgr->disk_write(plReq);
    return err;
}

Error
ObjectDataStore::getObjectData(fds_volid_t volId,
                               const ObjectID &objId,
                               ObjMetaData::const_ptr objMetaData,
                               boost::shared_ptr<std::string> objData) {
    Error err(ERR_OK);

    // Construct persistent layer request
    meta_vol_io_t   vio;
    meta_obj_id_t   oid;
    fds_bool_t      sync = true;
    // TODO(Andrew): The persistent layer should read from whatever the
    // fastest tier is...This should request a tier
    diskio::DataTier tier = diskio::diskTier;
    ObjectBuf       objBuf;
    SmPlReq *plReq = new SmPlReq(vio,
                                 oid,
                                 static_cast<ObjectBuf *>(&objBuf),
                                 sync);
    plReq->setTier(tier);
    plReq->set_phy_loc(objMetaData->getObjPhyLoc(tier));
    objBuf.size = objMetaData->getObjSize();
    objBuf.data.resize(objBuf.size, 0);

    err = diskMgr->disk_read(plReq);

    // Copy the data to the give buffer
    // TODO(Andrew): Remove the ObjectBuf concept and just pass the
    // data pointer directly to the persistent layer so that this
    // copy can be avoided.
    *objData = objBuf.data;

    return err;
}

/**
 * Module initialization
 */
int
ObjectDataStore::mod_init(SysParams const *const p) {
    Module::mod_init(p);
    return 0;
}

/**
 * Module startup
 */
void
ObjectDataStore::mod_startup() {
}

/**
 * Module shutdown
 */
void
ObjectDataStore::mod_shutdown() {
}

}  // namespace fds
