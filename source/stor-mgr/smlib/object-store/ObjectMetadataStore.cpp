/*
 * Copyright 2014 Formation Data Systems, Inc. 
 */
#include <string>
#include <object-store/ObjectMetadataStore.h>

namespace fds {

ObjectMetadataStore::ObjectMetadataStore(const std::string& modName)
        : Module(modName.c_str()),
          metaDb_(new ObjectMetadataDb()) {
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

    return err;
}

Error
ObjectMetadataStore::putObjectMetadata(fds_volid_t volId,
                                       const ObjectID& objId,
                                       ObjMetaData::const_ptr objMeta) {
    Error err(ERR_OK);

    return err;
}

Error
ObjectMetadataStore::removeObjectMetadata(fds_volid_t volId,
                                          const ObjectID& objId) {
    Error err(ERR_OK);

    return err;
}

}  // namespace fds
