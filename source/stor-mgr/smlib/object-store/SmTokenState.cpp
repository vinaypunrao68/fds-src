/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <set>
#include <object-store/SmTokenState.h>

namespace fds {

const fds_uint16_t fds_hdd_row = 0;
const fds_uint16_t fds_ssd_row = 1;

///// SmTokenDesc class implementation //////////
SmTokenDesc &
SmTokenDesc::operator =(const SmTokenDesc & rhs) {
    if (&rhs != this) {
        writeFileId = rhs.writeFileId;
        tokenFlags = rhs.tokenFlags;
    }
    return *this;
}

////// TokenDescTable class implementation ///////
TokenDescTable::TokenDescTable() {
    memset(this, 0, sizeof(*this));
}

TokenDescTable::~TokenDescTable() {
}

void
TokenDescTable::initialize(const SmTokenSet& smToksValid) {
    // token id is also a column index into stateTbl
    for (SmTokenSet::const_iterator cit = smToksValid.cbegin();
         cit != smToksValid.cend();
         ++cit) {
        SmTokenDesc td(SM_INIT_FILE_ID, SMTOKEN_FLAG_VALID);
        stateTbl[fds_hdd_row][*cit] = td;
        stateTbl[fds_ssd_row][*cit] = td;
    }
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
TokenDescTable::getWriteFileId(fds_token_id smToken,
                              diskio::DataTier tier) const {
    fds_verify(smToken < SMTOKEN_COUNT);
    return stateTbl[row(tier)][smToken].writeFileId;
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
