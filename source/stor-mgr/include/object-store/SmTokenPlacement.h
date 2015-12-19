/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_SMTOKENPLACEMENT_H_
#define SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_SMTOKENPLACEMENT_H_

#include <string>
#include <set>
#include <map>

#include <SmTypes.h>
#include <persistent-layer/dm_io.h>

namespace fds {

/**
 * Plain Old Data definition of the object location table that maps
 * SM token id to disk id to HDD and SSD tier. SM token can reside on
 * HDD, SSD or both tiers
 *
 * When persisting data on stable storage, we want to keep the data in a
 * raw format.
 *
 * Object Location Table layout: each column is SM token ID
 * row 0 : HDD disk ID
 * row 1 : SSD disk ID
 *
 */
#define OBJ_LOCATION_TABLE_SECTOR_SIZE    (512)

struct __attribute__((__packed__)) ObjectLocationTable {
    ObjectLocationTable();

    /**
     * Sets disk id for a given sm token id and tier
     */
    void setDiskId(fds_token_id smToken,
                   diskio::DataTier tier,
                   fds_uint16_t diskId);
    fds_uint16_t getDiskId(fds_token_id smToken,
                           diskio::DataTier tier) const;
    static fds_bool_t isDiskIdValid(fds_uint16_t diskId);

    /* Get disk topology information from a specified tier.
     */
    std::set<uint16_t> getDiskSet(diskio::DataTier tier);

    /**
     * Get a set of SM tokens that reside on a given disk
     * @param[out] tokenSet is populated with SM tokens
     */
    SmTokenSet getSmTokens(fds_uint16_t diskId) const;

    /**
     * Validates Object Location Table for a given set of
     * disk IDs that belong to a given tier
     * @return ERR_OK if OLT contains all disks in diskIdSet
     * and not other disks; ERR_SM_OLT_DISKS_INCONSISTENT if OLT
     * contains at least one disk that is not in diskIdSet
     * AND/OR if OLT does not contain at least one of the disks
     * in diskIdSet
     */
    Error validate(const std::set<fds_uint16_t>& diskIdSet,
                   diskio::DataTier tier) const;

    /// compares to another object location table
    fds_bool_t operator ==(const ObjectLocationTable& rhs) const;

    /// POD data
    fds_uint16_t table[SM_TIER_COUNT][SMTOKEN_COUNT];
} __attribute__((aligned(OBJ_LOCATION_TABLE_SECTOR_SIZE)));

static_assert((sizeof(struct ObjectLocationTable) %  OBJ_LOCATION_TABLE_SECTOR_SIZE) == 0,
              "size of the struct Object Location Table should be multiple of 512 bytes");

std::ostream& operator<< (std::ostream &out,
                          const ObjectLocationTable& tbl);

std::ostream& operator<< (std::ostream &out,
                          const SmTokenSet& toks);

/**
 * Algorithm for distributing SM tokens over disks
 * The class is intended to be stateless. It takes all
 * required input via parameters and returns filled in
 * Object Location Table
 */
class SmTokenPlacement {
  public:
    static void compute(const std::set<fds_uint16_t>& hdds,
                        const std::set<fds_uint16_t>& ssds,
                        ObjectLocationTable* olt);

    static bool recompute(const std::set<fds_uint16_t>& baseStorage,
                          const std::set<fds_uint16_t>& addedStorage,
                          const std::set<fds_uint16_t>& removedStorage,
                          diskio::DataTier storageTier,
                          ObjectLocationTable* olt,
                          Error& error);

};

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_SMTOKENPLACEMENT_H_
