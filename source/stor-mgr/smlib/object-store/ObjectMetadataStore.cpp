/*
 * Copyright 2014 Formation Data Systems, Inc. 
 */
#include <string>
#include <object-store/ObjectMetadataStore.h>

namespace fds {

ObjectMetadataStore::ObjectMetadataStore(const std::string& modName,
                                         const std::string& obj_dir)
        : Module(modName.c_str()),
          metaDb_(new ObjectMetadataDb(obj_dir)) {
}

ObjectMetadataStore::~ObjectMetadataStore() {
}

int
ObjectMetadataStore::mod_init(SysParams const *const param) {
    Module::mod_init(param);
    return 0;
}

void
ObjectMetadataStore::mod_startup() {
}

void
ObjectMetadataStore::mod_shutdown() {
}

void
ObjectMetadataStore::setNumBitsPerToken(fds_uint32_t nbits) {
    metaDb_->setNumBitsPerToken(nbits);
}

Error
ObjectMetadataStore::getObjectMetadata(fds_volid_t volId,
                                       const ObjectID& objId,
                                       ObjMetaData::ptr objMeta) {
    Error err(ERR_OK);

    // TODO(xxx) meta cache
    // TODO(xxx) counters for obj meta DB accesses

    err = metaDb_->get(objId, objMeta);
    LOGDEBUG << "Vol " << std::hex << volId<< std::dec
             << " "<< objId << " refcnt: "<< objMeta->getRefCnt()
             << " dataexists: " << objMeta->dataPhysicallyExists()
             << " " << err;

    return err;
}

Error
ObjectMetadataStore::putObjectMetadata(fds_volid_t volId,
                                       const ObjectID& objId,
                                       ObjMetaData::const_ptr objMeta) {
    Error err(ERR_OK);
    LOGTRACE << "Vol " << std::hex << volId << std::dec << " " << objId
             << " " << *objMeta;

    // TODO(xxx) meta cache
    // TODO(xxx) port counters

    err = metaDb_->put(objId, objMeta);

    return err;
}

Error
ObjectMetadataStore::removeObjectMetadata(fds_volid_t volId,
                                          const ObjectID& objId) {
    Error err(ERR_OK);
    LOGTRACE << "Vol " << std::hex << volId << std::dec << " " << objId;

    // TODO(xxx) meta cache
    // TODO(xxx) port counters

    err = metaDb_->remove(objId);

    return err;
}

}  // namespace fds
