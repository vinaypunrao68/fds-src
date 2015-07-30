/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_SMSUPERBLOCK_H_
#define SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_SMSUPERBLOCK_H_

#include <string>
#include <set>
#include <map>
#include <boost/filesystem.hpp>

#include <sys/mount.h>
#include <functional>
#include <concurrency/RwLock.h>
#include <persistent-layer/dm_io.h>
#include <object-store/SmTokenPlacement.h>
#include <object-store/SmTokenState.h>

namespace fds {

typedef std::set<fds_uint16_t> DiskIdSet;
typedef std::unordered_map<fds_uint16_t, std::string> DiskLocMap;
typedef std::unordered_map<fds_uint16_t, bool> DiskHealthMap;

typedef uint32_t fds_checksum32_t;

typedef std::function<void (const diskio::DataTier&,
                            const std::set<std::pair<fds_token_id, fds_uint16_t>>&
                            )> DiskChangeFnObj;

/*
 * Some constants for SM superblock.
 */

#define SM_SUPERBLOCK_SECTOR_SIZE   (512)
/* Initial memory of the superblock is poisoned with following pattern.
 */
const uint8_t SmSuperblockHdrPoison = 0xff;

/* Static content of the superblock header.
 */
const uint32_t SmSuperblockMagicValue = 0xc000feee;

/* Major and minor versions.  This should be incremented (at least
 * the minor version) whenever the disklayout changes.
 */
const uint16_t SmSuperblockMajorVer = 1;
const uint16_t SmSuperblockMinorVer = 0;



/*
 * Design approach:
 *
 * 3 requirements: 1) must checksum the superblock to detect possible
 *                    data corruption
 *                 2) must have multiple copies of a "master" superblock for
 *                    redundancy and recovery.
 *                 3) persist data across reboot or fault.
 *
 * Note:  We cannot serialize data here.  We've tried, but our implementation
 *        of serialization is insufficient and inefficient.
 *        Moreover, We are storing persisent data that are Plain Old Data (POD).
 *        We cannot save class information, which may internally store runtime
 *        data.  It's better to store raw data, and re-build a class data as we
 *        restore the raw data into a class.
 *
 *        It will look kludge, but oh well..
 *
 * SM Superblock has 3 classes:
 *      - SmSuperblockHeader - metadata for SM superblock that has information
 *        about the super block
 *      - SmSuperblock - the content of the SM superblock.  Consists of
 *        SmSuperblockHeader + persisted data
 *      - SmSuperblockMgr - manages the superblock.
 *
 * Disk Layout:
 *
 *      --------------------------------------
 *      |    Superblock Header               |
 *      --------------------------------------
 *      |    DATA_0                          |
 *      |    DATA_1                          |
 *      |    ...                             |
 *      |    DATA_N                          |
 *      --------------------------------------
 *
 * The DATA_* should be a trivial-class (or Plain Old Data (POD) structures).
 * It *cannot* contain any virtual funcs nor virtual base classes.
 * We are persisting RAW data, and not a class, which may contain run-time data.
 *
 * In the initial version, we embed the POD data in the SMSuperblock class.
 * This means that the POD definition of DATA_* must be exposed to the
 * superblock class.
 * To read/write raw data from the class, do a single I/O to and from the
 * memory that points to the superblock class.
 */

/* Declaring POD (Plain Old Data) superblock header data structure.
 *
 * We are saving persistent data on a disk, and we can't have any garbage
 * from the class definitions, which may have run-time data.
 */
struct SmSuperblockHeader {
  public:
    /* Constructor and destructor
     */
    SmSuperblockHeader();
    ~SmSuperblockHeader();

    void initSuperblockHeader();
    Error validateSuperblockHeader();

    inline void setSuperblockHeaderChecksum(const fds_checksum32_t chksum) {
        SmSbHdrChecksum = chksum;
    };

    inline fds_checksum32_t getSuperblockHeaderChecksum() {
        return SmSbHdrChecksum;
    };

    /* Utility function to validate the SM Superblock header.
     */
    inline void assertMagic() {
        fds_assert(SmSbHdrMagic == SmSuperblockMagicValue);
    };

    /* NOTE:
     *   Exclusively define the size of each variable and make sure there
     *   is no padding in between.
     */

    /* The checksum is a simple crc32 of the header, but doesn't include
     * the checksum field.
     */
    fds_checksum32_t SmSbHdrChecksum;

    /* The magic value for the SM super block.
     */
    uint32_t SmSbHdrMagic;

    /* size of the superblock header struct.
     */
    uint32_t SmSbHdrHeaderSize;

    /* offset where the header really starts..  Right after the
     * SmSbHdrChecksum field.
     */
    uint32_t SmSbHdrOffsetBeginning;
    uint32_t SmSbHdrOffsetEnd;

    /* size of the superblock.  If the superblock size changes during the
     * development time, we can discard the current SM superblock and regen
     * a new one.
     * Superblock size = sizeof(header) + sizeof(data)
     */
    uint32_t SmSbHdrSuperblockSize;

    /* Version of the superblock.  If the in-memory version and on-disk
     * version differ, we have to discard the current superblock and rebuild it.
     */
    uint16_t SmSbHdrMajorVer;
    uint16_t SmSbHdrMinorVer;

    /* Offset (in bytes) of data section of the superblock data.
     */
    uint32_t SmSbHdrDataOffset;

    /* This is a dummy declaration to indicate that it's the last
     * field in the superblock header file.
     */
    uint32_t SmSbHdrLastFieldDummy;

    /* Structure is padded at this point.  The size of the data structure
     * should be modulo 512.
     */
} __attribute__((aligned(SM_SUPERBLOCK_SECTOR_SIZE)));

/* compile time assert to check that the superblock header is 512 bytes aligned.
 */
static_assert((sizeof(struct SmSuperblockHeader) % SM_SUPERBLOCK_SECTOR_SIZE) == 0,
              "Size of the SmSuperblockHeader should be multiple of 512 bytes");
static_assert((sizeof(struct SmSuperblockHeader) == SM_SUPERBLOCK_SECTOR_SIZE),
              "Size of the SmSuperblockHeader should be multiple of 512 bytes");

/* Definition of super block.
 * Superblock consists of
 *      - Header
 *      - Persistent data
 *          - OLT
 *
 *      - Additional data (POD) should be added to the end of the
 */
struct SmSuperblock {
  public:
    SmSuperblock();
    ~SmSuperblock();

    /* Interfaces for loading and storing superblock
     */
    Error readSuperblock(const std::string& path);
    Error writeSuperblock(const std::string& path);

    /* Compute checkusm for the superblock
     */
    uint32_t computeChecksum();

    /* Set superblock checksum, which is the first 4 bytes of the superblock.
     */
    inline void setSuperblockChecksum();

    /* Get superblock checksum value.
     */
    inline fds_checksum32_t getSuperblockChecksum() {
        return Header.getSuperblockHeaderChecksum();
    };

    /* Validity check for superblock.  Consists of checksum check and
     * header consistency check.
     */
    Error validateSuperblock();


    /* Initializes mainly the header of the superblock.
     */
    void initSuperblock();

    /* == operator for superblock.
     */
    fds_bool_t operator ==(const SmSuperblock& rhs) const;

    /* POD data definitions.  Collection of superblock header and data.
     */
    SmSuperblockHeader Header;
    fds_uint64_t DLTVersion;
    ObjectLocationTable olt;
    TokenDescTable tokTbl;

    /* Pad the superblock for future extensibility.
     */
    char SmSuperblockPadding[2048];
};

/* compile time assert to check that the superblock header is properly aligned
 */
static_assert((sizeof(struct SmSuperblock) % SM_SUPERBLOCK_SECTOR_SIZE) == 0,
              "size of the  struct SmSuperblock should be multiple of 512");

/**
 * SM Superblock Manager
 *
 * This class maintains the superblock.  It is responsible for
 * - Build superblock.
 * - Writing superblock to disks in a SM instance.
 * - Reading "master" and "buddy" superblock for content verification.
 * - Removal of superblock on disks in a SM instance.
 */
class SmSuperblockMgr {
  public:
    explicit SmSuperblockMgr(DiskChangeFnObj diskChangeFunc=DiskChangeFnObj());
    ~SmSuperblockMgr();

    typedef std::unique_ptr<SmSuperblockMgr> unique_ptr;

    /**
     * This is called on ObjectStore module bringup when SM
     * registers with OM.
     * This method tries to read superblock. If SM comes from
     * clean state, it will populate SM token to disk mappings and
     * state of SM tokens that this SM owns; If there is already a
     * superblock, the method currently validates it and compares
     * that its content matches existing disks
     * @param[in] hddIds set of disk ids of HDD devices
     * @param[in] ssdIds set of disk ids of SSD devices
     * @param[in] diskMap map of disk id to root path
     * @return ERR_OK if superblock of loaded successfully or if we
     * came from clean state and successfully persistent the superblock
     * Otherwise returns an error.
     */
    Error loadSuperblock(DiskIdSet& hddIds,
                         DiskIdSet& ssdIds,
                         const DiskLocMap & latestDiskMap,
                         const DiskLocMap & latestDiskDevMap = DiskLocMap());
    Error syncSuperblock();
    Error syncSuperblock(const std::set<uint16_t>& badSuperblock);

    void recomputeTokensForLostDisk(const DiskId& diskId,
                                    DiskIdSet& hddIds,
                                    DiskIdSet& ssdIds);

    /**
     * Reconcile superblocks, if there is inconsistency.
     */
    Error reconcileSuperblock();

    /**
     * Called when DLT update is received with current list of SM tokens
     * that this SM owns. This method finds SM tokens that are in the list
     * but was not previously owned, and updates TokenState to valid
     * Also updates DLT version
     * @return ERR_SM_NOERR_GAINED_SM_TOKENS if SM gained ownership of
     * new SM tokens; ERR_SM_NOERR_GAINED_SM_TOKENS if dltVersion matches
     * the version saved in superblock, but tokens that are not in smTokensOwned
     * are marked valid in superblock; ERR_OK if success
     */
    Error updateNewSmTokenOwnership(const SmTokenSet& smTokensOwned,
                                    fds_uint64_t dltVersion);

    /**
     * Called on DLT close received from OM with the list of SM tokens
     * that this SM does not own.
     * @return set of tokens for which SM lost ownership
     */
    SmTokenSet handleRemovedSmTokens(SmTokenSet& smTokensNotOwned,
                                     fds_uint64_t dltVersion);

    /**
     * Get disk ID where given SM token resides on a given tier
     * @return disk id or invalid disk ID if tier does not exist
     * or superblock was not loaded / initialized properly
     */
    fds_uint16_t getDiskId(fds_token_id smTokId,
                           diskio::DataTier tier);

    /**
     * Returns a set of SM tokens that this SM currently owns
     * Will revisit this method when we have more SM token states
     */
    SmTokenSet getSmOwnedTokens();
    SmTokenSet getSmOwnedTokens(fds_uint16_t diskId);
    fds_uint16_t getWriteFileId(fds_token_id smToken,
                                diskio::DataTier tier);
    fds_bool_t compactionInProgress(fds_token_id smToken,
                                    diskio::DataTier tier);
    Error changeCompactionState(fds_token_id smToken,
                                diskio::DataTier tier,
                                fds_bool_t inProg,
                                fds_uint16_t newFileId);

    /* Set of interfaces for unit testing */
    std::string SmSuperblockMgrTestGetFileName();

    /**
     * Get the latest committed DLT version.
     */
    fds_uint64_t getDLTVersion();

    // So we can print class members for logging
    friend std::ostream& operator<< (std::ostream &out,
                                     const SmSuperblockMgr& sbMgr);

    friend class SmSuperblockTestDriver;

  private:  // methods
    std::string
    getSuperblockPath(const std::string& dir_path);

    bool
    checkPristineState(DiskIdSet& newHDDs, DiskIdSet& newSSDs);

    Error changeTokenCompactionState(fds_token_id smToken,
                                     diskio::DataTier tier,
                                     fds_bool_t inProg,
                                     fds_uint16_t newFileId);

    size_t
    countUniqChecksum(const std::multimap<fds_checksum32_t, uint16_t>& checksumMap);

    void
    checkDiskTopology(DiskIdSet& newHDDs, DiskIdSet& newSSDs);

    void
    checkDisksAlive(DiskIdSet& newHDDs, DiskIdSet& newSSDs);

    DiskIdSet
    diffDiskSet(const DiskIdSet& diskSet1, const DiskIdSet& diskSet2);

    SmTokenSet getTokensOfThisSM(fds_uint16_t diskId);

    /**
     * Set the latest committed DLT version.
     */
    Error setDLTVersion(fds_uint64_t dltVersion, bool syncImmediately);
    /// this one assimes that lock is heald on superblock
    Error setDLTVersionLockHeld(fds_uint64_t dltVersion, bool syncImmediately);

    std::string getTempMount();

    void deleteMount(std::string& path);

    void checkForHandledErrors(Error& err);

    void setDiskHealthMap();

    void markDiskBad(const fds_uint16_t& diskId);

    bool isDiskHealthy(const fds_uint16_t& diskId);

  private:
    /// Master superblock. The master copy will persist
    SmSuperblock superblockMaster;
    /// lock for superblockMaster, for now used
    /// when updating compaction state, other ops are
    /// serialized by the caller. We will most likely refactor this
    /// pretty soon
    fds_rwlock sbLock;

    /// this is used for extra checks only
    fds_bool_t noDltReceived;

    /// set of disks.
    DiskLocMap diskMap;
    DiskLocMap diskDevMap;
    DiskHealthMap diskHealthMap;
    
    /// Name of the superblock file.
    const std::string superblockName = "SmSuperblock";

    fds::DiskChangeFnObj diskChangeFn;
};

std::ostream& operator<< (std::ostream &out,
                          const DiskLocMap& diskMap);

boost::log::formatting_ostream& operator<< (boost::log::formatting_ostream& out,
                                            const DiskIdSet& diskIds);

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_SMSUPERBLOCK_H_
