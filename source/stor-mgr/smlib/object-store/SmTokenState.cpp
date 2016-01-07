/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <set>
#include <object-store/SmTokenState.h>

namespace fds {

const fds_uint16_t fds_hdd_row = 0;
const fds_uint16_t fds_ssd_row = 1;
const fds_uint16_t DEFAULT_DISKIDX = 0;

////// TokenDescTable class implementation ///////
TokenDescTable::TokenDescTable() {
    memset(this, 0, sizeof(*this));
}

fds_bool_t
TokenDescTable::initializeSmTokens(const SmTokenLoc& tokens) {
    fds_bool_t initAtLeastOneToken = false;
    // token id is also a column index into stateTbl
    for (SmTokenSet::const_iterator cit = tokens.cbegin();
         cit != tokens.cend();
         ++cit) {
        if (!stateTbl[fds_hdd_row][cit->id][DEFAULT_DISKIDX].isValid()) {
            SmTokenDesc hd(cit->hdd, SM_INIT_FILE_ID, SMTOKEN_FLAG_VALID);
            stateTbl[fds_hdd_row][cit->id][DEFAULT_DISKIDX] = hd;
            SmTokenDesc sd(cit->ssd, SM_INIT_FILE_ID, SMTOKEN_FLAG_VALID);
            stateTbl[fds_ssd_row][cit->id][DEFAULT_DISKIDX] = sd;
            initAtLeastOneToken = true;
        }
    }
    return initAtLeastOneToken;
}

SmTokenSet
TokenDescTable::invalidateSmTokens(const SmTokenSet& smToksInvalid) {
    SmTokenSet tokens;
    // token id is also a column index into stateTbl
    for (SmTokenSet::const_iterator cit = smToksInvalid.cbegin();
         cit != smToksInvalid.cend();
         ++cit) {
        for (auto idx = DEFAULT_DISKIDX; idx < MAX_HOST_DISKS; ++idx) {
            if (stateTbl[fds_hdd_row][*cit][idx].isValid()) {
                stateTbl[fds_hdd_row][*cit][idx].setInvalid();
                stateTbl[fds_ssd_row][*cit][idx].setInvalid();
                tokens.insert(*cit);
            }
        }
    }
    return tokens;
}


Error
TokenDescTable::checkSmTokens(const SmTokenSet& smTokensOwned) {
    for (SmTokenSet::const_iterator cit = smTokensOwned.cbegin();
         cit != smTokensOwned.cend();
         ++cit) {
        if (!isValidOnAnyTier(*cit)) {
            // a token that we think we own is not valid!
            return ERR_SM_SUPERBLOCK_INCONSISTENT;
        }
    }
    // all tokens in smTokensOwned are marked 'valid'
    for (fds_token_id tokId = 0; tokId < SMTOKEN_COUNT; ++tokId) {
        if (isValidOnAnyTier(tokId) &&
            (smTokensOwned.count(tokId) == 0)) {
            // token is marked valid but we don't own it
            return ERR_SM_NOERR_LOST_SM_TOKENS;
        }
    }
    return ERR_OK;
}

fds_uint32_t
TokenDescTable::row(diskio::DataTier tier) const {
    // here we are explicit with translation of tier to row number
    // if tier enum changes ....
    if (tier == diskio::diskTier) {
        return fds_hdd_row;
    } else if (tier == diskio::flashTier) {
        return fds_ssd_row;
    }
    fds_panic("Unknown tier request from token descriptor table\n");
    return 0;
}

fds_uint16_t
TokenDescTable::getWriteFileId(DiskId diskId,
                               fds_token_id smToken,
                               diskio::DataTier tier) const {
    fds_verify(smToken < SMTOKEN_COUNT);
    auto idx = getIdx(diskId, smToken, tier);
    if (idx >= MAX_HOST_DISKS) {
        return SM_INVALID_FILE_ID;
    } else {
        return stateTbl[row(tier)][smToken][idx].writeFileId;
    }
}

void
TokenDescTable::setWriteFileId(fds_token_id smToken,
                               diskio::DataTier tier,
                               fds_uint16_t fileId) {
    stateTbl[row(tier)][smToken].writeFileId = fileId;
}

void
TokenDescTable::setCompactionState(fds_token_id smToken,
                                   diskio::DataTier tier,
                                   fds_bool_t inProgress) {
    if (inProgress) {
        stateTbl[row(tier)][smToken].setCompacting();
    } else {
        stateTbl[row(tier)][smToken].unsetCompacting();
    }
}

fds_bool_t
TokenDescTable::isCompactionInProgress(fds_token_id smToken,
                                       diskio::DataTier tier) const {
    return stateTbl[row(tier)][smToken].isCompacting();
}

fds_bool_t
TokenDescTable::isValidOnAnyTier(fds_token_id smToken) const {
    return (stateTbl[row(diskio::diskTier)][smToken][DEFAULT_DISKIDX].isValid() ||
            stateTbl[row(diskio::flashTier)][smToken][DEFAULT_DISKIDX].isValid());
}

fds_uint16_t
TokenDescTable::getIdx(DiskId diskId,
                       fds_token_id smToken,
                       diskio::DataTier tier) {
    if (diskID == SM_INVALID_DISK_ID) {
        return MAX_HOST_DISKS;
    }
    for (auto idx = DEFAULT_DISKIDX; idx < MAX_HOST_DISKS; ++idx) {
        if (stateTbl[tier][smToken][idx] == diskId) {
            return idx;
        }
    }
    return MAX_HOST_DISKS;
}

SmTokenSet
TokenDescTable::getSmTokens() const {
    SmTokenSet tokens;
    // we just need to check one tier, but the other tier
    // should have valid flag as well
    for (fds_token_id tokId = 0; tokId < SMTOKEN_COUNT; ++tokId) {
        if (stateTbl[fds_hdd_row][tokId].isValid()) {
            fds_verify(stateTbl[fds_ssd_row][tokId].isValid());
            tokens.insert(tokId);
        }
    }
    return tokens;
}

fds_bool_t
TokenDescTable::operator ==(const TokenDescTable& rhs) const {
    if (this == &rhs) {
        return true;
    }

    return (0 == memcmp(stateTbl,
                        rhs.stateTbl,
                        sizeof(stateTbl)));
}

std::ostream& operator<< (std::ostream &out,
                          const TokenDescTable& tbl) {
    out << "Token Descriptor Table:\n";
    for (fds_uint32_t i = 0; i < SM_TIER_COUNT; ++i) {
        if (i == fds_hdd_row) {
            out << "HDD: ";
        } else {
            out << "SSD: ";
        }
        for (fds_uint32_t j = 0; j < SMTOKEN_COUNT; ++j) {
            out << "[" << tbl.stateTbl[i][j] << "] ";
        }
        out << "\n";
    }
    return out;
}

std::ostream& operator<< (std::ostream &out,
                          const SmTokenDesc& td) {
    out << std::hex << td.writeFileId << std::dec << ",";
    if (td.tokenFlags == 0) {
        out << "{no flags}";
    } else {
        // output all flags that are set
        if (td.isValid()) {
            out << "{valid}";
        }
        if (td.isCompacting()) {
            out << "{compacting}";
        }
    }
    return out;
}

}  // namespace fds
