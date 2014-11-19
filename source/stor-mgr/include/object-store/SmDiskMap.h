/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_SMDISKMAP_H_
#define SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_SMDISKMAP_H_

#include <string>
#include <set>
#include <utility>

#include <fds_module.h>
#include <dlt.h>
#include <persistent-layer/dm_io.h>
#include <object-store/SmSuperblock.h>

namespace fds {

/*
 * SmDiskMap keeps track of data and metadata SM token layout
 * on disks, and manages SM tokens persistent state.
 */
class SmDiskMap : public Module, public boost::noncopyable {
  public:
    explicit SmDiskMap(const std::string& modName);
    ~SmDiskMap();

    typedef std::unique_ptr<SmDiskMap> unique_ptr;
    typedef std::shared_ptr<SmDiskMap> ptr;
    typedef std::shared_ptr<const SmDiskMap> const_ptr;

    /**
     * Updates SM token on-disk location table.
     * Currently assumes that new DLT does not change ownership
     * of SM tokens by this SM, will assert otherwise. This will
     * change when we port back SM token migration
     */
    Error handleNewDlt(const DLT* dlt);

    /**
     * Translation from token or object ID to SM token ID
     */
    static fds_token_id smTokenId(fds_token_id tokId);
    static fds_token_id smTokenId(const ObjectID& objId,
                                  fds_uint32_t bitsPerToken);
    fds_token_id smTokenId(const ObjectID& objId) const;

    /**
     * Return a set of SM tokens that this SM currently owns
     */
    SmTokenSet getSmTokens() const;
    SmTokenSet getSmTokens(fds_uint16_t diskId) const;

    /**
     * Get disk ID where ObjectID (or SM token) data and metadata
     * resides on a given tier.
     */
    fds_uint16_t getDiskId(const ObjectID& objId,
                           diskio::DataTier tier) const;
    fds_uint16_t getDiskId(fds_token_id smTokId,
                           diskio::DataTier tier) const;

    /**
     * Get the root path to disk for a given SM token and tier
     */
    std::string getDiskPath(fds_token_id smTokId,
                            diskio::DataTier tier) const;
    std::string getDiskPath(fds_uint16_t diskId) const;

    /**
     * Returns IDs of all existing disks of a given tier
     */
    DiskIdSet getDiskIds(diskio::DataTier tier) const;

    /**
    * Determines if a write to SSD will cause SSD usage to go beyond capacity threshold.
    * When called it will add writeSize to the map, and if the new value exceeds
    * the threshold will return false.
    *
    * @param oid objectID of the object being written
    * @param writeSize size in bytes of the write
    * @param thresholdPct percentage of total capacity that is considered full
    *
    * @returns true if write will not exceed full threshold, false otherwise
    */
    fds_bool_t ssdTrackCapacityAdd(ObjectID oid, fds_uint64_t writeSize,
            fds_uint32_t fullThreshold);

    /**
    * Subtracts writeSize from consumed capacity. Use this when an SSD write
    * failed to 'fix' the (now) incorrect accounting.
    *
    * @param writeSize size in bytes of the write
    *
    */
    void ssdTrackCapacityDelete(ObjectID oid, fds_uint64_t writeSize);

    /**
     * Module methods
     */
    virtual int mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

  private:  // methods
    /**
     * Reads existing HDDs and SSDs from /disk-map file
     * created by platform. Later we may do that through
     * shared memory
     */
    void getDiskMap();

  private:
    fds_uint32_t bitsPerToken_;

    /// disk ID to path map
    DiskLocMap disk_map;
    /// set of disk IDs of existing SDD devices
    DiskIdSet  ssd_ids;
    /// set of disk IDs of existing HDD devices
    DiskIdSet hdd_ids;

    /// Superblock caches and persists SM token info
    SmSuperblockMgr::unique_ptr superblock;

    /// if true, test mode where we assume no contact with
    /// platform, and use SM service uuid = 1
    fds_bool_t test_mode;

    /// SSD capacity tracking for tiering
    // Maps from DiskId -> <usedCapacity, totalCapcity>
    std::unordered_map<fds_uint16_t,
            std::pair<fds_uint64_t, fds_uint64_t>> * capacityMap;

    friend class ObjectPersistData;
};

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_SMDISKMAP_H_
