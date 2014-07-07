/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
/**
 * This header contains blob extent types
 * Blob extent is a unit of storage in volume catalog persistent
 * layer. Metadata describing a blob is stored in persistent layer
 * as a set of extents, where each extent contains a range of
 * object offsets. The offset range (and the size of each extent)
 * is configurable per-volume and a property of per volume persistent
 * catalog. Blob's metadata is stored in extent 0.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMEXTENTTYPES_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMEXTENTTYPES_H_

#include <string>
#include <map>
#include <DmBlobTypes.h>

namespace fds {

    typedef fds_uint32_t fds_extent_id;
    static const fds_extent_id fds_extentid_meta = 0;

    /**
     * BlobExtent is an extent that contains offset to object info
     * mapping. It is the base class for extent 0 (that also contains
     * blob meta) and class for all remaining extents.
     *
     * Current imlementation assumes that all offsets are max_obj_size
     * aligned. So any access to unaligned offset currently asserts
     * BlobExtent is unaware of each object size
     */
    class BlobExtent: public serialize::Serializable {
  public:
        typedef boost::shared_ptr<BlobExtent> ptr;
        typedef boost::shared_ptr<const BlobExtent> const_ptr;

        /**
         * Constructs extent with no offset->obj info entries
         */
        BlobExtent(fds_extent_id ext_id,
                   fds_uint32_t max_obj_size,
                   fds_uint32_t first_off,
                   fds_uint32_t num_off);
        virtual ~BlobExtent();

        /**
         * @return ID of this extent
         */
        inline fds_extent_id getExtentId() const {
            return extent_id;
        }

        /**
         * Updates offset to object info mapping, or adds it
         * if offset does not exist
         * 
         * @return ERR_OK if successfully updated/added offset to
         * object info mapping; ERR_DM_OFFSET_OUT_RANGE if this offset
         * is outside of offset range of this extent
         */
        Error updateOffset(fds_uint64_t offset, const BlobObjInfo& obj_info);

       /**
         * Retrieves object info at the given offset into 'obj_info'
         *
         * @return ERR_OK if offset is found; ERR_NOT_FOUND if offset
         * to object info mapping does not exist in volume catalog;
         * ERR_DM_OFFSET_OUTSIDE_RANGE is this offset is outside of
         * offset range of this extent
         */
        Error getObjectInfo(fds_uint64_t offset, BlobObjInfo* obj_info) const;

        virtual uint32_t write(serialize::Serializer* s) const;
        virtual uint32_t read(serialize::Deserializer* d);

        friend std::ostream& operator<<(std::ostream& out, const BlobExtent& extent);

  protected:
        fds_extent_id extent_id;
        /**
         * offset in units of 'offset_unit_bytes' to object info map
         */
        std::map<fds_uint32_t, BlobObjInfo> blob_obj_list;

        fds_uint32_t offset_unit_bytes;
        fds_uint32_t first_offset;
        fds_uint32_t last_offset;
    };

    /**
     * BlobExtent0 is the first extent of the blob meta that in addition
     * to holding offset to object info entries like other extents, also
     * contains metadata part of the blob (blob name, size, version, metadata
     * key-value pairs)
     */
    class BlobExtent0 : public BlobExtent {
  public:
        typedef boost::shared_ptr<BlobExtent0> ptr;
        typedef boost::shared_ptr<const BlobExtent0> const_ptr;

        /**
         * Constructs extent with un-initialized entries
         * except blob name and volume id
         */
        BlobExtent0(const std::string& blob_name,
                    fds_volid_t volume_id,
                    fds_uint32_t max_obj_size,
                    fds_uint32_t first_off,
                    fds_uint32_t num_offsets);
        virtual ~BlobExtent0();

        inline const std::string& blobName() const {
            return blob_meta.blob_name;
        }
        inline fds_volid_t volumeId() const {
            return blob_meta.vol_id;
        }
        inline blob_version_t blobVersion() const {
            return blob_meta.version;
        }
        inline fds_uint64_t blobSize() const {
            return blob_meta.blob_size;
        }

        uint32_t write(serialize::Serializer* s) const;
        uint32_t read(serialize::Deserializer* d);

        friend std::ostream& operator<<(std::ostream& out, const BlobExtent0& extent0);

  protected:
        BlobMetaDesc blob_meta;
    };

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMEXTENTTYPES_H_

