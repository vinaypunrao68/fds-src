/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMPERSISTVC_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMPERSISTVC_H_

#include <string>
#include <util/Log.h>
#include <fds_error.h>
#include <fds_module.h>
#include <dm-vol-cat/DmExtentTypes.h>

namespace fds {

    /**
     * Volume Catalog persistent layer
     */
    class DmPersistVolCatalog: public Module, public HasLogger {
  public:
        explicit DmPersistVolCatalog(char const *const name);
        ~DmPersistVolCatalog();

        /**
         * Module methods
         */
        virtual int mod_init(SysParams const *const param);
        virtual void mod_startup();
        virtual void mod_shutdown();

        /**
         * Creates volume catalog for volume described in
         * volume descriptor 'vol_desc'.
         */
        Error createCatalog(const VolumeDesc& vol_desc);

        /**
         * Maps offset in bytes to extent id for a given volume id.
         * Volume id is required because the mapping depends on volume
         * specific policy such as max object size, and also volume
         * specific configuration for persistent volume catalog, such as
         * maximum number of entries in an extent.
         * @param[in] volume_id is id of the volume
         * @param[in] offset_bytes is an offset in bytes
         * @return extent id that contains given offset in bytes
         */
        fds_extent_id getExtentId(fds_volid_t volume_id,
                                  fds_uint64_t offset_bytes);

        /**
         * Update or add extent 0 to the persistent volume catalog
         * for given volume_id,blob_name
         */
        Error putMetaExtent(fds_volid_t volume_id,
                            const std::string& blob_name,
                            const BlobExtent0::const_ptr& meta_extent);

        /**
         * Update or add extent to the persistent volume catalog
         * for given volume_id,blob_name
         */
        Error putExtent(fds_volid_t volume_id,
                        const std::string& blob_name,
                        fds_extent_id extent_id,
                        const BlobExtent::const_ptr& extent);

        /**
         * Retrieves extent 0 from the persistent layer for given
         * volume id, blob_name.
         * @param[out] error will contain ERR_OK on success; ERR_NOT_FOUND
         * if extent0 does not exist in the persistent layer; or other error.
         * @return BlobExtent0 containing extent from persistent layer; If
         * err is ERR_NOT_FOUND, BlobExtent0 is allocated but not filled in;
         * returns null pointer in other cases.
         */
        BlobExtent0::ptr getMetaExtent(fds_volid_t volume_id,
                                       const std::string& blob_name,
                                       Error& error);

        /**
         * Retrieves extent with id 'extent_id' from the persistent layer for given
         * volume id, blob_name.
         * @param[out] error will contain ERR_OK on success; ERR_NOT_FOUND
         * if extent0 does not exist in the persistent layer; or other error.
         * @return BlobExtent containing extent from persistent layer; If
         * err is ERR_NOT_FOUND, BlobExtent0 is allocated but not filled in;
         * returns null pointer in other cases.
         */
        BlobExtent::ptr getExtent(fds_volid_t volume_id,
                                  const std::string& blob_name,
                                  fds_extent_id extent_id,
                                  Error& error);

        /**
         * Deletes extent 'extent id' of blob 'blob_name' in volume
         * catalog of volume 'volume_id'
         */
        Error deleteExtent(fds_volid_t volume_id,
                           const std::string& blob_name,
                           fds_extent_id extent_id);
    };

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMPERSISTVC_H_
