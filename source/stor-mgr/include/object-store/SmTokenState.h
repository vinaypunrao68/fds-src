/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_SMTOKENSTATE_H_
#define SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_SMTOKENSTATE_H_

#include <SmTypes.h>
#include <persistent-layer/dm_io.h>

namespace fds {

/// flags for SM token state
/// SM token is owned by this SM
#define SMTOKEN_FLAG_VALID       0x0001
/// SM token is in the middle of GC
#define SMTOKEN_FLAG_COMPACTING  0x0002

/**
 * <tier, SM token> descriptor includes write file ID (file ID where
 * writes go for that tier, SM token) and state.
 * There are states that correspond to SM token independently to
 * tier it resides on; in that case, the state is repeated on both tiers
 */
struct SmTokenDesc {
    SmTokenDesc() : writeFileId(SM_INVALID_FILE_ID), tokenFlags(0) {}
    SmTokenDesc(fds_uint16_t writeFid, fds_uint16_t f) :
        writeFileId(writeFid), tokenFlags(f) {}
    explicit SmTokenDesc(const SmTokenDesc& td) :
        writeFileId(td.writeFileId),
        tokenFlags(td.tokenFlags) {}

    SmTokenDesc & operator =(const SmTokenDesc & rhs);
    inline void setValid() {
        tokenFlags |= SMTOKEN_FLAG_VALID;
    }
    inline void setInvalid() {
        tokenFlags &= ~SMTOKEN_FLAG_VALID;
    }
    inline fds_bool_t isValid() const {
        return (tokenFlags & SMTOKEN_FLAG_VALID);
    }
    inline void setCompacting() {
        tokenFlags |= SMTOKEN_FLAG_COMPACTING;
    }
    inline void unsetCompacting() {
        tokenFlags &= ~SMTOKEN_FLAG_COMPACTING;
    }
    inline fds_bool_t isCompacting() const {
        return (tokenFlags & SMTOKEN_FLAG_COMPACTING);
    }

    fds_uint16_t writeFileId;
    fds_uint16_t tokenFlags;
};

/**
 * Plain Old Data definition of the token descriptor table that maps
 * <tier, SM token id> tuple to token descriptor.
 *
 * When persisting data on stable storage, we want to keep the data in a
 * raw format.
 *
 * Object Location Table layout: each column is SM token ID
 * row 0 : HDD writeFileId, token flags
 * row 1 : SSD writeFileId, token flags
 *
 */
#define TOKEN_DESC_TABLE_SECTOR_SIZE    (512)

struct TokenDescTable {
    TokenDescTable();
    ~TokenDescTable();

    /**
     * Called when SM comes up for the first time to
     * init token states
     * Currently, inits all SM tokens as valid (owned by this SM)
     */
    void initialize(const SmTokenSet& smToksValid);

    /**
     * Sets write file id for a given sm token id and tier
     */
    void setWriteFileId(fds_token_id smToken,
                        diskio::DataTier tier,
                        fds_uint16_t fileId);
    fds_uint16_t getWriteFileId(fds_token_id smToken,
                                diskio::DataTier tier) const;

    /**
     * Set/get token compaction status
     */
    void setCompactionState(fds_token_id smToken,
                            diskio::DataTier tier,
                            fds_bool_t inProgress);
    fds_bool_t isCompactionInProgress(fds_token_id smToken,
                                      diskio::DataTier tier) const;

    /**
     * Get a set of SM tokens that reside on this SM
     * @param[out] tokenSet is populated with SM tokens
     */
    SmTokenSet getSmTokens() const;

    /// compares to another object location table
    fds_bool_t operator ==(const TokenDescTable& rhs) const;

    /// POD data
    SmTokenDesc stateTbl[SM_TIER_COUNT][SMTOKEN_COUNT];

  private:
    fds_uint32_t row(diskio::DataTier tier) const;
} __attribute__((aligned(TOKEN_DESC_TABLE_SECTOR_SIZE)));

static_assert((sizeof(struct TokenDescTable) %  TOKEN_DESC_TABLE_SECTOR_SIZE) == 0,
              "size of the struct Token Descriptor Table should be multiple of 512 bytes");

std::ostream& operator<< (std::ostream &out,
                          const TokenDescTable& tbl);

std::ostream& operator<< (std::ostream &out,
                          const SmTokenDesc& td);

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_SMTOKENSTATE_H_
