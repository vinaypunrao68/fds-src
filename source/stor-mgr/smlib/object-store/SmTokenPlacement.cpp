/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <set>
#include <object-store/SmTokenPlacement.h>

namespace fds {

#define fds_diskid_invalid 0xffff

ObjectLocationTable::ObjectLocationTable() {
    // initialize table with invalid disk ids
    for (fds_uint32_t i = 0; i < SM_TIER_COUNT; i++) {
        for (fds_token_id tokId = 0; tokId < SMTOKEN_COUNT; tokId++) {
            table[i][tokId] = fds_diskid_invalid;
        }
    }
}

ObjectLocationTable::~ObjectLocationTable() {
}

void
ObjectLocationTable::setDiskId(fds_token_id smToken,
                               diskio::DataTier tier,
                               fds_uint16_t diskId) {
    fds_verify(smToken < SMTOKEN_COUNT);
    // here we are explicit with translation of tier to row number
    // if tier enum changes ....
    if (tier == diskio::diskTier) {
        table[0][smToken] = diskId;
    } else if (tier == diskio::flashTier) {
        table[1][smToken] = diskId;
    } else {
        fds_panic("Unknown tier set to object location table\n");
    }
}

fds_uint16_t
ObjectLocationTable::getDiskId(fds_token_id smToken,
                               diskio::DataTier tier) const {
    fds_verify(smToken < SMTOKEN_COUNT);
    // here we are explicit with translation of tier to row number
    // if tier enum changes ....
    if (tier == diskio::diskTier) {
        return table[0][smToken];
    } else if (tier == diskio::flashTier) {
        return table[1][smToken];
    }
    fds_panic("Unknown tier request from object location table\n");
    return 0;
}

fds_bool_t
ObjectLocationTable::isDiskIdValid(fds_uint16_t diskId) const {
    return (diskId != fds_diskid_invalid);
}

void
ObjectLocationTable::generateDiskToSmTokenMap() {
    // clear the current map
    diskSmTokenMap.clear();

    // generate from sm token to disk id table
    for (fds_uint32_t i = 0; i < SM_TIER_COUNT; i++) {
        for (fds_token_id tokId = 0; tokId < SMTOKEN_COUNT; tokId++) {
            fds_uint32_t disk_id = table[i][tokId];
            if (disk_id == fds_diskid_invalid) continue;
            // we should not have a case when we find more than one
            // token for same disk id
            fds_verify(diskSmTokenMap[disk_id].count(tokId) == 0);
            diskSmTokenMap[disk_id].insert(tokId);
        }
    }
}

SmTokenSet
ObjectLocationTable::getSmTokens(fds_uint16_t diskId) const {
    SmTokenSet tokens;
    if (diskSmTokenMap.count(diskId) == 0) return tokens;
    const SmTokenSet& tok_set = diskSmTokenMap.at(diskId);
    for (SmTokenSet::const_iterator cit = tok_set.cbegin();
         cit != tok_set.cend();
         ++cit) {
        tokens.insert(*cit);
    }
    return tokens;
}

uint32_t
ObjectLocationTable::write(serialize::Serializer*  s) const {
    fds_uint32_t bytes = 0;
    for (fds_uint32_t i = 0; i < SM_TIER_COUNT; i++) {
        for (fds_uint32_t j = 0; j < SMTOKEN_COUNT; j++) {
            bytes += s->writeI32(table[i][j]);
        }
    }
    return bytes;
}

uint32_t
ObjectLocationTable::read(serialize::Deserializer* d) {
    fds_uint32_t bytes = 0;
    for (fds_uint32_t i = 0; i < SM_TIER_COUNT; i++) {
        for (fds_uint32_t j = 0; j < SMTOKEN_COUNT; j++) {
            fds_int32_t disk_id = 0;
            bytes += d->readI32(disk_id);
            table[i][j] = disk_id;
        }
    }

    // since we just changed the whole table, re-generate
    // reverse disk to sm tokens map
    generateDiskToSmTokenMap();

    return bytes;
}

fds_bool_t
ObjectLocationTable::operator ==(const ObjectLocationTable& rhs) const {
    if (this == &rhs) return true;
    return (0 == memcmp(
        table, rhs.table, SM_TIER_COUNT * SMTOKEN_COUNT * sizeof(fds_uint16_t)));
}

std::ostream& operator<< (std::ostream &out,
                          const ObjectLocationTable& tbl) {
    out << "Object Location Table:\n";
    for (fds_uint32_t i = 0; i < SM_TIER_COUNT; i++) {
        if (i == 0) {
            out << "HDD: ";
        } else {
            out << "SSD: ";
        }
        for (fds_uint32_t j = 0; j < SMTOKEN_COUNT; j++) {
            out << "[" << tbl.table[i][j] << "] ";
        }
        out << "\n";
    }
    out << "Disk to token ids map:\n";
    if (tbl.diskSmTokenMap.size() == 0) out << "[EMPTY]";
    for (DiskSmTokenMap::const_iterator map_it = tbl.diskSmTokenMap.cbegin();
         map_it != tbl.diskSmTokenMap.cend();
         ++map_it) {
        out << " disk_id " << map_it->first << " tokens {";
        for (SmTokenSet::const_iterator cit = (map_it->second).cbegin();
             cit != (map_it->second).cend();
             ++cit) {
            if (cit != (map_it->second).cbegin()) out << ", ";
            out << *cit;
        }
        out << "}\n";
    }
    return out;
}

/**** SmTokenPlacement implementation ***/

void
SmTokenPlacement::compute(const std::set<fds_uint16_t>& hdds,
                          const std::set<fds_uint16_t>& ssds,
                          ObjectLocationTable::ptr olt) {
    // use round-robin placement of tokens over disks
    // first assign placement on HDDs
    std::set<fds_uint16_t>::const_iterator cit = hdds.cbegin();
    if (cit !=hdds.cend()) {
        for (fds_token_id tok = 0; tok < SMTOKEN_COUNT; tok++) {
            olt->setDiskId(tok, diskio::diskTier, *cit);
            ++cit;
            if (cit == hdds.cend()) cit = hdds.cbegin();
        }
    }

    // assign placement on SSDs
    cit = ssds.cbegin();
    if (cit != ssds.cend()) {
        for (fds_token_id tok = 0; tok < SMTOKEN_COUNT; tok++) {
            olt->setDiskId(tok, diskio::flashTier, *cit);
            ++cit;
            if (cit == ssds.cend()) cit = ssds.cbegin();
        }
    }

    // generate reverse map of disk to tokens
    olt->generateDiskToSmTokenMap();
}

}  // namespace fds
