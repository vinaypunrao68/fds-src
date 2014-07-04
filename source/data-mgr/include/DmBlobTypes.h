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
#include <fds_types.h>
#include <fds_volume.h>

namespace fds {

    /**
     * List of metadata key-value pairs
     * Implemented as a map so we can easily search for particular key
     */
    struct MetaDataList :
            std::unordered_map<std::string, std::string>,
            serialize::Serializable {
        typedef std::unordered_map<
                std::string,
                std::string>::const_iterator const_iter;

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

        uint32_t write(serialize::Serializer* s) const;
        uint32_t read(serialize::Deserializer* d);
    };

    /**
     * typedef BlobObjInfo so that we can make BlobObjInfo into struct later
     * if we need to have more fields in object info than just ObjectID without
     * changes in API
     */
    typedef ObjectID BlobObjInfo;

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
         * Update offset to object info mapping, if offset does not exit,
         * adds as new offset to object info mapping
         */
        void updateObject(fds_uint64_t offset, const BlobObjInfo& obj_info);

        /**
         * Merges itself with another BlobObjList list and returns itself
         * If newer_blist contains an offset which already exists in this
         * list, it will over-write this offset's object info with object
         * info from newer_list
         */
        BlobObjList& merge(const BlobObjList& newer_blist);

        uint32_t write(serialize::Serializer* s) const;
        uint32_t read(serialize::Deserializer* d);
    };


    /**
     * Metadata that describes the blob, not including the
     * the offset to object id mappings list of the metadata
     */
    struct BlobMetaDesc: serialize::Serializable {
        std::string blob_name;
        fds_volid_t vol_id;
        blob_version_t version;
        fds_uint64_t blob_size;
        MetaDataList meta_list;

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
    };

    std::ostream& operator<<(std::ostream& out, const MetaDataList& metaList);
    std::ostream& operator<<(std::ostream& out, const BlobMetaDesc& blobMetaDesc);
    std::ostream& operator<<(std::ostream& out, const BlobObjList& obj_list);

    namespace BlobUtil {
        void toFDSPQueryCatalogMsg(const BlobMetaDesc::const_ptr& blob_meta_desc,
                                   const BlobObjList::const_ptr& blob_obj_list,
                                   fds_uint32_t max_obj_size_bytes,
                                   fpi::FDSP_QueryCatalogTypePtr& query_msg);
    }  // namespace BlobUtil

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DMBLOBTYPES_H_

