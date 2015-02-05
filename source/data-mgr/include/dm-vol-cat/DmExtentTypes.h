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
#include <set>
#include <DmBlobTypes.h>

namespace fds {

typedef fds_uint32_t fds_extent_id;
static const fds_extent_id fds_extentid_meta = 0;

/**
 * BlobExtent is an extent that contains offset to object info
 * mapping. It is the base class for extent 0 (that also contains
 * blob meta) and class for all remaining extents.
 *
 * Current implementation assumes that all offsets are max_obj_size
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
    BlobExtent(const BlobExtent& rhs);
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
    inline fds_uint64_t lastOffsetInRange() const {
        fds_uint64_t off_unit = offset_unit_bytes;
        fds_uint64_t last_off = last_offset;
        return off_unit * last_off;
    }
    inline fds_bool_t containsNoObjects() const {
        return (blob_obj_list.size() == 0);
    }
    inline fds_uint64_t objectCount() const {
        return blob_obj_list.size();
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
     * Truncates extent. All objects with offset > after_offset are removed
     * and object ids returned in the rm_list
     * @param[in,out] ret_rm_list list of objects that were removed, the
     * list gets updated, not cleared first
     * @return number of objects removed from the list
     */
    fds_uint32_t truncate(fds_uint64_t after_offset,
                          std::vector<ObjectID>* ret_rm_list);

    /**
     * Removes all objects from this extents; non-null objects that are
     * removed are returned in the rm_list
     * @param[in,out] ret_rm_list list of non-null objects that were removed,
     * the list gets updated, not cleared first
     */
    void deleteAllObjects(std::vector<ObjectID>* ret_rm_list);

    /**
     * Get all objects from this extent
     *
     * @param[in,out] ojIds set of objects
     */
    void getAllObjects(std::set<ObjectID> & objIds);

    void getAllObjects(std::vector<ObjectID> & onjIds);

    /**
     * Appends offset to object mappings from this extent to blob_obj_list
     */
    void addToFdspPayload(fpi::FDSP_BlobObjectList& olist,
                          fds_uint64_t last_blob_off,
                          fds_uint64_t last_obj_size) const;

    virtual uint32_t write(serialize::Serializer* s) const;
    virtual uint32_t read(serialize::Deserializer* d);

    BlobExtent& operator=(const BlobExtent& rhs);

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
    // data structures (size of part of the blob)
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
                fds_uint32_t max_obj_size,
                fds_uint32_t first_off,
                fds_uint32_t num_offsets);
    BlobExtent0(const BlobExtent0& rhs);
    virtual ~BlobExtent0();

    inline const std::string& blobName() const {
        return blob_meta.desc.blob_name;
    }
    inline blob_version_t blobVersion() const {
        return blob_meta.desc.version;
    }
    inline fds_uint64_t blobSize() const {
        return blob_meta.desc.blob_size;
    }
    inline fds_uint64_t lastBlobOffset() const {
        return last_blob_offset;
    }
    inline fds_uint64_t lastObjSize() const {
        if (blob_meta.desc.blob_size == 0) return 0;
        fds_uint64_t size = blob_meta.desc.blob_size % maxObjSizeBytes();
        // TODO(xxx) do we have a case when blob has multiple
        // objects and at least one obj has size zero?
        // this method is not correct if last objs is Null obj
        if (size == 0) size = maxObjSizeBytes();
        fds_verify(blob_meta.desc.blob_size >= size);
        return size;
    }

    /**
     * Sets size of the blob
     */
    inline void setBlobSize(fds_uint64_t blob_size) {
        blob_meta.desc.blob_size = blob_size;
    }
    inline void incrementBlobVersion() {
        if (blob_meta.desc.version == blob_version_deleted) {
            blob_meta.desc.version = blob_version_initial;
        } else {
            blob_meta.desc.version += 1;
        }
    }
    inline void setLastBlobOffset(fds_uint64_t offset) {
        last_blob_offset = offset;
    }

    /**
     * Mark object as deleted
     */
    void markDeleted();

    /**
     * Checks if the blob is marked as deleted
     */
    fds_bool_t isDeleted() const;

    /**
     * Update key-value metadata list, if entry already exists
     * in the current metadata list, it is updated with a new value
     */
    void updateMetaData(const MetaDataList::const_ptr& meta_list);

    /**
     * fills in fdsp metadata list from metadata list in extent0
     */
    inline void toMetaFdspPayload(fpi::FDSP_MetaDataList& mlist) const {
        blob_meta.meta_list.toFdspPayload(mlist);
    }

    uint32_t write(serialize::Serializer* s) const;
    uint32_t read(serialize::Deserializer* d);

    BlobExtent0& operator=(const BlobExtent0& rhs);

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
    fds_uint64_t last_blob_offset;
};

/**
 * This is for partial serialization of extent 0
 */
typedef BasicBlobMeta BlobExtent0Desc;

/**
 * Describes key in the Volume Catalog database
 */
struct ExtentKey: public serialize::Serializable {
    std::string blob_name;
    fds_extent_id extent_id;

    ExtentKey() : extent_id(0) {}
    ExtentKey(std::string bname, fds_extent_id eid)
            : blob_name(bname), extent_id(eid) {}
    ~ExtentKey() {}

    virtual uint32_t write(serialize::Serializer* s) const {
        uint32_t bytes = 0;
        bytes += s->writeI32(extent_id);
        bytes += s->writeString(blob_name);
        return bytes;
    }
    virtual uint32_t read(serialize::Deserializer* d) {
        uint32_t bytes = 0;
        bytes += d->readI32(extent_id);
        bytes += d->readString(blob_name);
        return bytes;
    }
    std::string toString() const {
        return blob_name + std::to_string(extent_id);
    }

    bool operator==(const ExtentKey& rhs) const;
    friend std::ostream& operator<<(std::ostream& out, const ExtentKey& key);
};

/// Provides hashing function for extent keys
class ExtentKeyHash {
  public:
    size_t operator()(const ExtentKey& eKey) const {
        return std::hash<std::string>()(eKey.toString());
    }
};

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMEXTENTTYPES_H_

