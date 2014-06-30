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
#include <map>
#include <fds_types.h>
#include <fds_volume.h>
#include <blob/BlobTypes.h>

namespace fds {

struct BlobObjList
        : std::unordered_map<fds_uint64_t, ObjectID> {
    typedef boost::shared_ptr<BlobObjList> ptr;
    typedef boost::shared_ptr<const BlobObjList> const_ptr;
    typedef std::unordered_map<
            fds_uint64_t,
            ObjectID>::const_iterator const_it;

    /**
     * Constructs the BlobObjList object with empty list
     */
    BlobObjList();
    /**
     * Constructs the object from FDSP object list type
     */
    explicit BlobObjList(const fpi::FDSP_BlobObjectList& blob_obj_list);
    virtual ~BlobObjList();
};


/**
 * Metadata that describes the blob, not including the
 * the offset to object id mappings list of the metadata
 */
struct BlobMetaDesc {
    std::string blob_name;
    fds_volid_t vol_id;
    blob_version_t version;
    fds_uint64_t blob_size;
    BlobKeyValue meta_list;

    typedef boost::shared_ptr<BlobMetaDesc> ptr;
    typedef boost::shared_ptr<const BlobMetaDesc> const_ptr;

    /**
     * Constructs invalid BlobMetaDesc object, must initialize
     * to valid fields after the constructor.
     */
    BlobMetaDesc();
    virtual ~BlobMetaDesc();

    void mkFDSPMetaDataList(fpi::FDSP_MetaDataList& mlist) const;
};

namespace BlobUtil {
    void toFDSPQueryCatalogMsg(const BlobMetaDesc::const_ptr& blob_meta_desc,
                               const BlobObjList::const_ptr& blob_obj_list,
                               fds_uint32_t max_obj_size_bytes,
                               fpi::FDSP_QueryCatalogTypePtr& query_msg);
}  // namespace BlobUtil


}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DMBLOBTYPES_H_

