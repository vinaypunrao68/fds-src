/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <PerfTrace.h>
#include <object-store/ObjectMetadataStore.h>
#include <fds_process.h>

namespace fds {

ObjectMetadataStore::ObjectMetadataStore(const std::string& modName,
                                         UpdateMediaTrackerFnObj fn)
        : Module(modName.c_str()),
          metaDb_(new ObjectMetadataDb(std::move(fn))),
          metaCache(new ObjectMetaCache("SM Object Metadata Cache")),
          currentState(METADATA_STORE_INITING) {
}

ObjectMetadataStore::~ObjectMetadataStore() {
    metaDb_.reset();
    metaCache.reset();
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
    Module::mod_startup();
}

void
ObjectMetadataStore::mod_shutdown() {
    metaDb_->closeMetadataDb();
    Module::mod_shutdown();
    LOGDEBUG << "Done.";
}

void
ObjectMetadataStore::setNumBitsPerToken(fds_uint32_t nbits) {
    metaDb_->setNumBitsPerToken(nbits);
}

Error
ObjectMetadataStore::openMetadataStore(SmDiskMap::ptr& diskMap) {
    Error err(ERR_OK);
    err = metaDb_->openMetadataDb(diskMap);
    if (err.ok()) {
        currentState = METADATA_STORE_INITED;
    }
    return err;
}

Error
ObjectMetadataStore::openMetadataStore(SmDiskMap::ptr& diskMap,
                                       const SmTokenSet& smToks) {
    return metaDb_->openMetadataDb(diskMap, smToks);
}

Error
ObjectMetadataStore::closeAndDeleteMetadataDbs(const SmTokenSet& smTokensLost) {
    return metaDb_->closeAndDeleteMetadataDbs(smTokensLost);
}

Error
ObjectMetadataStore::closeAndDeleteMetadataDb(const fds_token_id& smTokenLost) {
    return metaDb_->closeAndDeleteMetadataDb(smTokenLost);
}

Error
ObjectMetadataStore::deleteMetadataDb(const std::string& diskPath,
                                      const fds_token_id& smTokenLost) {
    return metaDb_->deleteMetadataDb(diskPath, smTokenLost);
}

ObjMetaData::const_ptr
ObjectMetadataStore::getObjectMetadata(fds_volid_t volId,
                                       const ObjectID& objId,
                                       Error &err) {
    // Check cache for metadata
    ObjMetaData::const_ptr objMeta
            = metaCache->getObjectMetadata(volId, objId, err);
    if (err.ok()) {
        PerfTracer::incr(PerfEventType::SM_OBJ_METADATA_CACHE_HIT, volId);
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

std::vector<ObjectID>
ObjectMetadataStore::getMetaDbKeys(const fds_token_id &smToken) {
   return metaDb_->getKeys(smToken); 
}

diskio::DataTier
ObjectMetadataStore::getMetadataTier() const {
    return metaDb_->getMetaTierInfo();
}

void
ObjectMetadataStore::snapshot(fds_token_id smTokId,
                              SmIoSnapshotObjectDB::CbType notifFn,
                              SmIoSnapshotObjectDB* snapReq) {
    Error err(ERR_OK);
    std::shared_ptr<leveldb::DB> db;
    leveldb::ReadOptions options;

    err = metaDb_->snapshot(smTokId, db, options);
    notifFn(err, snapReq, options, db, snapReq->retryReq, snapReq->unique_id);
}

void
ObjectMetadataStore::snapshot(fds_token_id smTokId,
                              SmIoSnapshotObjectDB::CbTypePersist notifFn,
                              SmIoSnapshotObjectDB* snapReq) {
    Error err(ERR_OK);
    std::string snapDir;
    leveldb::CopyEnv *env = nullptr;
    /**
     * Snapshots will be stored in
     * /fds/user-repo/sm-snapshots/<token_id>_<dlt_vers>_executorID_<snapshot# of SM token>
     */
    snapDir = g_fdsprocess->proc_fdsroot()->dir_fdsroot() + "user-repo/sm-snapshots/";
    FdsRootDir::fds_mkdir(snapDir.c_str());
    snapDir += boost::lexical_cast<std::string>(smTokId) +
               "_" +
               boost::lexical_cast<std::string>(snapReq->targetDltVersion) +
               "_" +
               boost::lexical_cast<std::string>(snapReq->executorId) +
               "_" +
               snapReq->snapNum;

    LOGDEBUG << "snapshot location " << snapDir << " snapNum" << snapReq->snapNum;
    err = metaDb_->snapshot(smTokId, snapDir, &env);
    notifFn(err, snapReq, snapDir, env);
}

}  // namespace fds
