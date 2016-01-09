/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <set>
#include <SmTypes.h>
#include <object-store/SmTokenPlacement.h>

namespace fds {

const fds_uint8_t ObjLocationTablePoison = 0xff;
/**
 * Below consts are tightly coupled to tier enum
 * Any changes to one should be made to other as well.
 * TODO(Gurpreet): Cleanup. Remove fds_hdd_* consts
 * from the code and use tier enum instead.
 */
const fds_uint16_t fds_hdd_row = 0;
const fds_uint16_t fds_ssd_row = 1;
const DiskId DEFAULT_DISKIDX = 0;

ObjectLocationTable::ObjectLocationTable() {
    memset(this, ObjLocationTablePoison, sizeof(*this));
}

void
ObjectLocationTable::setDiskId(fds_token_id smToken,
                               diskio::DataTier tier,
                               fds_uint16_t diskId) {
    fds_verify(smToken < SMTOKEN_COUNT);
    // here we are explicit with translation of tier to row number
    // if tier enum changes ....
    if (tier == diskio::diskTier) {
        table[fds_hdd_row][smToken][DEFAULT_DISKIDX] = diskId;
    } else if (tier == diskio::flashTier) {
        table[fds_ssd_row][smToken][DEFAULT_DISKIDX] = diskId;
    } else {
        fds_panic("Unknown tier set to object location table\n");
    }
}

void
ObjectLocationTable::addDiskId(fds_token_id smToken,
                               diskio::DataTier tier,
                               fds_uint16_t diskId) {
    fds_verify(smToken < SMTOKEN_COUNT);
    auto idx = DEFAULT_DISKIDX;
    while (table[tier][smToken][idx] != SM_INVALID_DISK_ID &&
           idx < MAX_HOST_DISKS) {
        idx++;
    }
    if (idx >= MAX_HOST_DISKS) {
        LOGERROR << "Failed to add new disk: " << diskId
                 << " for token: " << smToken
                 << " in tier: " << tier
                 << ".Reached disk fanout quota for SM Token.";
        //TODO(Gurpreet): Gracefully handle the error here.
        return;
    }
    // here we are explicit with translation of tier to row number
    // if tier enum changes ....
    if (tier == diskio::diskTier) {
        table[fds_hdd_row][smToken][idx] = diskId;
    } else if (tier == diskio::flashTier) {
        table[fds_ssd_row][smToken][idx] = diskId;
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
        return table[fds_hdd_row][smToken][DEFAULT_DISKIDX];
    } else if (tier == diskio::flashTier) {
        return table[fds_ssd_row][smToken][DEFAULT_DISKIDX];
    }
    fds_panic("Unknown tier request from object location table\n");
    return 0;
}

std::set<DiskId>
ObjectLocationTable::getDiskIds(fds_token_id smToken,
                                diskio::DataTier tier) const {
    std::set<DiskId> disks;
    if (tier == diskio::diskTier) {
        auto idx = DEFAULT_DISKIDX;
        while (table[fds_hdd_row][smToken][idx] != SM_INVALID_DISK_ID &&
               idx < MAX_HOST_DISKS) {
            disks.insert(table[fds_hdd_row][smToken][idx]);
            idx++;
        }
    } else if (tier == diskio::flashTier) {
        auto idx = DEFAULT_DISKIDX;
        while (table[fds_ssd_row][smToken][idx] != SM_INVALID_DISK_ID &&
               idx < MAX_HOST_DISKS) {
            disks.insert(table[fds_ssd_row][smToken][idx]);
            idx++;
        }
    }
    return disks;
}

fds_bool_t
ObjectLocationTable::isDiskIdValid(fds_uint16_t diskId) {
    return (diskId != SM_INVALID_DISK_ID);
}

SmTokenSet
ObjectLocationTable::getSmTokens(fds_uint16_t diskId) const {
    SmTokenSet tokens;
    fds_verify(diskId != SM_INVALID_DISK_ID);
    for (fds_uint32_t i = 0; i < SM_TIER_COUNT; ++i) {
        for (fds_token_id tokId = 0; tokId < SMTOKEN_COUNT; ++tokId) {
            for (DiskId idx = DEFAULT_DISKIDX; idx < MAX_HOST_DISKS; idx++) {
                if (table[i][tokId][idx] == diskId) {
                    tokens.insert(tokId);
                }
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

/*
 * Get set of disks from the specified tier (HDD or SSD) in the OLT
 * record.  There can be multiple entry of the same disk ID, but
 * std::set should filter out duplicate entries.
 */
std::set<uint16_t>
ObjectLocationTable::getDiskSet(diskio::DataTier tier)
{
    uint16_t rowOffset;
    std::set<uint16_t> diskSet;

    if (tier == diskio::diskTier) {
        rowOffset = fds_hdd_row;
    } else if (tier == diskio::flashTier) {
        rowOffset = fds_ssd_row;
    } else {
        fds_panic("Unknown tier");
    }

    for (fds_token_id tok = 0; tok < SMTOKEN_COUNT; ++tok) {
        for (DiskId idx = DEFAULT_DISKIDX; idx < MAX_HOST_DISKS; idx++) {
            if (SM_INVALID_DISK_ID != table[rowOffset][tok][idx]) {
                diskSet.insert(table[rowOffset][tok][idx]);
            }
        }
    }

    return diskSet;
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

/**
 * Token replacement on prisitine (i.e. brand new SM node) SM storage.  This is
 * a simple round-robin token placement.
 *
 * Assumptions:
 * 1) Uniform storage capacity: since round-robin distribution, disks are of same or similiar
 *    capacity.
 */
bool
SmTokenPlacement::compute(const std::set<fds_uint16_t>& hdds,
                          const std::set<fds_uint16_t>& ssds,
                          ObjectLocationTable* olt)
{
    fds_assert(olt);
    if (!olt) {
        LOGCRITICAL << "Invalid OLT. Failing SM initialization";
        return false;
    }

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
    return true;
}


/**
 * This is called if the SM Superblock detects changes in the storage topology:
 *
 * Assumptions:
 * 1) Disk capacity is uniform in the SM storage -- existing is a simple round-robin distribution, which
 *    does not account for per disk capacity.
 * 2) Newly added disk(s) does not have any SM data.
 *
 * TODO(Sean): rebalance levelDB and token files across SM storage.
 *             this requires
 *             1) some code-refactoring
 *             2) minimal rebalance of levelDB and token files
 *             3) data verification
 *             4) no io during re-balance.
 *
 * There are 3 scenarios we cover:
 * 1) only new disks without lost disks/tokens - do nothing.  these disks are not used.  this will be
 *    addressed in the future TODO(Sean).
 * 2) new disks with lost disks/tokens - rebalance using new disks to distribute lost tokens uniformly across
 *    SM storage.  For now  add lost tokens from lost disks to newly added disks, and also balance across existing storage.
 * 3) no new disks with lost disks/tokens - rebalance tokens across existing storage.
 */
bool
SmTokenPlacement::recompute(const std::set<fds_uint16_t>& baseStorage,
                            const std::set<fds_uint16_t>& addedStorage,
                            const std::set<fds_uint16_t>& removedStorage,
                            diskio::DataTier storageTier,
                            ObjectLocationTable* olt,
                            Error& err)
{
    fds_assert(olt);
    if (!olt) {
        LOGCRITICAL << "Invalid OLT. Failing SM initialization";
        err = ERR_SM_SUPERBLOCK_INCONSISTENT;
        return false;
    }
    // there should be at least disk in the base storage.
    fds_verify(baseStorage.size() > 0);
    // should be called only if the topology has changed.
    if (!(addedStorage.size() > 0 || removedStorage.size() > 0)) {
        LOGDEBUG << "Disk topology unchanged. Ignoring recompute request.";
        err = ERR_OK;
        return true;
    }

    LOGNOTIFY << "Recomputing token placment";

    // Contains set for all storage for specified storage tier.
    std::set<fds_uint16_t> totalStorage;

    // totalStorage contains the full topological view of the disks on the system
    // contains ((baseStorage + addedStorage) - removedStorage).
    // Tokens are distributed based on this
    totalStorage.insert(baseStorage.begin(), baseStorage.end());

    // If storage is added, add to the totalStorage.
    if (addedStorage.size() > 0) {
        for (auto diskId: addedStorage) {
            totalStorage.insert(diskId);
        }
    }

    // If storage is removed, remove from totalStorage.
    if (removedStorage.size() > 0) {
        for (auto diskId : removedStorage) {
            totalStorage.erase(diskId);
        }
    }

    LOGNOTIFY << "Tier=" << storageTier << ", "
              << "baseStorage size=" << baseStorage.size() << ", "
              << "addedStorage size=" << addedStorage.size() << ", "
              << "removedStorage size=" << removedStorage.size() << ", "
              << "totalStorage size=" << totalStorage.size()
              << std::endl;

    if (totalStorage.size() == 0) {
        LOGCRITICAL << "No disks left in the node.";
        err = ERR_SM_NO_DISK;
        return false;
    }

    // Calculate average per disk token based.
    uint32_t tokensPerDisk = ceil(SMTOKEN_COUNT / totalStorage.size());
    // Should be at least one token per disk.
    if (tokensPerDisk == 0) {
        tokensPerDisk = 1;
    }

    // get list of tokens from the removed storage.
    SmTokenSet removedTokens;

    for (auto diskId : removedStorage) {
        SmTokenSet tmpRemovedTokens = olt->getSmTokens(diskId);
        if (tmpRemovedTokens.size() > 0) {
            removedTokens.insert(tmpRemovedTokens.begin(), tmpRemovedTokens.end());
        }
    }

    // Here, we want to distribute the tokens to added storage first
    if (addedStorage.size() > 0) {
        std::set<fds_uint16_t>::const_iterator addedStoreCit = addedStorage.cbegin();

        uint32_t tokDistCnt = 0;

        std::set<fds_token_id>::iterator tokenIt;
        while (!removedTokens.empty()) {
            tokenIt = removedTokens.begin();
            olt->setDiskId(*tokenIt, storageTier, *addedStoreCit);
            removedTokens.erase(tokenIt);
            ++addedStoreCit;
            if (addedStorage.cend() == addedStoreCit) {
                addedStoreCit = addedStorage.cbegin();
                if (++tokDistCnt > tokensPerDisk) {
                    break;
                }
            }
        }
    }

    // Now, any leftover tokens should be distributed to all disks.
    std::set<fds_uint16_t>::const_iterator totalStoreCit = totalStorage.cbegin();
    std::set<fds_token_id>::iterator tokenIt;
    while (!removedTokens.empty()) {
        tokenIt = removedTokens.begin();
        olt->setDiskId(*tokenIt, storageTier, *totalStoreCit);
        removedTokens.erase(tokenIt);
        ++totalStoreCit;
        if (totalStorage.cend() == totalStoreCit) {
            totalStoreCit = totalStorage.cbegin();
        }
    }

    return true;
}

}  // namespace fds
