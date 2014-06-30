/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <DmBlobTypes.h>

namespace fds {

/**
 * Constructs invalid version of a blob meta
 * until someone else actually assigns valid data
 */
BlobMetaDesc::BlobMetaDesc() {
    version = blob_version_invalid;
    vol_id = invalid_vol_id;
    blob_size = 0;
    meta_list.clear();
}

BlobMetaDesc::~BlobMetaDesc() {
}

void BlobMetaDesc::mkFDSPMetaDataList(fpi::FDSP_MetaDataList& mlist) const {
    mlist.clear();
    for (BlobKeyValue::const_iterator cit = meta_list.cbegin();
         cit != meta_list.cend();
         ++cit) {
        fpi::FDSP_MetaDataPair mpair;
        mpair.key = cit->first;
        mpair.value = cit->second;
        mlist.push_back(mpair);
    }
}

BlobObjList::BlobObjList() {
}

BlobObjList::BlobObjList(const fpi::FDSP_BlobObjectList& blob_obj_list) {
    for (fds_uint32_t i = 0; i < blob_obj_list.size(); ++i) {
        ObjectID oid;
        fds_uint64_t offset = blob_obj_list[i].offset;
        oid.SetId((const char*)blob_obj_list[i].data_obj_id.digest.c_str(),
                  blob_obj_list[i].data_obj_id.digest.length());
        (*this)[offset] = oid;
    }
}

BlobObjList::~BlobObjList() {
}

//---------------- BlobUtil methods -----------------------------------------//

void BlobUtil::toFDSPQueryCatalogMsg(const BlobMetaDesc::const_ptr& blob_meta_desc,
                                     const BlobObjList::const_ptr& blob_obj_list,
                                     fds_uint32_t max_obj_size_bytes,
                                     fpi::FDSP_QueryCatalogTypePtr& query_msg) {
    query_msg->blob_name = blob_meta_desc->blob_name;
    query_msg->blob_size = blob_meta_desc->blob_size;
    query_msg->blob_version = blob_meta_desc->version;
    blob_meta_desc->mkFDSPMetaDataList(query_msg->meta_list);

    // fill in blob_obj_list
    (query_msg->obj_list).clear();
    for (BlobObjList::const_it cit = blob_obj_list->cbegin();
         cit != blob_obj_list->cend();
         ++cit) {
        fpi::FDSP_BlobObjectInfo obj_info;
        obj_info.offset = cit->first;
        obj_info.data_obj_id.digest =
                std::string((const char *)(cit->second.GetId()),
                            (size_t)cit->second.GetLen());
        obj_info.size = max_obj_size_bytes;
        if ((query_msg->obj_list).size() == (blob_obj_list->size() - 1)) {
            // this is the last object, object size = blob size % max obj size
            obj_info.size = blob_meta_desc->blob_size % max_obj_size_bytes;
        }
        (query_msg->obj_list).push_back(obj_info);
    }
}

}  // namespace fds
