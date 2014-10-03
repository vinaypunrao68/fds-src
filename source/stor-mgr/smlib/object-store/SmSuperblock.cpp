/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <boost/crc.hpp>
#include <fds_process.h>
#include <object-store/SmSuperblock.h>

namespace fds {

/************************************
 * BEGIN Deprecated
 */
SmSuperblockDeprecated::SmSuperblockDeprecated()
        : olt(new ObjectLocationTable()) {
}

SmSuperblockDeprecated::~SmSuperblockDeprecated() {
}

Error
SmSuperblockDeprecated::loadSuperblock(const DiskIdSet& hddIds,
                             const DiskIdSet& ssdIds,
                             const DiskLocMap& diskMap,
                             SmTokenSet& smTokensOwned) {
    Error err(ERR_OK);

    // before we implement reading superblock from disk, validating, etc
    // we are just assuming we always come from clean state, so fill
    // in the datastructs and return
    ownedTokens.swap(smTokensOwned);
    SmTokenPlacement::compute(hddIds, ssdIds, olt);

    return err;
}

fds_uint16_t
SmSuperblockDeprecated::getDiskId(fds_token_id smTokId,
                        diskio::DataTier tier) const {
    return olt->getDiskId(smTokId, tier);
}

SmTokenSet
SmSuperblockDeprecated::getSmOwnedTokens() const {
    // we will return a copy
    SmTokenSet tokens;
    tokens.insert(ownedTokens.begin(), ownedTokens.end());
    return tokens;
}
uint32_t SmSuperblockDeprecated::write(serialize::Serializer*  s) const {
    fds_uint32_t bytes = 0;
    bytes += olt->write(s);
    return bytes;
}

uint32_t SmSuperblockDeprecated::read(serialize::Deserializer* d) {
    fds_uint32_t bytes = 0;
    bytes += olt->read(d);
    return bytes;
}

// So we can print class members for logging
std::ostream& operator<< (std::ostream &out,
                          const SmSuperblockDeprecated& sb) {
    out << "SM Superblock:\n" << *(sb.olt);
    out << "SM tokens owned by this SM: {";
    if (sb.ownedTokens.size() == 0) out << "none";
    for (SmTokenSet::const_iterator cit = sb.ownedTokens.cbegin();
         cit != sb.ownedTokens.cend();
         ++cit) {
        if (cit != sb.ownedTokens.cbegin()) out << ", ";
        out << *cit;
    }
    out << "}\n";
    return out;
}
/************************************
 * END Deprecated
 */

/************************************
 * SuperblockHeader Ifaces
 */

/*
 * TODO(Sean):
 *
 * Need to define SM Superblock Errors in fds_error.h.
 * Being lazy and trying to avoid long compile time for now.
 */

SmSuperblockHeader::SmSuperblockHeader()
{
    /* poison the superblock header
     */
    memset(this, SmSuperblockHdrPoison, sizeof(*this));
}

SmSuperblockHeader::~SmSuperblockHeader()
{
}

void
SmSuperblockHeader::initSuperblockHeader()
{
    /* Fill the header with static information
     */
    this->SmSbHdrMagic = SmSuperblockMagicValue;
    this->SmSbHdrHeaderSize = sizeof(*this);
    this->SmSbHdrMajorVer = SmSuperblockMajorVer;
    this->SmSbHdrMinorVer = SmSuperblockMinorVer;
    this->SmSbHdrDataOffset = sizeof(*this);
    this->SmSbHdrSuperblockSize = sizeof(struct SmSuperblock);
    this->SmSbHdrOffsetBeginning = offsetof(struct SmSuperblockHeader,
                                            SmSbHdrMagic);
    this->SmSbHdrOffsetEnd = offsetof(struct SmSuperblockHeader,
                                      SmSbHdrLastFieldDummy);
}


Error
SmSuperblockHeader::validateSuperblockHeader()
{
    Error err(ERR_OK);

    /* Fill the header with static information
     */
    if ((SmSbHdrMagic != SmSuperblockMagicValue) ||
        (SmSbHdrHeaderSize != sizeof(*this)) ||
        (SmSbHdrMajorVer != SmSuperblockMajorVer) ||
        (SmSbHdrMinorVer != SmSuperblockMinorVer) ||
        (SmSbHdrDataOffset != sizeof(*this)) ||
        (SmSbHdrSuperblockSize != sizeof(struct SmSuperblock)) ||
        (SmSbHdrOffsetBeginning != offsetof(struct SmSuperblockHeader,
                                            SmSbHdrMagic)) ||
        (SmSbHdrOffsetEnd != offsetof(struct SmSuperblockHeader,
                                      SmSbHdrLastFieldDummy))) {
        err = ERR_SM_SUPERBLOCK_DATA_CORRUPT;
    }

    return err;
}

void
SmSuperblockHeader::setSuperblockHeaderChecksum(const fds_checksum32_t chksum)
{
    SmSbHdrChecksum = chksum;
}


fds_checksum32_t
SmSuperblockHeader::getSuperblockHeaderChecksum()
{
    return SmSbHdrChecksum;
}


/************************************
 * Superblock Ifaces
 */

SmSuperblock::SmSuperblock()
{
}


SmSuperblock::~SmSuperblock()
{
}

void
SmSuperblock::initSuperblock()
{
    Header.initSuperblockHeader();
}


Error
SmSuperblock::readSuperblock(const std::string& path)
{
    Error err(ERR_OK);

    /* Check if the file exists before trying to read.
     */
    std::ifstream fileStr(path.c_str());

    /* If file exist, then read the superblock.
     */
    if (fileStr.good()) {
        fileStr.read(reinterpret_cast<char *>(this), sizeof(*this));
    } else {
        /* TODO(Sean):
         * Need to update the error.  Need SM superblock error value.
         */
        err = ERR_SM_SUPERBLOCK_MISSING_FILE;
    }

    return err;
}

Error
SmSuperblock::writeSuperblock(const std::string& path)
{
    Error err(ERR_OK);

    /* open output stream.
     */
    std::ofstream fileStr(path.c_str());
    if (fileStr.good()) {
        /* If the output stream is good, then write the superblock to
         * disk.
         */
        fileStr.write(reinterpret_cast<char *>(this), sizeof(*this));

        /* Must flush the stream immediately after writing to ensure
         * the superblock lands on the disk.
         */
        fileStr.flush();
    } else {
        /* TODO(Sean):
         * Need to update the error.  Need SM superblock error value.
         */
        err = ERR_SM_SUPERBLOCK_WRITE_FAIL;
    }

    return err;
}

void
SmSuperblock::setSuperblockChecksum()
{
    fds_checksum32_t chksum = computeChecksum();

    Header.setSuperblockHeaderChecksum(chksum);
}

/*
 * Checksum calculation for the superblock.
 *
 */
uint32_t
SmSuperblock::computeChecksum()
{
    boost::crc_32_type checksum;

    /* Since the first 4 bytes of the superblock holds the checksum value,
     * start checksum calculation from after the first 4 bytes.  Also,
     * adjust the length.
     */
    unsigned char *bytePtr = reinterpret_cast<unsigned char*>(this);
    bytePtr += sizeof(fds_checksum32_t);
    size_t len = sizeof(*this) - sizeof(fds_checksum32_t);

    /* Quick sanity check that we are pointing to the right address.
     */
    fds_assert(bytePtr == reinterpret_cast<unsigned char*>(&Header.SmSbHdrMagic));

    /* Calculate the checksum.
     */
    checksum.process_bytes(bytePtr, len);

    return checksum.checksum();
}


Error
SmSuperblock::validateSuperblock()
{
    Error err(ERR_OK);

    fds_checksum32_t thisChecksum, computedChecksum;

    // thisChecksum = *reinterpret_cast<fds_checksum32_t *>(this);
    thisChecksum = Header.getSuperblockHeaderChecksum();

    computedChecksum = computeChecksum();

    if (thisChecksum != computedChecksum) {
        /* TODO(Sean):
         * Need to update the error.  Need SM superblock error value.
         */
        return ERR_SM_SUPERBLOCK_CHECKSUM_FAIL;
    }

    err = Header.validateSuperblockHeader();
    if (err != ERR_OK) {
        /* TODO(Sean):
         * Need to update the error.  Need SM superblock error value.
         * This should really be like: ERR_SM_SUPERBLOCK_HEADER_CHECK
         */
        return ERR_SM_SUPERBLOCK_DATA_CORRUPT;
    }

    return err;
}

/************************************
 * SuperblockMgr Ifaces
 */

SmSuperblockMgr::SmSuperblockMgr()
{
}

SmSuperblockMgr::~SmSuperblockMgr()
{
}

Error
SmSuperblockMgr::loadSuperblock(const DiskLocMap& latestDiskMap)
{
    Error err(ERR_OK);
    std::string superblockPath;
    bool genNewSuperblock = false;
    fds_uint16_t masterSuperblockDisk = 0xffff;

    /* There should be at least one disk in the map.
     */
    fds_assert(latestDiskMap.size() > 0);

    /* Cache the diskMap to a local storage.
     */
    diskMap = latestDiskMap;

    SmSuperblockMgrTestSetDumpDiskMap();

    /* Get the first disk from the map.
     */
    auto firstDisk = diskMap.begin();
    superblockPath = firstDisk->second.c_str();
    superblockPath.append(superblockName);

    /* Save off the first disk id, so when we compare with other superblock,
     * we don't need to check against itself.
     */
    masterSuperblockDisk = firstDisk->first;

    /* Try reading the superblock from the first disk.
     */
    err = superblockMaster.readSuperblock(superblockPath);
        /* TODO(Sean)
         * Need to beef up the error check here.  Can be !found.
         */
    if (err != ERR_OK) {
        /* If there is an error, then we can safely assume that superblocks
         * are bad.  Need to create one.
         */
        /* TODO(Sean):
         * How do I populate the OLT at this point?
         */
        genNewSuperblock = true;
    } else {
        /* Successfully read the superblock.  Validate the superblock.
         */
        err = superblockMaster.validateSuperblock();
        if (err != ERR_OK) {
            /* TODO(Sean):
             * How do I populate the OLT at this point?
             * Also, can re-factor this codde.   being lazy now.
             */
            genNewSuperblock = true;
        } else {
            /* TODO(Sean):
             * Probably can re-arrange the if-else clause in this func a bit
             * better.
             */
            /* Everything seems to be ok..  Now load other superblocks and
             * compare it with the "master"
             *
             * TODO(Sean):
             * Do we want to compare it with all the rest or just another (random)
             * one?
             */
            if (diskMap.size() > 1) {
                for (auto cit = diskMap.begin(); cit != diskMap.end(); ++cit) {
                    /* Skip the master superblock.
                     */
                    if (cit->first == masterSuperblockDisk) {
                        continue;
                    }
                    superblockPath = cit->second.c_str();
                    superblockPath.append(superblockName);

                    SmSuperblock tmpSuperblock;

                    err = tmpSuperblock.readSuperblock(superblockPath);
                    if (err != ERR_OK) {
                        genNewSuperblock = true;
                    } else {
                        /* TODO(Sean):
                         * Should overload "==" operator for this.
                         */
                        if (memcmp(&superblockMaster,
                                   &tmpSuperblock,
                                   sizeof(struct SmSuperblock)) != 0) {
                            genNewSuperblock = true;
                        }
                    }
                }
            }
        }
    }

    /* TODO(Sean):
     * Ok... I am not too proud of previous code block.  Too complicated,
     * but I am too tired to think straight.  So for now, leaving
     * it as is.  Will clean up later.
     */

    if (genNewSuperblock == true) {
        /*
         * TODO(Sean):
         * How do we update the smtoken map or other persisten data here?
         */
        superblockMaster.initSuperblock();

        /* After creating a new superblock, sync to disks.
         */
        err = syncSuperblock();
        fds_assert(err == ERR_OK);
    }

    return err;
}

Error
SmSuperblockMgr::syncSuperblock()
{
    Error err(ERR_OK);
    std::string superblockPath;

    /* At this point, in-memory superblock can be sync'ed to the disk.
     * Calculate and stuff the checksum, and write to disks.
     */
    superblockMaster.setSuperblockChecksum();

    /* Sync superblock to all devices in the disk map */
    for (auto cit = diskMap.begin(); cit != diskMap.end(); ++cit) {
        superblockPath = cit->second.c_str();
        superblockPath.append(superblockName);
        err = superblockMaster.writeSuperblock(superblockPath);
        if (err != ERR_OK) {
            return err;
        }
    }

    return err;
}

std::string
SmSuperblockMgr::SmSuperblockMgrTestGetFileName()
{
    return superblockName;
}

void
SmSuperblockMgr::SmSuperblockMgrTestSetDumpDiskMap()
{
    for (auto cit = diskMap.begin(); cit != diskMap.end(); ++cit) {
        std::cout << cit->first << ":" << cit->second << std::endl;
    }
}

}  // namespace fds
