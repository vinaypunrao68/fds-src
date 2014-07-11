/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <fds_process.h>
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
    /**
     * Does not initialize catalog yet
     */
    PersistVolumeMeta(fds_volid_t volid,
                      fds_uint32_t max_obj_size,
                      fds_uint32_t extent0_objs,
                      fds_uint32_t extent_objs);
    ~PersistVolumeMeta();

    /**
     * Initializes and opens persistent catalog
     */
    Error init();

    inline fds_volid_t volumeId() const {
        return volume_id;
    }
    inline fds_uint32_t maxObjSizeBytes() const {
        return max_obj_size_bytes;
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
        return catalog->Update(key, value);
    }

    /**
     * Queries DB for entry with key 'key'
     */
    inline Error queryEntry(const std::string& key,
                            std::string* value) {
        return catalog->Query(key, value);
    }

    /**
     * Deletes entry with key 'key' from the DB
     */
    inline Error deleteEntry(const std::string& key) {
        return catalog->Delete(key);
    }

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
    Catalog *catalog;
};


//
// does not initialize catalog yet
//
PersistVolumeMeta::PersistVolumeMeta(fds_volid_t volid,
                                     fds_uint32_t max_obj_size,
                                     fds_uint32_t extent0_objs,
                                     fds_uint32_t extent_objs)
        : volume_id(volid),
          max_obj_size_bytes(max_obj_size),
          extent0_obj_entries(extent0_objs),
          extent_obj_entries(extent_objs),
          catalog(NULL) {
    fds_verify(extent0_obj_entries > 0);
    fds_verify(extent_obj_entries > 0);
}

PersistVolumeMeta::~PersistVolumeMeta() {
    if (catalog) {
        delete catalog;
    }
}

Error PersistVolumeMeta::init() {
    Error err(ERR_OK);
    const std::string vol_name = std::to_string(volume_id);
    const FdsRootDir* root = g_fdsprocess->proc_fdsroot();
    const std::string cat_name = root->dir_user_repo_dm() + vol_name + "_vcat.ldb";
    fds_verify(catalog == NULL);
    catalog = new(std::nothrow) Catalog(cat_name);
    if (!catalog) {
        LOGERROR << "Failed to create catalog for volume "
                 << std::hex << volume_id << std::dec;
        return ERR_OUT_OF_MEMORY;
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
                                            1024,
                                            2048));
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
// initializes volume catalog
//
Error DmPersistVolCatalog::openCatalog(fds_volid_t volume_id) {
    Error err(ERR_OK);
    PersistVolumeMetaPtr volmeta = getVolumeMeta(volume_id);

    LOGTRACE << "Will open Catalog for volume "
             << std::hex << volume_id << std::dec;

    err = volmeta->init();
    if (!err.ok()) {
        LOGERROR << "Failed to open Catalog for volume " << std::hex
                 << volume_id << std::dec << " error " << err;
    }
    return err;
}

//
// Returns shared pointer to volume meta for 'volume_id'
//
PersistVolumeMetaPtr DmPersistVolCatalog::getVolumeMeta(fds_volid_t volume_id) {
    read_synchronized(vol_map_lock) {
        fds_verify(vol_map.count(volume_id) > 0);
        return vol_map[volume_id];
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

    LOGTRACE << "Will update extent 0 for " << std::hex << volume_id << std::dec
             << "," << blob_name << " extent " << *meta_extent;

    err = meta_extent->getSerialized(serialized_data);
    if (err.ok()) {
        err = putExtent(volume_id, blob_name, 0, serialized_data);
    }
    return err;
}

//
// Update or add extent to the persistent volume catalog
// for given volume_id,blob_name
Error DmPersistVolCatalog::putExtent(fds_volid_t volume_id,
                                     const std::string& blob_name,
                                     fds_extent_id extent_id,
                                     const BlobExtent::const_ptr& extent) {
    Error err(ERR_OK);
    std::string serialized_data;

    // must be called for extents other than extent 0
    fds_verify(extent_id > 0);

    LOGTRACE << "Will update extent " << extent_id << " for " << std::hex << volume_id
             << std::dec << "," << blob_name << " extent " << *extent;

    err = extent->getSerialized(serialized_data);
    if (err.ok()) {
        err = putExtent(volume_id, blob_name, extent_id, serialized_data);
    }
    return err;
}

//
// Actually updates volume catalog
//
Error DmPersistVolCatalog::putExtent(fds_volid_t volume_id,
                                     const std::string& blob_name,
                                     fds_extent_id extent_id,
                                     const std::string& extent_data) {
    Error err(ERR_OK);
    std::string key = blob_name + std::to_string(extent_id);

    err = getVolumeMeta(volume_id)->updateEntry(key, extent_data);
    if (!err.ok()) {
        LOGERROR << "Failed to update extent " << extent_id << " for volume " << std::hex
                 << volume_id << std::dec << " error " << err;
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
    std::string extent_data;
    PersistVolumeMetaPtr volmeta = getVolumeMeta(volume_id);

    LOGTRACE << "Will get extent 0 for " << std::hex << volume_id << std::dec
             << "," << blob_name;

    err = volmeta->queryEntry(blob_name + std::to_string(0), &extent_data);
    if (err.ok() || err == ERR_CAT_ENTRY_NOT_FOUND) {
        BlobExtent0::ptr extent0(new(std::nothrow) BlobExtent0(blob_name, volume_id,
                                                               volmeta->maxObjSizeBytes(),
                                                               volmeta->firstOffset(0),
                                                               volmeta->numObjEntries(0)));
        if (extent0) {
            // if we read extent from catalog, fill in extent0 with data
            if (err.ok()) {
                fds_verify(extent0->loadSerialized(extent_data) == ERR_OK);
            }
            // otherwise will return BlobExtent0 without actual data but with
            // proper limits so that extent can be filled in with data
            error = err;
            return extent0;
        }
        err = ERR_OUT_OF_MEMORY;
    }

    error = err;
    LOGERROR << "Failed to query extent 0 for blob " << blob_name << " err " << err;
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
    PersistVolumeMetaPtr volmeta = getVolumeMeta(volume_id);

    LOGTRACE << "Will get extent " << extent_id << " for " << std::hex << volume_id
             << std::dec << "," << blob_name;

    err = volmeta->queryEntry(blob_name + std::to_string(extent_id), &extent_data);
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

    LOGERROR << "Failed to query extent " << extent_id <<
            " for blob " << blob_name << " err " << err;
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
    PersistVolumeMetaPtr volmeta = getVolumeMeta(volume_id);

    LOGTRACE << "Will delete extent " << extent_id << " for " << std::hex
             << volume_id << std::dec << "," << blob_name;

    err = volmeta->deleteEntry(blob_name + std::to_string(extent_id));
    if (!err.ok()) {
        LOGERROR << "Failed to delete extent " << extent_id <<
                " for blob " << blob_name << " err " << err;
    }
    return err;
}

}  // namespace fds
