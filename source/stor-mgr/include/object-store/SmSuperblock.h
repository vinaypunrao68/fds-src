/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_SMSUPERBLOCK_H_
#define SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_SMSUPERBLOCK_H_

#include <string>
#include <set>

#include <persistent-layer/dm_io.h>
#include <object-store/SmTokenPlacement.h>

namespace fds {

typedef std::set<fds_uint16_t> DiskIdSet;
typedef std::unordered_map<fds_uint16_t, std::string> DiskLocMap;

/*
 * Keeps track and persists info about SM token to disk mappings
 * and SM token state (whether SM owns it, GC state, Migration state)
 */
class SmSuperblock : public serialize::Serializable {
  public:
    SmSuperblock();
    ~SmSuperblock();

    typedef std::unique_ptr<SmSuperblock> unique_ptr;

    /**
     * This is called when SM comes up and receives its first
     * DLT. This method tries to read superblock. If SM comes from
     * clean state, it will populate SM token to disk mappings and
     * state of SM tokens that this SM owns; If there is already a
     * superblock, the method currently validates it and compares
     * that its content matches existing disks and SM tokens
     * @param[in] hddIds set of disk ids of HDD devices
     * @param[in] ssdIds set of disk ids of SSD devices
     * @param[in] diskMap map of disk id to root path
     * @param[in] smTokensOwned set of SM token ids that this SM owns
     * smTokensOwned can be modified by the method.
     * TODO(Anna) we will probably change smTokensOwned parameter to
     * something like a map of sm token to token state
     * @return ERR_OK if superblock of loaded successfully or if we
     * came from clean state and successfully persistent the superblock
     * Otherwise returns an error.
     */
    Error loadSuperblock(const DiskIdSet& hddIds,
                         const DiskIdSet& ssdIds,
                         const DiskLocMap& diskMap,
                         SmTokenSet& smTokensOwned);


    /**
     * Get disk ID where given SM token resides on a given tier
     * @return disk id or fds_diskid_invalid if tier does not exists
     * or superblock was not loaded / initialized properly
     */
    fds_uint16_t getDiskId(fds_token_id smTokId,
                           diskio::DataTier tier) const;

    /**
     * Returns a set of SM tokens that this SM currently owns
     * Will revisit this method when we have more SM token states
     */
    SmTokenSet getSmOwnedTokens() const;

    // Serializable
    uint32_t virtual write(serialize::Serializer*  s) const;
    uint32_t virtual read(serialize::Deserializer* d);

    // So we can print class members for logging
    friend std::ostream& operator<< (std::ostream &out,
                                     const SmSuperblock& sb);

  private:
    /**
     * Object Location Table that maps SM tokens to disk ids
     * The mapping is there regardless whether SM owns the token
     * or not
     */
    ObjectLocationTable::ptr olt;

    /**
     * We should revisit this when we add GC and migration state
     * but for now keeping a list of SM tokens that this SM owns
     */
    SmTokenSet ownedTokens;
};

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_SMSUPERBLOCK_H_
