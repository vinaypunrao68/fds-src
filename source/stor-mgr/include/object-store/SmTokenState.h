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
struct __attribute__((__packed__)) SmTokenDesc {
    SmTokenDesc() : writeFileId(SM_INVALID_FILE_ID), tokenFlags(0) {}
    SmTokenDesc(fds_uint16_t writeFid, fds_uint16_t f) :
        writeFileId(writeFid), tokenFlags(f) {}

    inline void setValid() {
        tokenFlags |= SMTOKEN_FLAG_VALID;
    }
    inline void setInvalid() {
        tokenFlags = 0;
        writeFileId = SM_INVALID_FILE_ID;
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

struct __attribute__((__packed__)) TokenDescTable {
    /**
     * Initializes token descriptor table to "invalid"
     * state for all the SM tokens
     */
    TokenDescTable();

    /**
     * Initialize given set of tokens as "valid"
     * If a token already valid, does not do anything
     * @return true if at least one token was initialized to "valid"
     *         false if all given tokes were already valid
     */
    fds_bool_t initializeSmTokens(const SmTokenSet& smToksValid);

    /**
     * Resets given set of tokens into "invalid" state
     * This will cause given set of SM tokens to lose GC info
     * @return set of tokens that were invalidated and that were
     * valid before they were invalidated
     */
    SmTokenSet invalidateSmTokens(const SmTokenSet& smToksInvalid);

    /**
     * Checks if all tokens that are in given set are marked 'valid'
     * and tokens that are not in the given set are marked 'invalid'
     * @param[in] set of tokens owned by this SM
     * @return ERR_OK if smTokensOwned exactly match token state;
     * ERR_SM_SUPERBLOCK_INCONSISTENT if at least one token in smTokensOwned
     * set is marked 'invalid'; ERR_SM_NOERR_LOST_SM_TOKENS if at least
     * one SM token marked 'valid' that is not in smTokensOwned set
     */
    Error checkSmTokens(const SmTokenSet& smTokensOwned);

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
     * Checks whether token is valid on at least one tier
     */
    fds_bool_t isValidOnAnyTier(fds_token_id smToken) const;

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
