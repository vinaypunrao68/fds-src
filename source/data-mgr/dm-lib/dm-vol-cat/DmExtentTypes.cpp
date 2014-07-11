/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <map>
#include <vector>
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
// Returns true if given offset is in range for this extent
//
fds_bool_t BlobExtent::offsetInRange(fds_uint64_t offset) const
{
    fds_uint32_t off_units = offset / offset_unit_bytes;
    return ((off_units >= first_offset) && (off_units <= last_offset));
}

//
// Updates object info 'obj_info' for given offset, if offset does
// not exist, adds offset to object info mapping
//
Error BlobExtent::updateOffset(fds_uint64_t offset,
                               const ObjectID& obj_info) {
    Error err(ERR_OK);
    fds_uint32_t off_units = toOffsetUnits(offset);

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
                                ObjectID* obj_info) const {
    Error err(ERR_OK);
    fds_uint32_t off_units = toOffsetUnits(offset);

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
// Removes all objects with offset > after_offset
//
fds_uint32_t BlobExtent::truncate(fds_uint64_t after_offset,
                                  std::vector<ObjectID>* ret_rm_list) {
    fds_verify(ret_rm_list);
    std::map<fds_uint32_t, ObjectID>::iterator it, rm_it;
    fds_bool_t found = false;
    fds_uint32_t last_off_units = toOffsetUnits(after_offset);
    fds_uint32_t num_objs_truncated = 0;

    for (it = blob_obj_list.begin();
         it != blob_obj_list.end();
         ++it) {
        if (it->first > last_off_units) {
            if (!found) {
                rm_it = it;
                found = true;
            }
            // is it possible to non-first obj be null obj?
            // if so, need to take that into account
            fds_verify(it->second != NullObjectID);
            (*ret_rm_list).push_back(it->second);
            num_objs_truncated++;
        }
    }
    blob_obj_list.erase(rm_it, blob_obj_list.end());
    return num_objs_truncated;
}

//
// We serialize offsets into as offsets in max_obj_size units
//
uint32_t BlobExtent::write(serialize::Serializer* s) const {
    uint32_t bytes = 0;
    std::map<fds_uint32_t, ObjectID>::const_iterator cit;

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
        ObjectID obj_info;
        bytes += d->readI32(off);
        bytes += obj_info.read(d);
        blob_obj_list[off] = obj_info;
    }
    return bytes;
}

std::ostream& operator<<(std::ostream& out, const BlobExtent& extent) {
    std::map<fds_uint32_t, ObjectID>::const_iterator cit;
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

void BlobExtent0::updateMetaData(const MetaDataList::const_ptr& meta_list) {
    for (MetaDataList::const_iter cit = meta_list->cbegin();
         cit != meta_list->cend();
         ++cit) {
        blob_meta.meta_list.updateMetaDataPair(cit->first, cit->second);
    }
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
    std::map<fds_uint32_t, ObjectID>::const_iterator cit;
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
