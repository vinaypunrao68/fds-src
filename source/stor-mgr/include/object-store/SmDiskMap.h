/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_SMDISKMAP_H_
#define SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_SMDISKMAP_H_

#include <string>
#include <set>
#include <utility>
#include <atomic>

#include <fds_module.h>
#include <dlt.h>
#include <persistent-layer/dm_io.h>
#include <object-store/SmSuperblock.h>
#include <include/util/disk_utils.h>

namespace fds {

/*
 * SmDiskMap keeps track of data and metadata SM token layout
 * on disks, and manages SM tokens persistent state.
 */
class SmDiskMap : public Module, public boost::noncopyable {
  public:
    const std::string DISK_MAP_FILE = "/disk-map";

    explicit SmDiskMap(const std::string& modName,
                       DiskChangeFnObj diskChangeFn=DiskChangeFnObj());
    ~SmDiskMap();

    typedef std::unique_ptr<SmDiskMap> unique_ptr;
    typedef std::shared_ptr<SmDiskMap> ptr;
    typedef std::shared_ptr<const SmDiskMap> const_ptr;


    /**
     * Loads and validates superblock / disk map
     * @return ERR_SM_NOERR_PRISTINE_STATE is SM comes up from
     * pristine state; otherwise ERR_OK on success
     */
    Error loadPersistentState();

    /**
     * Updates SM token on-disk location table.
     */
    Error handleNewDlt(const DLT* dlt);
    Error handleNewDlt(const DLT* dlt, NodeUuid& mySvcUuid);

    /**
     * Updates SM token on-disk location table -- invalidates
     * SM tokens for which this SM lost ownership
     * @return set of SM tokens for which SM lost ownership
     */
    SmTokenSet handleDltClose(const DLT* dlt);
    SmTokenSet handleDltClose(const DLT* dlt, NodeUuid& mySvcUuid);

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
     * Returns all disk IDs, regardless of tier
     */
    DiskIdSet getDiskIds() const;

    /**
     * Returns total number of disks: hdds + ssds
     */
    fds_uint32_t getTotalDisks() const;
    fds_uint32_t getTotalDisks(diskio::DataTier tier) const;


    /**
     * Checks and returns the type of storage this SM has.
     * All SSD or hybrid.
     */
    bool isAllDisksSSD() const;

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
     * Gets the total consumed space and returns a pair (totalConsumed, totalAvailable).
     * The returned values can be divided out to get the % full
     */
    DiskCapacityUtils::capacity_tuple getDiskConsumedSize(fds_uint16_t diskId);

    /**
     * Get current (i.e. closed DLT) from persitent storage.
     */
    fds_uint64_t getDLTVersion();

    /**
     * Called when encounted IO error when writing to token file or metadata DB
     * When SmDiskMap sees too many IO errors from the same disk, it declares disk
     * failed and migrates SM tokens from that disk to other disks
     */
    void notifyIOError(fds_token_id smTokId,
                       diskio::DataTier tier,
                       const Error& error);

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

    /**
     * To handle disk errors
     */
    void initDiskErrorHandlers();
  private:
    fds_uint32_t bitsPerToken_;

    /// disk ID to path map
    DiskLocMap disk_map;

    /// disk ID to disk dev path map
    DiskLocMap diskDevMap;

    /// set of disk IDs of existing SDD devices
    DiskIdSet  ssd_ids;
    /// set of disk IDs of existing HDD devices
    DiskIdSet hdd_ids;

    /// Superblock caches and persists SM token info
    SmSuperblockMgr::unique_ptr superblock;

    /// if true, test mode where we assume no contact with
    /// platform, and use SM service uuid = 1
    fds_bool_t test_mode;

    /// Map from disk idx to SSD capacity array idx
    std::unordered_map<fds_uint16_t, fds_uint8_t> * ssdIdxMap;
    // For now we'll assume no more than 60 SSDs.
    std::atomic<fds_uint64_t> maxSSDCapacity[60];
    std::atomic<fds_uint64_t> consumedSSDCapacity[60];


    friend class ObjectPersistData;
};

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_SMDISKMAP_H_
