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

typedef uint32_t fds_checksum32_t;

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
    void setSuperblockHeaderChecksum(const fds_checksum32_t chksum);
    fds_checksum32_t getSuperblockHeaderChecksum();

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

    /* Structure is padded at this point.  The size of the data structure
     * should be modulo 512.
     */

  private:
    inline void assertMagic() {
        fds_assert(SmSbHdrMagic == SmSuperblockMagicValue);
    };
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
    void setSuperblockChecksum();

    /* Validity check for superblock.  Consists of checksum check and
     * header consistency check.
     */
    Error validateSuperblock();

    /* Initializes mainly the header of the superblock.
     */
    void initSuperblock();

    /* POD data definitions.  Collection of superblock header and data.
     */
    SmSuperblockHeader Header;
    DiskMapTable DiskMap;
};

/* compile time assert to check that the superblock header is
 */
static_assert((sizeof(struct SmSuperblock) % SM_SUPERBLOCK_SECTOR_SIZE) == 0,
              "size of the  struct SmSuperblock should be multiple of 512");

/*
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
    SmSuperblockMgr();
    ~SmSuperblockMgr();

    typedef std::unique_ptr<SmSuperblockMgr> unique_ptr;

    Error loadSuperblock(const DiskLocMap & diskMap);
    Error syncSuperblock();

    /* Set of interfaces for unit testing */
    void SmSuperblockMgrTestSetDumpDiskMap();
    std::string SmSuperblockMgrTestGetFileName();

  private:
    /* Master superblock.  The master copy will persist
     */
    SmSuperblock superblockMaster;

    /* set of disks.
     */
    DiskLocMap diskMap;

    /* Name of the superblock file.
     */
    const std::string superblockName = "SmSuperblock";
};

/*
 * Keeps track and persists info about SM token to disk mappings
 * and SM token state (whether SM owns it, GC state, Migration state)
 */
class SmSuperblockDeprecated : public serialize::Serializable {
  public:
    SmSuperblockDeprecated();
    ~SmSuperblockDeprecated();

    typedef std::unique_ptr<SmSuperblockDeprecated> unique_ptr;

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
                                     const SmSuperblockDeprecated& sb);

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
