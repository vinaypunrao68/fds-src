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
     * A list of offset to Object ID mappings
     */
    struct BlobObjList :
            std::unordered_map<fds_uint64_t, ObjectID>,
            serialize::Serializable {
        typedef boost::shared_ptr<BlobObjList> ptr;
        typedef boost::shared_ptr<const BlobObjList> const_ptr;
        typedef std::unordered_map<
                fds_uint64_t,
                ObjectID>::const_iterator const_iter;

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
         * Update offset to object id mapping, if offset does not exit,
         * adds as new offset to object id mapping
         */
        void updateObject(fds_uint64_t offset, const ObjectID& oid);

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

