/*
 * Copyright 2014 Formation Data Systems, Inc. 
 */
#include <string>
#include <PerfTrace.h>
#include <object-store/ObjectMetadataStore.h>

namespace fds {

ObjectMetadataStore::ObjectMetadataStore(const std::string& modName)
        : Module(modName.c_str()),
          metaDb_(new ObjectMetadataDb()),
          metaCache(new ObjectMetaCache("SM Object Metadata Cache")) {
}

ObjectMetadataStore::~ObjectMetadataStore() {
    LOGDEBUG << "Destructing ObjectMetadataStore";
    metaDb_.reset();
}

int
ObjectMetadataStore::mod_init(SysParams const *const param) {
    static Module *metaStoreDepMods[] = {
        metaCache.get(),
        NULL
    };
    mod_intern = metaStoreDepMods;

    Module::mod_init(param);
    LOGDEBUG << "Done";
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
ObjectMetadataStore::openMetadataStore(const SmDiskMap::const_ptr& diskMap) {
    return metaDb_->openMetadataDb(diskMap);
}

ObjMetaData::const_ptr
ObjectMetadataStore::getObjectMetadata(fds_volid_t volId,
                                       const ObjectID& objId,
                                       Error &err) {
    // Check cache for metadata
    ObjMetaData::const_ptr objMeta
            = metaCache->getObjectMetadata(volId, objId, err);
    if (err.ok()) {
        PerfTracer::incr(SM_OBJ_METADATA_CACHE_HIT, volId, PerfTracer::perfNameStr(volId));
        LOGDEBUG << "Got " << objId << " metadata from cache";
        return objMeta;
    }

    // Read from metadata db. If the metadata is found, the
    // pointer will be allocated and set.
    objMeta = metaDb_->get(volId, objId, err);
    if (err.ok()) {
        LOGDEBUG << "Got metadata from db: Vol " << std::hex << volId << std::dec
                 << " " << objId << " refcnt: "<< objMeta->getRefCnt()
                 << " dataexists: " << objMeta->dataPhysicallyExists();
    } else {
        if (err == ERR_NOT_FOUND) {
            LOGDEBUG << "Metadata not found in db for obj " << objId;
        } else {
            LOGERROR << "Failed to get " << objId << " from metadata db: " << err;
        }
    }

    return objMeta;
}

Error
ObjectMetadataStore::putObjectMetadata(fds_volid_t volId,
                                       const ObjectID& objId,
                                       ObjMetaData::const_ptr objMeta) {
    Error err = metaDb_->put(volId, objId, objMeta);
    if (err.ok()) {
        LOGDEBUG << "Wrote " << objId << " metadata to db " << *objMeta;
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

    err = metaDb_->remove(volId, objId);
    if (err.ok()) {
        LOGDEBUG << "Removed " << objId << " from metadata db";
    } else {
        LOGERROR << "Failed to remove " << objId << " from metadata db";
    }

    return err;
}

void
ObjectMetadataStore::snapshot(fds_token_id smTokId,
                              SmIoSnapshotObjectDB::CbType notifFn) {
    Error err(ERR_OK);
    leveldb::DB *db;
    leveldb::ReadOptions options;
    metaDb_->snapshot(smTokId, db, options);
    notifFn(err, NULL, options, db);
}

}  // namespace fds
