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

namespace fds {

/**
 * Needs review: why MetaList in VolumeMeta is a vector
 * of std::pair<string, string>?
 */
typedef std::map<std::string, std::string> MetadataList;

/**
 * Metadata that describes the blob, not including the
 * the offset to object id mappings list of the metadata
 */
struct BlobMetaDesc {
    std::string blob_name;
    fds_volid_t vol_id;
    blob_version_t version;
    fds_uint64_t blob_size;
    MetadataList meta_list;

    typedef boost::shared_ptr<BlobMetaDesc> ptr;
    typedef boost::shared_ptr<const BlobMetaDesc> const_ptr;

    /**
     * Constructs invalid BlobMetaDesc object, must initialize
     * to valid fields after the constructor.
     */
    BlobMetaDesc();
    virtual ~BlobMetaDesc();
};

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DMBLOBTYPES_H_

