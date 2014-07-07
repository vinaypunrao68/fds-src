/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <map>
#include <dm-vol-cat/DmExtentTypes.h>

namespace fds {

//------ BlobExtent implementation ------------//

//
// Constructs extent with empty blob object list
//
BlobExtent::BlobExtent(fds_extent_id ext_id,
                       fds_uint32_t max_obj_size,
                       fds_uint32_t first_off,
                       fds_uint32_t num_off)
        : extent_id(ext_id),
          offset_unit_bytes(max_obj_size),
          first_offset(first_off) {
    fds_verify(num_off > 0);
    last_offset = first_offset + num_off - 1;
}

BlobExtent::~BlobExtent() {
}

//
// Updates object info 'obj_info' for given offset, if offset does
// not exist, adds offset to object info mapping
//
Error BlobExtent::updateOffset(fds_uint64_t offset,
                               const BlobObjInfo& obj_info) {
    Error err(ERR_OK);
    fds_uint32_t off_units = offset / offset_unit_bytes;
    // must be max obj size aligned (until we add support for
    // unaligned offsets in DM)
    fds_verify((offset % offset_unit_bytes) == 0);
    // make sure offset belongs to this blob extent
    if ((off_units < first_offset) || (off_units > last_offset)) {
        return ERR_DM_OFFSET_OUT_RANGE;
    }

    blob_obj_list[off_units] = obj_info;
    return err;
}

//
// Retrieves object info at the give offset into 'obj_info'
//
Error BlobExtent::getObjectInfo(fds_uint64_t offset,
                                BlobObjInfo* obj_info) const {
    Error err(ERR_OK);
    fds_uint32_t off_units = offset / offset_unit_bytes;
    // must be max obj size aligned (until we add support for
    // unaligned offsets in DM)
    fds_verify((offset % offset_unit_bytes) == 0);
    // make sure offset belongs to this blob extent
    if ((off_units < first_offset) || (off_units > last_offset)) {
        return ERR_DM_OFFSET_OUT_RANGE;
    }
    if (blob_obj_list.count(off_units) == 0) {
        return ERR_NOT_FOUND;
    }
    *obj_info = blob_obj_list.at(off_units);
    return err;
}

//
// We serialize offsets into as offsets in max_obj_size units
//
uint32_t BlobExtent::write(serialize::Serializer* s) const {
    uint32_t bytes = 0;
    std::map<fds_uint32_t, BlobObjInfo>::const_iterator cit;

    bytes += s->writeI32(blob_obj_list.size());
    for (cit = blob_obj_list.cbegin();
         cit != blob_obj_list.cend();
         ++cit) {
        bytes += s->writeI32(cit->first);
        bytes += (cit->second).write(s);
    }
    return bytes;
}

uint32_t BlobExtent::read(serialize::Deserializer* d) {
    uint32_t bytes = 0;
    uint32_t sz = 0;
    blob_obj_list.clear();

    bytes += d->readI32(sz);
    for (fds_uint32_t i = 0; i < sz; ++i) {
        fds_uint32_t off;
        BlobObjInfo obj_info;
        bytes += d->readI32(off);
        bytes += obj_info.read(d);
        blob_obj_list[off] = obj_info;
    }
    return bytes;
}

std::ostream& operator<<(std::ostream& out, const BlobExtent& extent) {
    std::map<fds_uint32_t, BlobObjInfo>::const_iterator cit;
    out << "Extent " << extent.getExtentId() << "\n";
    for (cit = extent.blob_obj_list.cbegin();
         cit != extent.blob_obj_list.cend();
         ++cit) {
        fds_uint64_t offset = cit->first * extent.offset_unit_bytes;
        out << "[offset " << offset << " -> " << cit->second << "]\n";
    }
    return out;
}

//--------- BlobExtent0 implementatin ----------//

//
// Constructs extent 0 with uninitialized entries
// except blob name and volume id
//
BlobExtent0::BlobExtent0(const std::string& blob_name,
                         fds_volid_t volume_id,
                         fds_uint32_t max_obj_size,
                         fds_uint32_t first_off,
                         fds_uint32_t num_offsets)
: BlobExtent(fds_extentid_meta, max_obj_size, first_off, num_offsets) {
    blob_meta.blob_name = blob_name;
    blob_meta.vol_id = volume_id;
}

BlobExtent0::~BlobExtent0() {
}

uint32_t BlobExtent0::write(serialize::Serializer* s) const {
    uint32_t bytes = 0;
    // first write meta part
    bytes += blob_meta.write(s);
    // then offsets -> obj info entries
    bytes += BlobExtent::write(s);
    return bytes;
}

uint32_t BlobExtent0::read(serialize::Deserializer* d) {
    uint32_t bytes = 0;
    // first read meta part
    bytes += blob_meta.read(d);
    // then read offset -> obj info entries
    bytes += BlobExtent::read(d);
    return bytes;
}

std::ostream& operator<<(std::ostream& out, const BlobExtent0& extent0) {
    std::map<fds_uint32_t, BlobObjInfo>::const_iterator cit;
    out << "Extent 0 for " << extent0.blob_meta << "; Object List \n";
    for (cit = extent0.blob_obj_list.cbegin();
         cit != extent0.blob_obj_list.cend();
         ++cit) {
        fds_uint64_t offset = cit->first * extent0.offset_unit_bytes;
        out << "[offset " << offset << " -> " << cit->second << "]\n";
    }
    return out;
}

}  // namespace fds
