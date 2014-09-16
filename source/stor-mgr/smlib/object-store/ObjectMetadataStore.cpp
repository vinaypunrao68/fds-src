/*
 * Copyright 2014 Formation Data Systems, Inc. 
 */
#include <string>
#include <object-store/ObjectMetadataStore.h>

namespace fds {

ObjectMetadataStore::ObjectMetadataStore(const std::string& modName,
                                         const std::string& obj_dir)
        : Module(modName.c_str()),
          metaDb_(new ObjectMetadataDb(obj_dir)),
          metaCache(new ObjectMetaCache("SM Object Metadata Cache")) {
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
                                       ObjMetaData::const_ptr* objMeta) {
    // Check cache for metadata
    Error err = metaCache->getObjectMetadata(volId, objId, *objMeta);
    if (err.ok()) {
        LOGDEBUG << "Got " << objId << " metadata from cache";
        return err;
    }
    // TODO(xxx) counters for obj meta DB accesses

    // Read from metadata db. If the metadata is found, the
    // pointer will be allocated and set.
    err = metaDb_->get(objId, objMeta);
    if (err.ok()) {
        LOGDEBUG << "Got " << objId << " metadata from db";
        LOGDEBUG << "Vol " << std::hex << volId<< std::dec
                 << " "<< objId << " refcnt: "<< (*objMeta)->getRefCnt()
                 << " dataexists: " << (*objMeta)->dataPhysicallyExists()
                 << " " << err;
    } else {
        LOGERROR << "Failed to get " << objId << " from metadata db: " << err;
    }

    return err;
}

Error
ObjectMetadataStore::putObjectMetadata(fds_volid_t volId,
                                       const ObjectID& objId,
                                       ObjMetaData::const_ptr objMeta) {
    // TODO(xxx) port counters

    Error err = metaDb_->put(objId, objMeta);
    if (err.ok()) {
        LOGDEBUG << "Wrote " << objId << " metadata to db";
        metaCache->putObjectMetadata(volId, objId, objMeta);
        LOGDEBUG << "Wrote " << objId << " metadata to cache";
    } else {
        LOGERROR << "Failed to write " << objId << " to metadata db: " << err;
    }

    return err;
}

Error
ObjectMetadataStore::removeObjectMetadata(fds_volid_t volId,
                                          const ObjectID& objId) {
    Error err(ERR_OK);
    LOGTRACE << "Vol " << std::hex << volId << std::dec << " " << objId;

    // Remove from metadata cache
    metaCache->removeObjectMetadata(volId, objId);
    // TODO(xxx) port counters

    err = metaDb_->remove(objId);
    if (err.ok()) {
        LOGDEBUG << "Removed " << objId << " from metadata db";
    } else {
        LOGERROR << "Failed to remove " << objId << " from metadata db";
    }

    return err;
}

}  // namespace fds
