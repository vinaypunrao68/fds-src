/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <fds_process.h>
#include <object-store/SmSuperblock.h>

namespace fds {

SmSuperblock::SmSuperblock()
        : olt(new ObjectLocationTable()) {
}

SmSuperblock::~SmSuperblock() {
}

Error
SmSuperblock::loadSuperblock(const DiskIdSet& hddIds,
                             const DiskIdSet& ssdIds,
                             const DiskLocMap& diskMap,
                             SmTokenSet& smTokensOwned) {
    Error err(ERR_OK);

    // before we implement reading superblock from disk, validating, etc
    // we are just assuming we always come from clean state, so fill
    // in the datastructs and return
    ownedTokens.swap(smTokensOwned);
    SmTokenPlacement::compute(hddIds, ssdIds, olt);

    return err;
}

fds_uint16_t
SmSuperblock::getDiskId(fds_token_id smTokId,
                        diskio::DataTier tier) const {
    return olt->getDiskId(smTokId, tier);
}

SmTokenSet
SmSuperblock::getSmOwnedTokens() const {
    // we will return a copy
    SmTokenSet tokens;
    tokens.insert(ownedTokens.begin(), ownedTokens.end());
    return tokens;
}

uint32_t SmSuperblock::write(serialize::Serializer*  s) const {
    fds_uint32_t bytes = 0;
    bytes += olt->write(s);
    return bytes;
}

uint32_t SmSuperblock::read(serialize::Deserializer* d) {
    fds_uint32_t bytes = 0;
    bytes += olt->read(d);
    return bytes;
}

// So we can print class members for logging
std::ostream& operator<< (std::ostream &out,
                          const SmSuperblock& sb) {
    out << "SM Superblock:\n" << *(sb.olt);
    out << "SM tokens owned by this SM: {";
    if (sb.ownedTokens.size() == 0) out << "none";
    for (SmTokenSet::const_iterator cit = sb.ownedTokens.cbegin();
         cit != sb.ownedTokens.cend();
         ++cit) {
        if (cit != sb.ownedTokens.cbegin()) out << ", ";
        out << *cit;
    }
    out << "}\n";
    return out;
}


}  // namespace fds
