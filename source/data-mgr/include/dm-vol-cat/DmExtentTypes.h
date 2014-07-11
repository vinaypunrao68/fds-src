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
#include <vector>
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
        inline fds_uint32_t maxObjSizeBytes() const {
            return offset_unit_bytes;
        }

        /**
         * Returns true if offset is in range for this extent
         */
        fds_bool_t offsetInRange(fds_uint64_t offset) const;

        /**
         * Updates offset to object info mapping, or adds it
         * if offset does not exist
         * 
         * @return ERR_OK if successfully updated/added
         * offset to object info mapping; ERR_DM_OFFSET_OUT_RANGE if
         * this offset is outside of offset range of this extent
         */
        Error updateOffset(fds_uint64_t offset, const ObjectID& obj_info);

       /**
         * Retrieves object info at the given offset into 'obj_info'
         *
         * @return ERR_OK if offset is found; ERR_NOT_FOUND if offset
         * to object info mapping does not exist in volume catalog;
         * ERR_DM_OFFSET_OUTSIDE_RANGE is this offset is outside of
         * offset range of this extent
         */
        Error getObjectInfo(fds_uint64_t offset, ObjectID* obj_info) const;

        /**
         * Truncates extent. All objects with offset > last_offset are removed
         * and object ids returned in the rm_list
         * @param[out] last_off_removed last offset removed from the list
         * @param[in,out] ret_rm_list list of objects that were removed, the
         * list gets updated, not cleared first
         * @return number of objects removed from the list
         */
        fds_uint32_t truncate(fds_uint64_t last_offset,
                              fds_uint64_t* last_off_removed,
                              std::vector<ObjectID>* ret_rm_list);

        virtual uint32_t write(serialize::Serializer* s) const;
        virtual uint32_t read(serialize::Deserializer* d);

        friend std::ostream& operator<<(std::ostream& out, const BlobExtent& extent);

  private:  // methods
        inline fds_uint32_t toOffsetUnits(fds_uint64_t offset) const {
            fds_uint32_t off_units = offset / offset_unit_bytes;
            // must be max obj size aligned (until we add support for
            // unaligned offsets in DM)
            fds_verify((offset % offset_unit_bytes) == 0);
            return off_units;
        }

  protected:
        fds_extent_id extent_id;
        /**
         * offset in units of 'offset_unit_bytes' to object info map
         */
        std::map<fds_uint32_t, ObjectID> blob_obj_list;

        fds_uint32_t offset_unit_bytes;
        fds_uint32_t first_offset;
        fds_uint32_t last_offset;

        // TODO(xxx) consider managing total obj size inside this
        // data struct (size of part of the blob)
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
        inline fds_uint64_t lastOffset() const {
            return last_offset;
        }
        inline fds_uint64_t lastObjSize() const {
            if (blob_meta.blob_size == 0) return 0;
            fds_uint64_t size = blob_meta.blob_size % maxObjSizeBytes();
            if (size > 0) size = maxObjSizeBytes();
            fds_verify(blob_meta.blob_size >= size);
            return size;
        }

        /**
         * Sets size of the blob
         */
        inline void setBlobSize(fds_uint64_t blob_size) {
            blob_meta.blob_size = blob_size;
        }
        inline void incrementBlobVersion() {
            if (blob_meta.version == blob_version_deleted) {
                blob_meta.version = blob_version_initial;
            } else {
                blob_meta.version += 1;
            }
        }
        inline void setLastOffset(fds_uint64_t offset) {
            last_offset = offset;
        }

        /**
         * Update key-value metadata list, if entry already exists
         * in the current metadata list, it is updated with a new value
         */
        void updateMetaData(const MetaDataList& meta_list);

        uint32_t write(serialize::Serializer* s) const;
        uint32_t read(serialize::Deserializer* d);

        friend std::ostream& operator<<(std::ostream& out, const BlobExtent0& extent0);

  protected:
        BlobMetaDesc blob_meta;
        /**
         * Last offset of the blob.
         * Since for now we assume all obj are max obj size
         * except last obj variable size, we need to remember the last
         * offset to correctly keep track of size. Plus, last offset
         * also helps for deleting the whole blob (to find out which
         * extent id is last extent)
         */
        fds_uint64_t last_offset;
    };

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMEXTENTTYPES_H_

