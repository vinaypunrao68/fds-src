/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <map>
#include <limits>
#include <DmBlobTypes.h>
#include <utility>
namespace fds {

//
// Constructs emty metadata list
//
MetaDataList::MetaDataList() {
}

//
// Constructs metadata list from fdsp metadata list
//
MetaDataList::MetaDataList(const fpi::FDSP_MetaDataList& mlist) {
    for (auto const& item : mlist) {
        insert(value_type(std::move(item.key), std::move(item.value)));
    }
}

MetaDataList::~MetaDataList() {
}

//
// Updates metadata value for the given key, or if key does not exist
// adds new key-value pair
//
void MetaDataList::updateMetaDataPair(const std::string& key,
                                      const std::string& value) {
    insert(value_type(key , value));
}

//
// Copies metadata into fdsp metadata list structure
//
void MetaDataList::toFdspPayload(fpi::FDSP_MetaDataList& mlist) const {
    mlist.clear();
    mlist.reserve(size());
    for (const auto& cit : *this) {
        fpi::FDSP_MetaDataPair mpair;
        mpair.key = cit.first;
        mpair.value = cit.second;
        mlist.push_back(mpair);
    }
}

MetaDataList & MetaDataList::merge(const MetaDataList & rhs) {
    for (auto elem : rhs) {
        (*this)[elem.first] = elem.second;
    }

    return *this;
}

uint32_t MetaDataList::write(serialize::Serializer* s) const {
    uint32_t bytes = 0;
    uint32_t size = (*this).size();
    bytes += s->writeI32(size);
    for (auto const & cit : *this) {
        bytes += s->writeString(cit.first);
        bytes += s->writeString(cit.second);
    }
    return bytes;
}

uint32_t MetaDataList::read(serialize::Deserializer* d) {
    uint32_t bytes = 0;
    uint32_t size = 0;
    (*this).clear();
    bytes += d->readI32(size);
    for (fds_uint32_t i = 0; i < size; ++i) {
        std::string key;
        std::string value;
        bytes += d->readString(key);
        bytes += d->readString(value);
        insert(value_type(key , value));
    }
    return bytes;
}

MetaDataList& MetaDataList::operator=(const MetaDataList &rhs) {
    // Base map assignment
    std::map<std::string, std::string>::operator=(rhs);
    return *this;
}

bool MetaDataList::operator==(const MetaDataList& rhs) const {
    bool ret = (size() == rhs.size() &&
            std::equal(begin(), end(), rhs.begin()));
    return ret;
}

std::ostream& operator<<(std::ostream& out, const MetaDataList& metaList) {
    out << "Metadata: ";
    if (metaList.size() == 0) {
        out << "[empty]\n";
        return out;
    }

    for (auto const& cit : metaList) {
        out << "[" << cit.first << ":" << cit.second << "]";
    }
    return out;
}

//---------- BasicBlobMeta implementation  -------------//

BasicBlobMeta::BasicBlobMeta() {
    version = blob_version_invalid;
    sequence_id = 0;
    blob_size = 0;
}

BasicBlobMeta::~BasicBlobMeta() {
}

uint32_t BasicBlobMeta::write(serialize::Serializer* s) const {
    uint32_t bytes = 0;
    bytes += s->writeString(blob_name);
    bytes += s->writeI64(version);
    bytes += s->writeI64(sequence_id);
    bytes += s->writeI64(blob_size);
    return bytes;
}

uint32_t BasicBlobMeta::read(serialize::Deserializer* d) {
    uint32_t bytes = 0;
    blob_name.clear();
    bytes += d->readString(blob_name);
    bytes += d->readI64(version);
    bytes += d->readI64(sequence_id);
    bytes += d->readI64(blob_size);
    return bytes;
}

bool BasicBlobMeta::operator==(const BasicBlobMeta& rhs) const {
    bool ret = (blob_name == rhs.blob_name &&
                version == rhs.version &&
                sequence_id == rhs.sequence_id &&
                blob_size == rhs.blob_size);
    return ret;
}

std::ostream& operator<<(std::ostream& out, const BasicBlobMeta& desc) {
    out << "BasicBlobMeta: "
        << "name " << desc.blob_name
        << ", version " << desc.version
        << ", sequence ID " << desc.sequence_id
        << ", size " << desc.blob_size << " bytes; ";
    return out;
}

//---------   BlobMetaDesc implementation   -----------//


/**
 * Constructs invalid version of a blob meta
 * until someone else actually assigns valid data
 */
BlobMetaDesc::BlobMetaDesc()
        : desc(), meta_list() {
}

BlobMetaDesc::~BlobMetaDesc() {
}

uint32_t BlobMetaDesc::write(serialize::Serializer* s) const {
    uint32_t bytes = 0;
    bytes += desc.write(s);
    bytes += meta_list.write(s);
    return bytes;
}

uint32_t BlobMetaDesc::read(serialize::Deserializer* d) {
    uint32_t bytes = 0;
    bytes += desc.read(d);
    bytes += meta_list.read(d);
    return bytes;
}

BlobMetaDesc& BlobMetaDesc::operator=(const BlobMetaDesc &rhs) {
    desc      = rhs.desc;
    meta_list = rhs.meta_list;
    return *this;
}

bool BlobMetaDesc::operator==(const BlobMetaDesc &rhs) const {
    bool ret = (desc == rhs.desc &&
                meta_list == rhs.meta_list);
    return ret;
}

std::ostream& operator<<(std::ostream& out, const BlobMetaDesc& blobMetaDesc) {
    out << "BlobMeta: ";
    out << blobMetaDesc.desc;
    out << blobMetaDesc.meta_list;
    return out;
}

//----------- BlobObjInfo implementation ---------------//

BlobObjInfo::BlobObjInfo()
        : size(0) {
}

BlobObjInfo::BlobObjInfo(const ObjectID& obj_id, fds_uint64_t sz)
        : oid(obj_id), size(sz) {
}

BlobObjInfo::BlobObjInfo(const fpi::FDSP_BlobObjectInfo& obj_info) {
    oid.SetId((const char*)obj_info.data_obj_id.digest.c_str(),
              obj_info.data_obj_id.digest.length());
    size = obj_info.size;
}

BlobObjInfo::~BlobObjInfo() {
}

BlobObjInfo& BlobObjInfo::operator= (const BlobObjInfo &rhs) {
    oid = rhs.oid;
    size = rhs.size;
    return *this;
}

uint32_t BlobObjInfo::write(serialize::Serializer* s) const {
    uint32_t bytes = 0;
    bytes += oid.write(s);
    bytes += s->writeI64(size);
    return bytes;
}

uint32_t BlobObjInfo::read(serialize::Deserializer* d) {
    uint32_t bytes = 0;
    bytes += oid.read(d);
    bytes += d->readI64(size);
    return bytes;
}


//----------- BlobObjList implementation ---------------//

BlobObjList::BlobObjList() {
    end_of_blob = false;
}

BlobObjList::BlobObjList(const fpi::FDSP_BlobObjectList& blob_obj_list) {
    end_of_blob = false;
    for (fds_uint32_t i = 0; i < blob_obj_list.size(); ++i) {
        if (blob_obj_list[i].size == 0) {
            BlobObjInfo obj_info(NullObjectID, 0);
            (*this)[blob_obj_list[i].offset] = obj_info;
        } else {
            BlobObjInfo obj_info(blob_obj_list[i]);
            (*this)[blob_obj_list[i].offset] = obj_info;
        }
    }
}

BlobObjList::~BlobObjList() {
}

//
// Merge itself with another blob object list and return itself
//
BlobObjList& BlobObjList::merge(const BlobObjList& newer_blist){
    // once end_of_blob is set, do not allow merges
    fds_verify(end_of_blob == false);
    for (auto const & cit : newer_blist) {
        insert(value_type(cit.first, cit.second));
    }
    return *this;
}

//
// Returns last offset in the obj list
//
fds_uint64_t BlobObjList::lastOffset() const {
    std::map<fds_uint64_t, BlobObjInfo>::const_reverse_iterator crit;
    if (size() == 0)
        return std::numeric_limits<fds_uint64_t>::max();

    crit = crbegin();
    fds_verify(crit != crend());
    return crit->first;
}

//
// Merge itself with one object
//
void BlobObjList::updateObject(fds_uint64_t offset,
                               const ObjectID& oid,
                               fds_uint64_t size) {
    BlobObjInfo obj_info(oid, size);
    (*this)[offset] = obj_info;
}

//
// Verifies each obj is max size; if end_of_blob is set
// last object can be less than max size; also that each offset
// is aligned.
//
Error BlobObjList::verify(fds_uint32_t max_obj_size) const {
    std::map<fds_uint64_t, BlobObjInfo>::const_reverse_iterator crit;
    crit = crbegin();
    if (crit == crend()) {
        LOGERROR << "Object list is empty";
        return ERR_DM_OP_NOT_ALLOWED;
    }

    // Check the last object is either truncating the blob or is of
    // max_obj_size and that it is aligned.
    //    if (!end_of_blob && (crit->second).size != max_obj_size) {
    //    LOGERROR << "Object list ends with short object but is not a truncation";
    //    return ERR_DM_OP_NOT_ALLOWED;
    // } else if (0 < (crit->first % max_obj_size)) {
    //    LOGERROR << "Object list ends with an unaligned object at: 0x"
    //             << std::hex << crit->first;
    //    return ERR_DM_OP_NOT_ALLOWED;
    // }

    // ++crit;
    // while (crit != crend()) {
    //    if ((crit->second).size != max_obj_size ||
    //        (0 < (crit->first % max_obj_size))) {
    //        LOGERROR << "Invalid short object of size: " << crit->second.size;
    //        return ERR_DM_OP_NOT_ALLOWED;
    //    }
    //    ++crit;
    //}
    return ERR_OK;
}

//
// fills in blob_obj_list
//
void BlobObjList::toFdspPayload(fpi::FDSP_BlobObjectList& blob_obj_list) const {
    blob_obj_list.clear();
    for (auto const& cit : *this) {
        fpi::FDSP_BlobObjectInfo obj_info;
        obj_info.offset = cit.first;
        obj_info.size = (cit.second).size;
        obj_info.data_obj_id.digest = std::string((const char *)((cit.second).oid.GetId()),
                                                  (size_t)(cit.second).oid.GetLen());
        blob_obj_list.push_back(obj_info);
    }
}

uint32_t BlobObjList::write(serialize::Serializer* s) const {
    uint32_t bytes = 0;
    bytes += s->writeI32(end_of_blob);
    bytes += s->writeI32(size());
    for (auto const & cit : *this) {
        bytes += s->writeI64(cit.first);
        bytes += (cit.second).write(s);
    }
    return bytes;
}

uint32_t BlobObjList::read(serialize::Deserializer* d) {
    uint32_t bytes = 0;
    uint32_t sz = 0;
    clear();
    bytes += d->readI32(reinterpret_cast<int32_t&>(end_of_blob));
    bytes += d->readI32(sz);
    for (fds_uint32_t i = 0; i < sz; ++i) {
        fds_uint64_t offset;
        BlobObjInfo obj_info;
        bytes += d->readI64(offset);
        bytes += obj_info.read(d);
        (*this)[offset] = obj_info;
    }
    return bytes;
}

std::ostream& operator<<(std::ostream& out, const BlobObjList& obj_list) {
    out << "Object list \n";
    for (auto const& cit : obj_list) {
        out << "[offset " << cit.first << " -> " << (cit.second).oid
            << "," << (cit.second).size << "]\n";
    }
    out << " End of blob? " << obj_list.endOfBlob() << "\n";
    return out;
}

//---------   VolumeMetaDesc implementation   -----------//

/**
 * Constructs invalid version of a volume metadata object
 * until someone else actually assigns valid data
 */
VolumeMetaDesc::VolumeMetaDesc(const fpi::FDSP_MetaDataList &metadataList,
                               const sequence_id_t seq_id)
        : meta_list(metadataList) {
        sequence_id = seq_id;
}

VolumeMetaDesc::~VolumeMetaDesc() = default;

uint32_t VolumeMetaDesc::write(serialize::Serializer* s) const {
    uint32_t bytes = 0;
    // bytes += desc.write(s);
    bytes += s->writeI64(sequence_id);
    bytes += meta_list.write(s);
    return bytes;
}

uint32_t VolumeMetaDesc::read(serialize::Deserializer* d) {
    uint32_t bytes = 0;
    // bytes += desc.read(d);
    bytes += d->readI64(sequence_id);
    bytes += meta_list.read(d);
    return bytes;
}

VolumeMetaDesc& VolumeMetaDesc::operator=(const VolumeMetaDesc &rhs) = default;

bool VolumeMetaDesc::operator==(const VolumeMetaDesc& rhs) const {
    return meta_list == rhs.meta_list;
}

std::ostream& operator<<(std::ostream& out, const VolumeMetaDesc& blobMetaDesc) {
    out << "VolumeMeta: sequence id:" << blobMetaDesc.sequence_id;
    // out << blobMetaDesc.desc;
    out << blobMetaDesc.meta_list;
    return out;
}

}  // namespace fds
