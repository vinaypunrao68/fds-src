/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
/**
 * This header contains blob-related data structs that
 * are common for Volume Catalog, Time Volume Catalog, and DM
 * processing layer.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_DMBLOBTYPES_H_
#define SOURCE_DATA_MGR_INCLUDE_DMBLOBTYPES_H_

#include <string>
#include <unordered_map>
#include <map>
#include "fdsp/dm_api_types.h"
#include <fds_types.h>
#include <fds_volume.h>

#define APPROX_BLOB_NAME_SIZE 256
#define METADATA_SIZE(blobCount, objectCount) \
        (blobCount * (APPROX_BLOB_NAME_SIZE + sizeof(BasicBlobMeta)) + \
        objectCount * (OBJECTID_DIGESTLEN + sizeof(fds_volid_t)))

namespace fds {

/**
 * List of metadata key-value pairs
 * Implemented as a map so we can easily search for particular key
 */
struct MetaDataList :
            std::map<std::string, std::string>,
            serialize::Serializable {
    typedef boost::shared_ptr<MetaDataList> ptr;
    typedef boost::shared_ptr<const MetaDataList> const_ptr;
    typedef std::unordered_map<std::string, std::string>::const_iterator const_iter;

    /**
     * Constructs empty metadata list
     */
    MetaDataList();
    /**
     * Constructs metadata list from FDSP metadata message
     */
    explicit MetaDataList(const fpi::FDSP_MetaDataList& mlist);
    virtual ~MetaDataList();

    /**
     * Add metadata key-value pair to the list if key it does not
     * exist yet, or update existing key-value pair
     */
    void updateMetaDataPair(const std::string& key, const std::string& value);
    /**
     * Copies metadata list to FDSP metadata list message; if there is any
     * data in mlist, it will be cleared first;
     */
    void toFdspPayload(fpi::FDSP_MetaDataList& mlist) const;

    /**
     * Merge name-value pairs from rhs into this and return reference to this
     */
    MetaDataList & merge(const MetaDataList & rhs);

    uint32_t write(serialize::Serializer* s) const;
    uint32_t read(serialize::Deserializer* d);

    MetaDataList& operator=(const MetaDataList &rhs);
    bool operator==(const MetaDataList& lhs) const;
};

/**
 * Object info we keep in the offset to object info list in the blob
 * At the moment we still have object size field here. Currently all
 * objects in the blob are max obj size except the last one. However, that
 * can change + we still need to be able to verify that this assumption
 * still holds. So, in the Vol Cat we maintain this assumption and keep
 * smaller datastucts there. But not in TVC layer. We need to do more work on
 * DM service layer API and agree on all the limits/requirements for the
 * object sizes, and then possibly improve space usage for blob info.
 */
struct BlobObjInfo : serialize::Serializable {
    ObjectID oid;
    fds_uint64_t size;

    BlobObjInfo();
    explicit BlobObjInfo(const fpi::FDSP_BlobObjectInfo& obj_info);
    BlobObjInfo(const ObjectID& obj_id, fds_uint64_t sz);
    ~BlobObjInfo();

    uint32_t write(serialize::Serializer* s) const;
    uint32_t read(serialize::Deserializer* d);

    BlobObjInfo& operator=(const BlobObjInfo &rhs);
};

/**
 * A list of offset to Object info mappings
 * Object info is just an object ID
 */
struct BlobObjList :
            std::map<fds_uint64_t, BlobObjInfo>,
            serialize::Serializable {
    typedef boost::shared_ptr<BlobObjList> ptr;
    typedef boost::shared_ptr<const BlobObjList> const_ptr;
    typedef std::map<fds_uint64_t, BlobObjInfo>::const_iterator const_iter;

    /**
     * Constructs the BlobObjList object with empty list
     */
    BlobObjList();
    /**
     * Constructs the object from FDSP object list type
     */
    explicit BlobObjList(const fpi::FDSP_BlobObjectList& blob_obj_list);
    virtual ~BlobObjList();

    /**
     * Sets the last offset in this obj list to be last offset of the blob.
     * This must be the last call that changes object list (if we need to
     * set end of blob). After that call, merge will assert.
     */
    inline void setEndOfBlob() {
        end_of_blob = true;
    }

    /**
     * Returns true if the last offset of this obj list is also a
     * last offset of the blob
     */
    inline fds_bool_t endOfBlob() const {
        return end_of_blob;
    }

    /**
     * Returns last offset in the blob object list;
     * If object list empty, returns max uint64
     */
    fds_uint64_t lastOffset() const;

    /**
     * Merges itself with another BlobObjList list and returns itself
     * If newer_blist contains an offset which already exists in this
     * list, it will over-write this offset's object info with object
     * info from newer_list
     */
    BlobObjList& merge(const BlobObjList& newer_blist);

    /**
     * Update offset to object info mapping, if offset does not exit,
     * adds as new offset to object info mapping
     */
    void updateObject(fds_uint64_t offset,
                      const ObjectID& oid,
                      fds_uint64_t size);

    /**
     * This is for current implementation -- we expect that every
     * obj size is max obj size, unless this is the last object of the blob.
     * We also expect that each offset is max_obj_size aligned
     *
     * If we remove any of these assumptions, we will
     * the last object of the blob; Once that assumption is not true,
     * we need to make some modifications in Volume Catalog to
     * remove these assumptions
     */
    Error verify(fds_uint32_t max_obj_size) const;

    /**
     * Copies obj info list to FDSP obj list message; if there is any
     * data in blob_obj_list, it will be cleared first;
     */
    void toFdspPayload(fpi::FDSP_BlobObjectList& blob_obj_list) const;

    uint32_t write(serialize::Serializer* s) const;
    uint32_t read(serialize::Deserializer* d);

  private:
    /**
     * If true, the highest offset in the map
     * is the last offset (e.g. if truncating the blob)
     */
    fds_bool_t end_of_blob;
};

/**
 * Basic blob metadata
 */
struct BasicBlobMeta: serialize::Serializable {
    std::string blob_name;
    blob_version_t version;
    sequence_id_t sequence_id;

    // NOTE(bszmyd): Wed 07 Jan 2015 04:31:44 PM PST
    // This should be interpreted as the size of the blob
    // including any missing (i.e. sparse) objects. Not the
    // actual data-on-disk size.
    fds_uint64_t blob_size;

    BasicBlobMeta();
    virtual ~BasicBlobMeta();

    uint32_t write(serialize::Serializer* s) const;
    uint32_t read(serialize::Deserializer* d);

    bool operator==(const BasicBlobMeta& rhs) const;
    BasicBlobMeta& operator=(const BasicBlobMeta& rhs) = default;
};

/**
 * Metadata that describes the blob, not including the
 * the offset to object id mappings list of the metadata
 */
struct BlobMetaDesc: serialize::Serializable {
    BasicBlobMeta desc;  // basic blob meta descriptor
    MetaDataList  meta_list;  // key-value metadata

    typedef boost::shared_ptr<BlobMetaDesc> ptr;
    typedef boost::shared_ptr<const BlobMetaDesc> const_ptr;

    /**
     * Constructs invalid BlobMetaDesc object, must initialize
     * to valid fields after the constructor.
     */
    BlobMetaDesc();
    virtual ~BlobMetaDesc();

    uint32_t write(serialize::Serializer* s) const;
    uint32_t read(serialize::Deserializer* d);

    BlobMetaDesc& operator=(const BlobMetaDesc& rhs);
    bool operator==(const BlobMetaDesc &rhs) const;
};

/**
 * Metadata that describes the volume and catalog state.
 */
struct VolumeMetaDesc : serialize::Serializable {
    /// Descriptor of the volume that the catalog is backing
    // TODO(Andrew): Add this back when we do restartability work.
    // VolumeDesc volDesc;
    sequence_id_t sequence_id;
    /// List of user defined volume specific key-value metadata
    MetaDataList  meta_list;

    typedef boost::shared_ptr<VolumeMetaDesc> ptr;
    typedef boost::shared_ptr<const VolumeMetaDesc> const_ptr;

    /**
     * Constructs invalid VolumeMetaDesc object, must initialize
     * to valid fields after the constructor.
     */
    explicit VolumeMetaDesc(const fpi::FDSP_MetaDataList &metadataList,
                            const sequence_id_t seq_id);
    virtual ~VolumeMetaDesc();

    uint32_t write(serialize::Serializer* s) const;
    uint32_t read(serialize::Deserializer* d);

    VolumeMetaDesc& operator=(const VolumeMetaDesc& rhs);
    bool operator==(const VolumeMetaDesc& rhs) const;
};

std::ostream& operator<<(std::ostream& out, const BasicBlobMeta& bdesc);
std::ostream& operator<<(std::ostream& out, const MetaDataList& metaList);
std::ostream& operator<<(std::ostream& out, const BlobMetaDesc& blobMetaDesc);
std::ostream& operator<<(std::ostream& out, const BlobObjList& obj_list);
std::ostream& operator<<(std::ostream& out, const VolumeMetaDesc& volumeMetaDesc);

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DMBLOBTYPES_H_
