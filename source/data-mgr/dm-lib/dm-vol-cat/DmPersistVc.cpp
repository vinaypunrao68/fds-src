/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <linux/limits.h>
#include <libgen.h>

#include <string>
#include <vector>
#include <fds_process.h>
#include <net/net-service.h>
#include <dm-platform.h>
#include <lib/Catalog.h>
#include <dm-vol-cat/DmPersistVc.h>

namespace fds {

/**
 * Struct that hold per-volume catalog and configuration
 * Used by Volume Catalog Persistent Layer (DmPersistVolCatalog) only
 * This class is threads safe (note that leveldb operations are
 * atomic);
 */
class PersistVolumeMeta {
  public:
    static const std::string CATALOG_WRITE_BUFFER_SIZE_STR;
    static const std::string CATALOG_CACHE_SIZE_STR;
    static const std::string CATALOG_MAX_LOG_FILES_STR;

    /**
     * Does not initialize catalog yet
     */
    PersistVolumeMeta(fds_volid_t volid,
                      fds_uint32_t max_obj_size,
                      fds_uint32_t extent0_objs,
                      fds_uint32_t extent_objs,
                      fds_volid_t srcVolId = invalid_vol_id,
                      fds_bool_t snapshot = false,
                      fds_bool_t readOnly = false);
    ~PersistVolumeMeta();

    /**
     * Initializes and opens persistent catalog
     */
    Error init();

    inline fds_bool_t isInitialized() const {
        return (catalog_ != NULL);
    }
    inline fds_volid_t volumeId() const {
        return volume_id;
    }
    inline fds_uint32_t maxObjSizeBytes() const {
        return max_obj_size_bytes;
    }
    inline const std::string volumeIdStr() const {
        return std::to_string(volume_id);
    }
    inline fds_uint32_t extent0ObjEntries() const {
        return extent0_obj_entries;
    }
    inline fds_uint32_t extentObjEntries() const {
        return extent_obj_entries;
    }
    inline fds_bool_t isSnapshot() const {
        return snapshot_;
    }
    inline fds_bool_t isReadOnly() const {
        return readOnly_;
    }
    inline Error copyDB(const std::string& fileName) {
        return catalog_->DbSnap(fileName);
    }

    /**
     * Currently expects offset_bytes to be aligned to max object size
     */
    inline fds_extent_id getExtentId(fds_uint64_t offset_bytes) const {
        fds_uint32_t off_units = offset_bytes / max_obj_size_bytes;
        fds_verify((offset_bytes % max_obj_size_bytes) == 0);
        if (off_units < extent0_obj_entries) return 0;
        off_units -= extent0_obj_entries;
        return ((off_units/extent_obj_entries) + 1);
    }

    /**
     * Returns first offset of extent with id 'extent_id' in units of
     * max object size in bytes
     */
    inline fds_uint32_t firstOffset(fds_extent_id extent_id) const {
        if (extent_id == 0) return 0;
        return (extent0_obj_entries + (extent_id - 1) * extent_obj_entries);
    }

    /**
     * Returns number of offset to object info entries in extent 'extent_id'
     */
    inline fds_uint32_t numObjEntries(fds_extent_id extent_id) const {
        if (extent_id == 0) return extent0_obj_entries;
        return extent_obj_entries;
    }

    /**
     * Updates entry with key 'key' in the Db
     */
    inline Error updateEntry(const std::string& key,
                             const std::string& value) {
        SCOPEDREAD(snap_lock_);
        if (readOnly_) return ERR_DM_OP_NOT_ALLOWED;
        if (marked_deleted) return ERR_DM_VOL_MARKED_DELETED;
        return catalog_->Update(key, value);
    }

    /**
     * Atomically applies a set of updates in 'batch'
     */
    inline Error updateBatch(CatWriteBatch* batch) {
        SCOPEDREAD(snap_lock_);
        if (readOnly_) return ERR_DM_OP_NOT_ALLOWED;
        if (marked_deleted) return ERR_DM_VOL_MARKED_DELETED;
        return catalog_->Update(batch);
    }

    /**
     * Queries DB for entry with key 'key'
     */
    inline Error queryEntry(const std::string& key,
                            std::string* value) {
        return catalog_->Query(key, value);
    }

    /**
     * Deletes entry with key 'key' from the DB
     */
    inline Error deleteEntry(const std::string& key) {
        SCOPEDREAD(snap_lock_);
        if (readOnly_) return ERR_DM_OP_NOT_ALLOWED;
        return catalog_->Delete(key);
    }

    inline Catalog::catalog_iterator_t*
    getSnapshotIter(Catalog::catalog_roptions_t& opts) {
        catalog_->GetSnapshot(opts);
        return catalog_->NewIterator(opts);
    }

    inline void
    releaseSnapshotIter(Catalog::catalog_roptions_t opts,
                        Catalog::catalog_iterator_t* it) {
        delete it;
        catalog_->ReleaseSnapshot(opts);
    }

    /**
     * Mark as deleted if catalog does not contain any valid blobs
     */
    Error markDeleted();

    inline fds_bool_t isMarkedDeleted() {
        SCOPEDREAD(snap_lock_);
        return marked_deleted;
    }

    /**
     * Sync the catalog to the DM with uuid 'dm_uuid'
     */
    Error syncToDM(NodeUuid dm_uuid);

  private:
    /**
     * Configurable parameters
     */
    fds_volid_t volume_id;
    fds_uint32_t max_obj_size_bytes;

    /**
     * Number of offset to object info entries in extent 0
     * and all other extents
     */
    fds_uint32_t extent0_obj_entries;
    fds_uint32_t extent_obj_entries;

    /**
     * Catalog that stores volume's extents
     */
    Catalog *catalog_;

    // as the configuration will not be refreshed frequently, we can read it without lock
    FdsConfigAccessor config_helper_;

    /**
     * Protects from writing to the catalog while taking
     * a snapshot for rsync (cp -r kind of snapshot)
     */
    fds_rwlock snap_lock_;

    // if true, updates to catalog will not be allowed, and
    // will return ERR_DM_VOL_MARKED_DELETED
    // also protected by snap_lock_
    fds_bool_t marked_deleted;

    // catalog is for snapshot
    const fds_bool_t snapshot_;
    const fds_bool_t readOnly_;

    // parent volId for snapshot
    fds_volid_t srcVolId_;
};

const std::string PersistVolumeMeta::CATALOG_WRITE_BUFFER_SIZE_STR("catalog_write_buffer_size");
const std::string PersistVolumeMeta::CATALOG_CACHE_SIZE_STR("catalog_cache_size");
const std::string PersistVolumeMeta::CATALOG_MAX_LOG_FILES_STR("catalog_max_log_files");

//
// does not initialize catalog yet
//
PersistVolumeMeta::PersistVolumeMeta(fds_volid_t volId,
                                     fds_uint32_t max_obj_size,
                                     fds_uint32_t extent0_objs,
                                     fds_uint32_t extent_objs,
                                     fds_volid_t srcVolId /* = invalid_vol_id */,
                                     fds_bool_t snapshot /* = false */,
                                     fds_bool_t readOnly /* = false */)
        : volume_id(volId),
          max_obj_size_bytes(max_obj_size),
          extent0_obj_entries(extent0_objs),
          extent_obj_entries(extent_objs),
          catalog_(NULL),
          marked_deleted(false),
          config_helper_(g_fdsprocess->get_conf_helper()),
          snapshot_(snapshot),
          readOnly_(readOnly),
          srcVolId_(srcVolId) {
    if (invalid_vol_id == srcVolId_) {
        srcVolId_ = volume_id;
    }
    fds_verify(extent0_obj_entries > 0);
    fds_verify(extent_obj_entries > 0);
    if (snapshot) {
        fds_verify(readOnly);
    }
}

PersistVolumeMeta::~PersistVolumeMeta() {
    if (catalog_) {
        delete catalog_;
    }
    // if marked as deleted, actually delete the whole leveldb dir
    if (marked_deleted) {
        const FdsRootDir* root = g_fdsprocess->proc_fdsroot();
        const std::string loc_src_db = root->dir_user_repo_dm() + std::to_string(srcVolId_) +
                (readOnly_ ? "/snapshot/" : "/") + volumeIdStr() + "_vcat.ldb";
        const std::string rm_cmd = "rm -rf  " + loc_src_db;
        int retcode = std::system((const char *)rm_cmd.c_str());
        LOGNOTIFY << "Removed leveldb dir, retcode " << retcode;
    }
}

Error PersistVolumeMeta::init() {
    Error err(ERR_OK);
    const FdsRootDir* root = g_fdsprocess->proc_fdsroot();
    std::string catName(root->dir_user_repo_dm());
    if (!snapshot_ && srcVolId_ == invalid_vol_id) {
        // volume
        catName += volumeIdStr();
        catName += "/" + volumeIdStr() + "_vcat.ldb";
    } else if (srcVolId_ > invalid_vol_id && !snapshot_) {
        // clone
        catName += volumeIdStr();
        catName += "/" + volumeIdStr() + "_vcat.ldb";
    } else {
        // snapshot
        catName += std::to_string(srcVolId_) + "/snapshot";
        catName += "/" + volumeIdStr() + "_vcat.ldb";
    }

    LOGNOTIFY << " Meta Init  '" << catName << "'";
    FdsRootDir::fds_mkdir(catName.c_str());

    fds_uint32_t writeBufferSize = config_helper_.get<fds_uint32_t>(CATALOG_WRITE_BUFFER_SIZE_STR,
            Catalog::WRITE_BUFFER_SIZE);
    fds_uint32_t cacheSize = config_helper_.get<fds_uint32_t>(CATALOG_CACHE_SIZE_STR,
            Catalog::CACHE_SIZE);
    fds_uint32_t maxLogFiles = config_helper_.get<fds_uint32_t>(CATALOG_MAX_LOG_FILES_STR, 5);

    std::string logDirName = snapshot_ ? "" : root->dir_user_repo_dm() + volumeIdStr() + "/";
    std::string logFilePrefix(snapshot_ ? "" : "catalog.journal");

    catalog_ = new(std::nothrow) Catalog(catName, writeBufferSize, cacheSize, logDirName,
            logFilePrefix, maxLogFiles);
    if (!catalog_) {
        LOGERROR << "Failed to create catalog for volume "
                 << std::hex << volume_id << std::dec;
        return ERR_OUT_OF_MEMORY;
    }
    return err;
}

//
// Mark volume as deleted if there are not valid blobs
// may be called when there are concurrent updates to the catalog
//
Error PersistVolumeMeta::markDeleted() {
    Error err(ERR_OK);
    Catalog::catalog_roptions_t options;
    Catalog::catalog_iterator_t* dbIt;

    SCOPEDWRITE(snap_lock_);

    // it's possible that catalog not open yet, if we just starting
    // to push meta for this volume from another DM -- in that
    // case return ERR_OK (== volume is empty), if it's not empty on
    // other DMs, they will return false
    if (!isInitialized()) {
        LOGNOTIFY << "Catalog not open for volume " << std::hex
                  << volume_id << std::dec << " marking as deleted";
        marked_deleted = true;
        return err;
    } else if (readOnly_) {
        LOGNOTIFY << "Delete request for snapshot " << std::hex << volume_id << std::dec
                << ", marking as deleted";
        marked_deleted = true;
        return err;
    }

    /**
     * We will now delete volumes even if they are not empty
     *     
    dbIt = getSnapshotIter(options);
    for (dbIt->SeekToFirst(); dbIt->Valid(); dbIt->Next()) {
        Record db_key = dbIt->key();
        ExtentKey extent_key;
        fds_verify(extent_key.loadSerialized(dbIt->key().ToString()) == ERR_OK);
        // ignore non-0 extents
        if (extent_key.extent_id > 0) {
            LOGTRACE << "Will ignore non-0 extent " << extent_key << " vol "
                     << std::hex << volume_id << std::dec;
            continue;
        }

        BlobExtent0Desc extdesc;
        fds_verify(extdesc.loadSerialized(dbIt->value().ToString()) == ERR_OK);
        LOGDEBUG << "Found extent 0" << extent_key << " vol "
                 << std::hex << volume_id << std::dec << " " << extdesc;
        if (extdesc.version != blob_version_deleted) {
            err = ERR_VOL_NOT_EMPTY;
            break;
        }
    }
    releaseSnapshotIter(options, dbIt);
    */

    // if volume is empty, mark as deleted
    if (err.ok()) {
        marked_deleted = true;
    }

    return err;
}

//
// RSync catalog to DM 'dm_uuid'
//
Error PersistVolumeMeta::syncToDM(NodeUuid dm_uuid) {
    Error err(ERR_OK);
    fds_uint64_t start_t, end_t;
    int retcode = 0;
    const FdsRootDir* root = g_fdsprocess->proc_fdsroot();
    const std::string loc_src_db = root->dir_user_repo_dm() + std::to_string(srcVolId_) +
                (readOnly_ ? "/snapshot/" : "/") + volumeIdStr() + "_vcat.ldb";
    const std::string loc_snap_dir = root->dir_user_repo_snap()
            + std::to_string(dm_uuid.uuid_get_val()) + std::string("/");
    const std::string loc_snap_db = loc_snap_dir + volumeIdStr() + "_vcat.ldb";
    NodeAgent::pointer node = Platform::plf_dm_nodes()->agent_info(dm_uuid);
    DmAgent::pointer dm = agt_cast_ptr<DmAgent>(node);
    const std::string dst_node = dm->get_node_root() + "user-repo/dm-names/";
    std::string dst_ip;

    // make dir if it does not exist yet
    auto sret = std::system((const char *)("mkdir -p "+loc_snap_dir+" ").c_str());

    if (NetMgr::ep_mgr_singleton()->ep_uuid_binding(dm_uuid.toSvcUuid(), 0, 0, &dst_ip) < 0) {
        LOGERROR << "Failed to sync catalog: Failed to get IP address for destination DM "
                 << std::hex << dm_uuid.uuid_get_val() << std::dec;
        return ERR_NOT_FOUND;
    }

    LOGDEBUG << "Will Sync volume catalog of volume " << volume_id
             << " to DM " << std::hex << dm_uuid.uuid_get_val() << std::dec;

    const std::string copy_cmd = "cp -r "+ loc_src_db + "*  " + loc_snap_dir + " ";
    const std::string rm_cmd = "rm -rf  " + loc_snap_db;
    const std::string rsync_cmd = "sshpass -p passwd rsync -r "
            + loc_snap_db + "  root@" + dst_ip + ":" + dst_node + "";

    LOGTRACE << " rsync: local copy  " << copy_cmd;
    LOGTRACE << " rsync:  " << rsync_cmd;

    write_synchronized(snap_lock_) {
        retcode = std::system((const char *)rm_cmd.c_str());
        if (retcode == 0) {
           //  retcode = std::system((const char *)copy_cmd.c_str());
            Error err   = copyDB(loc_snap_dir);
        }
    }

    if (retcode != 0) {
        LOGERROR << "Copy command failed: " << copy_cmd << " ; code " << retcode;
        return ERR_DM_RSYNC_FAILED;
    }

    start_t = fds::util::rdtsc();
    retcode = std::system((const char *)rsync_cmd.c_str());
    end_t = fds::util::rdtsc();
    if ((end_t - start_t) > 0) {
        LOGDEBUG << "Rsync time: " << ((end_t - start_t) / fds::util::getClockTicks());
    }

    if (retcode != 0) {
        LOGERROR << "Rsync command failed: " << rsync_cmd << " ; code " << retcode;
        err = ERR_DM_RSYNC_FAILED;
    }

    return err;
}

//----------- DmPersistVolCatalog implementation -------------//

DmPersistVolCatalog::DmPersistVolCatalog(char const *const name)
        : Module(name) {
}

DmPersistVolCatalog::~DmPersistVolCatalog() {
}

int DmPersistVolCatalog::mod_init(SysParams const *const param)
{
    Module::mod_init(param);
    return 0;
}

void DmPersistVolCatalog::mod_startup()
{
}

void DmPersistVolCatalog::mod_shutdown()
{
}

//
// Creates volume catalog for volume described in
// volume descriptor 'vol_desc'
//
Error DmPersistVolCatalog::createCatalog(const VolumeDesc& vol_desc){
    Error err(ERR_OK);
    LOGTRACE << "Will create Catalog for volume " << vol_desc.name
             << ":" << std::hex << vol_desc.volUUID << std::dec;

    // TODO(Anna) we should experiment with choosing max object
    // entries in extent 0 and other extents and possible set
    // these values depenting on volume type (e.g., S3 vs. block)
    PersistVolumeMetaPtr volmeta(
        new(std::nothrow) PersistVolumeMeta(vol_desc.volUUID,
                                            vol_desc.maxObjSizeInBytes,
                                            MAX_EXTENT0_OBJS,
                                            MAX_EXTENT_OBJS, vol_desc.volUUID));
    if (!volmeta) {
        LOGERROR << "Failed to allocate persistent layer vol meta for vol"
                 << std::hex << vol_desc.volUUID << std::dec;
        return ERR_OUT_OF_MEMORY;
    }

    write_synchronized(vol_map_lock) {
        fds_verify(vol_map.count(vol_desc.volUUID) == 0);
        vol_map[vol_desc.volUUID] = volmeta;
    }
    return err;
}

//
// Create snapshot for volume specified by id
//
Error DmPersistVolCatalog::copyVolume(fds_volid_t srcVolId, fds_volid_t destVolId,
        fds_bool_t snapshot, fds_bool_t readOnly) {
    LOGTRACE << "Creating a Catalog for snapshot '" << std::hex << destVolId << std::dec <<
            "' for volume '" << std::hex << srcVolId << std::dec << "'";

    fds_uint32_t maxEntries = 0;
    fds_uint32_t extent0Entries = 0;
    fds_uint32_t extentEntries = 0;

    read_synchronized(vol_map_lock) {
        std::unordered_map<fds_volid_t, PersistVolumeMetaPtr>::const_iterator iter =
                vol_map.find(srcVolId);
        fds_verify(vol_map.end() != iter);
        maxEntries = iter->second->maxObjSizeBytes();
        extent0Entries = iter->second->extent0ObjEntries();
        extentEntries = iter->second->extentObjEntries();
    }

    const FdsRootDir* root = g_fdsprocess->proc_fdsroot();
    std::ostringstream oss;
    oss << root->dir_user_repo_dm() << srcVolId << "/" << srcVolId << "_vcat.ldb";
    std::string dbDir = oss.str();

    oss.clear();
    oss.str("");
    oss << root->dir_user_repo_dm();
    if (!snapshot) {
        oss <<  destVolId;
    } else {
        oss << srcVolId << "/snapshot";
    }

    FdsRootDir::fds_mkdir(oss.str().c_str());

    oss << "/" << destVolId << "_vcat.ldb";
    std::string copyDir =  oss.str();

    Error err = vol_map[srcVolId]->copyDB(copyDir);

    PersistVolumeMetaPtr volmeta(new PersistVolumeMeta(destVolId, maxEntries, extent0Entries,
            extentEntries, srcVolId, snapshot, readOnly));
    write_synchronized(vol_map_lock) {
        fds_verify(vol_map.count(destVolId) == 0);
        vol_map[destVolId] = volmeta;
    }

    return err;
}

//
// initializes volume catalog
//
Error DmPersistVolCatalog::openCatalog(fds_volid_t volume_id) {
    Error err(ERR_OK);
    PersistVolumeMetaPtr volmeta = getVolumeMeta(volume_id);
    if (!volmeta) {
        LOGWARN << "Vol " << std::hex << volume_id << std::dec << " does not exist";
        return ERR_VOL_NOT_FOUND;
    }

    LOGDEBUG << "Will open Catalog for vol " << std::hex << volume_id << std::dec;

    err = volmeta->init();
    if (!err.ok()) {
        LOGERROR << "Failed to open Catalog for volume " << std::hex
                 << volume_id << std::dec << " error " << err;
    }
    return err;
}

//
// Marks volume as deleted if the volume does not contain any valid blobs.
//
Error DmPersistVolCatalog::markVolumeDeleted(fds_volid_t volume_id) {
    PersistVolumeMetaPtr volmeta = getVolumeMeta(volume_id);
    if (!volmeta) {
        LOGWARN << "Vol " << std::hex << volume_id << std::dec << " does not exist";
        return ERR_VOL_NOT_FOUND;
    }

    return volmeta->markDeleted();
}

// delete volume catalog if it is marked as deleted
Error DmPersistVolCatalog::deleteEmptyCatalog(fds_volid_t volume_id) {
    Error err(ERR_OK);
    read_synchronized(vol_map_lock) {
        if (vol_map.count(volume_id) > 0) {
            PersistVolumeMetaPtr volmeta = vol_map[volume_id];
            if (volmeta->isMarkedDeleted()) {
                vol_map.erase(volume_id);
            } else {
                err = ERR_NOT_READY;
            }
        }
    }
    return err;
}

//
// Rsync volume catalog
//
Error DmPersistVolCatalog::syncCatalog(fds_volid_t volume_id,
                                       const NodeUuid& dm_uuid) {
    return getVolumeMeta(volume_id)->syncToDM(dm_uuid);
}

//
// Returns shared pointer to volume meta for 'volume_id'
//
PersistVolumeMetaPtr DmPersistVolCatalog::getVolumeMeta(fds_volid_t volume_id) {
    read_synchronized(vol_map_lock) {
        if (vol_map.count(volume_id) > 0) {
            return vol_map[volume_id];
        }
    }
    return PersistVolumeMetaPtr();
}

//
// Returns extent id given volume id and offset in bytes
//
fds_extent_id DmPersistVolCatalog::getExtentId(fds_volid_t volume_id,
                                               fds_uint64_t offset_bytes) {
    fds_extent_id ext_id = getVolumeMeta(volume_id)->getExtentId(offset_bytes);
    LOGTRACE << "Volume " << std::hex << volume_id << std::dec
             << ", offset " << offset_bytes << " bytes -> " << ext_id;

    return ext_id;
}

//
// Update or add extent 0 to the persistent volume catalog
// for given volume_id,blob_name
Error
DmPersistVolCatalog::putMetaExtent(fds_volid_t volume_id,
                                   const std::string& blob_name,
                                   const BlobExtent0::const_ptr& meta_extent) {
    Error err(ERR_OK);
    std::string serialized_data;
    std::string serialized_key;

    PersistVolumeMetaPtr volmeta = getVolumeMeta(volume_id);
    fds_assert(volmeta);
    if (volmeta->isSnapshot() || volmeta->isReadOnly()) {
        LOGWARN << "Volume is either snapshot or read only. Updates not allowed";
        return ERR_DM_OP_NOT_ALLOWED;
    }

    LOGTRACE << "Will update extent 0 for " << std::hex << volume_id << std::dec
             << "," << blob_name << " extent " << *meta_extent;

    err = getKeyString(blob_name, 0, serialized_key);
    fds_verify(err.ok());

    err = meta_extent->getSerialized(serialized_data);
    if (err.ok()) {
        err = volmeta->updateEntry(serialized_key, serialized_data);
        if (!err.ok()) {
            LOGERROR << "Failed to update extent 0 for volume " << std::hex
                     << volume_id << std::dec << " blob " << blob_name << " " << err;
        }
    }
    return err;
}

//
// Update or add extent to the persistent volume catalog
// for given volume_id,blob_name
Error DmPersistVolCatalog::putExtents(fds_volid_t volume_id,
                                      const std::string& blob_name,
                                      const BlobExtent0::const_ptr& meta_extent,
                                      const std::vector<BlobExtent::const_ptr>& extents) {
    Error err(ERR_OK);
    std::string serialized_meta;
    std::string serialized_key;
    CatWriteBatch batch;
    ExtentKey extent_key(blob_name, 0);

    PersistVolumeMetaPtr volmeta = getVolumeMeta(volume_id);
    fds_assert(volmeta);
    if (volmeta->isSnapshot() || volmeta->isReadOnly()) {
        LOGWARN << "Volume is either snapshot or or read only. Updates not allowed";
        return ERR_DM_OP_NOT_ALLOWED;
    }

    LOGTRACE << "Will update extents for " << std::hex << volume_id
             << std::dec << "," << blob_name << " meta extent " << *meta_extent;

    err = extent_key.getSerialized(serialized_key);
    if (!err.ok()) {
        LOGERROR << "Failed to serialize extent key for " << extent_key;
        return err;
    }

    err = meta_extent->getSerialized(serialized_meta);
    if (err.ok()) {
        batch.Put(serialized_key, serialized_meta);

        // add non-meta  extents
        for (std::vector<BlobExtent::const_ptr>::const_iterator cit = extents.cbegin();
             cit != extents.cend();
             ++cit) {
            std::string skey;
            std::string sdata;
            extent_key.extent_id = (*cit)->getExtentId();
            err = extent_key.getSerialized(skey);
            if (!err.ok()) break;
            if ((*cit)->containsNoObjects()) {
                LOGTRACE << "Will delete " << extent_key << " in vol " << std::hex
                         << volume_id << std::dec << " extent " << *(*cit);
                batch.Delete(skey);
            } else {
                LOGTRACE << "Will update " << extent_key << " for vol " << std::hex
                         << volume_id << std::dec << " extent " << *(*cit);
                err = (*cit)->getSerialized(sdata);
                if (!err.ok()) break;
                batch.Put(skey, sdata);
            }
        }
    }

    if (!err.ok()) {
        LOGERROR << "Failed to serialize extents for blob " << blob_name
                 << " volume " << std::hex << volume_id << std::dec << " " << err;
        return err;
    }

    // do atomic update to the database
    err = getVolumeMeta(volume_id)->updateBatch(&batch);
    if (!err.ok()) {
        LOGERROR << "Failed to update batch of extents for volume "
                 << std::hex << volume_id << std::dec << " " << err;
    }
    return err;
}

//
// Retrieves extent 0 from the persistent layer for given volume id, blob_name.
// Returns ERR_OK on success; ERR_CAT_ENTRY_NOT_FOUND if extent 0 does not exists in
// the persistent layer
// Returns BlobExtent0 containing extent from persistent layer; if err is
// ERR_CAT_ENTRY_NOT_FOUND, BlobExtent0 is allocated but not filled in; returns null
// pointer in other cases;
//
BlobExtent0::ptr DmPersistVolCatalog::getMetaExtent(fds_volid_t volume_id,
                                                    const std::string& blob_name,
                                                    Error& error) {
    Error err(ERR_OK);
    std::string serialized_key;
    std::string extent_data;
    PersistVolumeMetaPtr volmeta = getVolumeMeta(volume_id);

    LOGTRACE << "Will get extent 0 for " << std::hex << volume_id << std::dec
             << "," << blob_name;

    if (!volmeta) {
        LOGWARN << "Volume " << std::hex << volume_id << std::dec
                << " does not exist";
        error = ERR_VOL_NOT_FOUND;
        return BlobExtent0::ptr();
    }

    err = getKeyString(blob_name, 0, serialized_key);
    fds_verify(err.ok());

    err = volmeta->queryEntry(serialized_key, &extent_data);
    if (err.ok() || err == ERR_CAT_ENTRY_NOT_FOUND) {
        BlobExtent0::ptr extent0(new(std::nothrow) BlobExtent0(blob_name,
                                                               volmeta->maxObjSizeBytes(),
                                                               volmeta->firstOffset(0),
                                                               volmeta->numObjEntries(0)));
        if (extent0) {
            // if we read extent from catalog, fill in extent0 with data
            if (err.ok()) {
                fds_verify(extent0->loadSerialized(extent_data) == ERR_OK);
                LOGTRACE << "Retrieved extent0 " << *extent0;
            }
            LOGDEBUG << "Result of retrieving extent0 for " << std::hex
                     << volume_id << "," << blob_name << " err " << err;
            // otherwise will return BlobExtent0 without actual data but with
            // proper limits so that extent can be filled in with data
            error = err;
            return extent0;
        }
        err = ERR_OUT_OF_MEMORY;
    }

    error = err;
    LOGERROR << "Failed to query extent 0 for " << std::hex << volume_id
             << std::dec << "," << blob_name << " err " << err;
    return BlobExtent0::ptr();
}

//
// Retrieves extent with id 'extent_id' from the persistent layer for given
// volume id, blob_name.
// Returns ERR_OK on success; ERR_CAT_ENTRY_NOT_FOUND if extent 0 does not exists in
// the persistent layer
// Returns BlobExtent0 containing extent from persistent layer; if err is
// ERR_NOT_FOUND, BlobExtent0 is allocated but not filled in; returns null
// pointer in other cases;
//
BlobExtent::ptr DmPersistVolCatalog::getExtent(fds_volid_t volume_id,
                                               const std::string& blob_name,
                                               fds_extent_id extent_id,
                                               Error& error) {
    Error err(ERR_OK);
    std::string extent_data;
    std::string serialized_key;
    PersistVolumeMetaPtr volmeta = getVolumeMeta(volume_id);

    LOGTRACE << "Will get extent " << extent_id << " for "
             << std::hex << volume_id << std::dec << "," << blob_name;

    if (!volmeta) {
        LOGWARN << "Volume " << std::hex << volume_id << std::dec
                << " does not exist for blob " << blob_name;
        error = ERR_VOL_NOT_FOUND;
        return BlobExtent::ptr();
    }

    err = getKeyString(blob_name, extent_id, serialized_key);
    fds_verify(err.ok());

    err = volmeta->queryEntry(serialized_key, &extent_data);
    if (err.ok() || err == ERR_CAT_ENTRY_NOT_FOUND) {
        BlobExtent::ptr extent(new(std::nothrow) BlobExtent(extent_id,
                                                            volmeta->maxObjSizeBytes(),
                                                            volmeta->firstOffset(extent_id),
                                                            volmeta->numObjEntries(extent_id)));
        if (extent) {
            // if we read extent from catalog, fill in extent with data
            if (err.ok()) {
                fds_verify(extent->loadSerialized(extent_data) == ERR_OK);
            }
            // otherwise will return BlobExtent0 without actual data but with
            // proper limits so that extent can be filled in with data
            error = err;
            return extent;
        }
        err = ERR_OUT_OF_MEMORY;
    }

    LOGERROR << "Failed to query extent " << extent_id << " for blob "
             << blob_name << " " << err;
    error = err;
    return BlobExtent::ptr();
}

//
// Deletes extent 'extent_id' of blob 'blob_name' in volume catalog
// of volume 'volume_id'
//
Error DmPersistVolCatalog::deleteExtent(fds_volid_t volume_id,
                                        const std::string& blob_name,
                                        fds_extent_id extent_id) {
    Error err(ERR_OK);
    std::string key;
    PersistVolumeMetaPtr volmeta = getVolumeMeta(volume_id);

    if (volmeta->isSnapshot() || volmeta->isReadOnly()) {
        LOGWARN << "Volume is either snapshot or readonly, operation not allowed";
        return ERR_DM_OP_NOT_ALLOWED;
    }

    LOGTRACE << "Will delete extent " << extent_id << " for " << std::hex
             << volume_id << std::dec << "," << blob_name;

    if (!volmeta) {
        LOGWARN << "Vol " << std::hex << volume_id << std::dec << " does not exist";
        return ERR_VOL_NOT_FOUND;
    }

    err = getKeyString(blob_name, extent_id, key);
    fds_verify(err.ok());

    err = volmeta->deleteEntry(key);
    if (!err.ok()) {
        LOGERROR << "Failed to delete extent " << extent_id
                 << " for blob " << blob_name << " " << err;
    }
    return err;
}

Error DmPersistVolCatalog::getMetaDescList(fds_volid_t volume_id,
                                           std::vector<BlobExtent0Desc>& desc_list) {
    Error err(ERR_OK);
    Catalog::catalog_roptions_t options;
    Catalog::catalog_iterator_t* dbIt;
    PersistVolumeMetaPtr volmeta = getVolumeMeta(volume_id);

    if (!volmeta) {
        LOGWARN << "Vol " << std::hex << volume_id << std::dec << " does not exist";
        return ERR_VOL_NOT_FOUND;
    }

    LOGDEBUG << "Will get extents 0 for vol " << std::hex << volume_id << std::dec;

    dbIt = volmeta->getSnapshotIter(options);
    for (dbIt->SeekToFirst(); dbIt->Valid(); dbIt->Next()) {
        Record db_key = dbIt->key();
        ExtentKey extent_key;
        fds_verify(extent_key.loadSerialized(dbIt->key().ToString()) == ERR_OK);
        // ignore non-0 extents
        if (extent_key.extent_id > 0) {
            LOGTRACE << "Will ignore non-0 extent " << extent_key << " vol "
                     << std::hex << volume_id << std::dec;
            continue;
        }

        BlobExtent0Desc extdesc;
        fds_verify(extdesc.loadSerialized(dbIt->value().ToString()) == ERR_OK);
        LOGDEBUG << "Found extent 0" << extent_key << " vol "
                 << std::hex << volume_id << std::dec << " " << extdesc;
        if (extdesc.version != blob_version_deleted) {
            desc_list.push_back(extdesc);
        }
    }

    volmeta->releaseSnapshotIter(options, dbIt);
    return err;
}

Error DmPersistVolCatalog::getKeyString(const std::string &blob_name,
                                        fds_extent_id extent_id,
                                        std::string& serialized_key) {
    Error err(ERR_OK);
    ExtentKey extent_key(blob_name, extent_id);
    err = extent_key.getSerialized(serialized_key);
    if (!err.ok()) {
        LOGERROR << "Failed to serialize extent key for " << extent_key;
    }
    return err;
}

}  // namespace fds
