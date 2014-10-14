/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <set>
#include <object-store/SmTokenPlacement.h>

namespace fds {

const fds_uint8_t ObjLocationTablePoison = 0xff;
const fds_uint16_t fds_diskid_invalid = 0xffff;

ObjectLocationTable::ObjectLocationTable() {
    memset(this, ObjLocationTablePoison, sizeof(*this));
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
ObjectLocationTable::isDiskIdValid(fds_uint16_t diskId) {
    return (diskId != fds_diskid_invalid);
}

SmTokenSet
ObjectLocationTable::getSmTokens(fds_uint16_t diskId) const {
    SmTokenSet tokens;
    fds_verify(diskId != fds_diskid_invalid);
    for (fds_uint32_t i = 0; i < SM_TIER_COUNT; ++i) {
        for (fds_token_id tokId = 0; tokId < SMTOKEN_COUNT; ++tokId) {
            if (table[i][tokId] == diskId) {
                tokens.insert(tokId);
            }
        }
    }
    return tokens;
}

Error
ObjectLocationTable::validate(const std::set<fds_uint16_t>& diskIdSet,
                              diskio::DataTier tier) const {
    Error err(ERR_OK);
    // build set of disks IDs that are currently in OLT for a given tier
    // and check if OLT contains disks that are not in diskIdSet
    std::set<fds_uint16_t> oltDisks;
    for (fds_token_id tokId = 0; tokId < SMTOKEN_COUNT; ++tokId) {
        fds_uint16_t diskId = getDiskId(tokId, tier);
        if (!isDiskIdValid(diskId)) {
            continue;  // empty cell
        }

        // TODO(Anna) we should also verify that once we have one 'empty'
        // cell, all cells in that row must be empty!
        // check if this disks are in diskIdSet
        //
        if (diskIdSet.count(diskId) == 0) {
            return ERR_SM_OLT_DISKS_INCONSISTENT;
        }
        oltDisks.insert(diskId);
    }
    // check if disks found in OLT for a given tier are also in diskIdSet
    for (std::set<fds_uint16_t>::const_iterator cit = diskIdSet.cbegin();
         cit != diskIdSet.cend();
         ++cit) {
        if (oltDisks.count(*cit) == 0) {
            return ERR_SM_OLT_DISKS_INCONSISTENT;
        }
    }
    return err;
}

fds_bool_t
ObjectLocationTable::operator ==(const ObjectLocationTable& rhs) const {
    if (this == &rhs) {
        return true;
    }

    return (0 == memcmp(table,
                        rhs.table,
                        sizeof(table)));
}

std::ostream& operator<< (std::ostream &out,
                          const ObjectLocationTable& tbl) {
    out << "Object Location Table:\n";
    for (fds_uint32_t i = 0; i < SM_TIER_COUNT; ++i) {
        if (i == 0) {
            out << "HDD: ";
        } else {
            out << "SSD: ";
        }
        for (fds_uint32_t j = 0; j < SMTOKEN_COUNT; ++j) {
            out << "[" << tbl.table[i][j] << "] ";
        }
        out << "\n";
    }
    return out;
}

std::ostream& operator<< (std::ostream &out,
                          const SmTokenSet& toks) {
    out << "tokens {";
    if (toks.size() == 0) out << "none";
    for (SmTokenSet::const_iterator cit = toks.cbegin();
         cit != toks.cend();
         ++cit) {
        if (cit != toks.cbegin()) {
            out << ", ";
        }
        out << *cit;
    }
    out << "}\n";
    return out;
}

/**** SmTokenPlacement implementation ***/

void
SmTokenPlacement::compute(const std::set<fds_uint16_t>& hdds,
                          const std::set<fds_uint16_t>& ssds,
                          ObjectLocationTable* olt) {
    fds_verify(olt);

    // use round-robin placement of tokens over disks
    // first assign placement on HDDs
    std::set<fds_uint16_t>::const_iterator cit = hdds.cbegin();
    if (cit !=hdds.cend()) {
        for (fds_token_id tok = 0; tok < SMTOKEN_COUNT; ++tok) {
            olt->setDiskId(tok, diskio::diskTier, *cit);
            ++cit;
            if (cit == hdds.cend()) {
                cit = hdds.cbegin();
            }
        }
    }

    // assign placement on SSDs
    cit = ssds.cbegin();
    if (cit != ssds.cend()) {
        for (fds_token_id tok = 0; tok < SMTOKEN_COUNT; ++tok) {
            olt->setDiskId(tok, diskio::flashTier, *cit);
            ++cit;
            if (cit == ssds.cend()) {
                cit = ssds.cbegin();
            }
        }
    }
}

}  // namespace fds
