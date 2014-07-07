/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <dm-vol-cat/DmPersistVc.h>

namespace fds {

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
    return err;
}

//
// Returns extent id given volume id and offset in bytes
//
fds_extent_id DmPersistVolCatalog::getExtentId(fds_volid_t volume_id,
                                               fds_uint64_t offset_bytes) const {
    fds_extent_id ext_id = 0;
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
    LOGTRACE << "Will update extent 0 for " << std::hex << volume_id << std::dec
             << "," << blob_name << " extent " << *meta_extent;
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
    LOGTRACE << "Will update extent " << extent_id << " for " << std::hex << volume_id
             << std::dec << "," << blob_name << " extent " << *extent;
    return err;
}

//
// Retrieves extent 0 from the persistent layer for given volume id, blob_name.
// Returns ERR_OK on success; ERR_NOT_FOUND if extent 0 does not exists in
// the persistent layer
// Returns BlobExtent0 containing extent from persistent layer; if err is
// ERR_NOT_FOUND, BlobExtent0 is allocated but not filled in; returns null
// pointer in other cases;
//
BlobExtent0::ptr DmPersistVolCatalog::getMetaExtent(fds_volid_t volume_id,
                                                    const std::string& blob_name,
                                                    Error& error) {
    LOGTRACE << "Will get extent 0 for " << std::hex << volume_id << std::dec
             << "," << blob_name;
    return BlobExtent0::ptr();
}

//
// Retrieves extent with id 'extent_id' from the persistent layer for given
// volume id, blob_name.
// Returns ERR_OK on success; ERR_NOT_FOUND if extent 0 does not exists in
// the persistent layer
// Returns BlobExtent0 containing extent from persistent layer; if err is
// ERR_NOT_FOUND, BlobExtent0 is allocated but not filled in; returns null
// pointer in other cases;
//
BlobExtent::ptr DmPersistVolCatalog::getExtent(fds_volid_t volume_id,
                                               const std::string& blob_name,
                                               fds_extent_id extent_id,
                                               Error& error) {
    LOGTRACE << "Will get extent " << extent_id << " for " << std::hex << volume_id
             << std::dec << "," << blob_name;
    return BlobExtent::ptr();
}

//
// Deletes extent 'extent id' of blob 'blob_name' in volume
// catalog of volume 'volume_id'
//
Error DmPersistVolCatalog::deleteExtent(fds_volid_t volume_id,
                                        const std::string& blob_name,
                                        fds_extent_id extent_id) {
    Error err(ERR_OK);
    return err;
}

DmPersistVolCatalog gl_DmVolCatPlMod("Global DM Volume Catalog Persistent Layer");

}  // namespace fds
