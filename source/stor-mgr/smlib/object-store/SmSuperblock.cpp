/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <set>
#include <map>
#include <utility>
#include <boost/crc.hpp>
#include <util/Log.h>
#include <fds_process.h>
#include <object-store/SmSuperblock.h>

namespace fds {

/************************************
 * SuperblockHeader Ifaces
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

    /* The header size incliude the padded area.
     */
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
        /* Check the file size
         */
        fileStr.seekg(0, fileStr.end);
        size_t fileStrSize = fileStr.tellg();
        fileStr.seekg(0, fileStr.beg);

        if (fileStrSize != sizeof(struct SmSuperblock)) {
            LOGERROR << "SM Superblock: file size doesn't match in-memory struct"
                     << fileStrSize
                     << "vs. "
                     << sizeof(struct SmSuperblock);
            err = ERR_SM_SUPERBLOCK_DATA_CORRUPT;
        }

        /* Read the superblock to buffer.
         */
        fileStr.read(reinterpret_cast<char *>(this), sizeof(struct SmSuperblock));

        /* Check the bytes read from the superblock.
         */
        if (fileStr.gcount() != sizeof(struct SmSuperblock)) {
            LOGERROR << "SM superblock: read bytes ("
                     << fileStr.gcount()
                     << "), but expected ("
                     << sizeof(struct SmSuperblock)
                     << ")";
            err = ERR_SM_SUPERBLOCK_DATA_CORRUPT;
        }

        /* Assert the magic value to see.
         */
        Header.assertMagic();
    } else if (fileStr.fail()) {
        LOGERROR << "SM Superblock: file not found on " << path;
        err = ERR_SM_SUPERBLOCK_MISSING_FILE;
    } else {
        LOGERROR << "SM Superblock: read failed on " << path;
        err = ERR_SM_SUPERBLOCK_READ_FAIL;
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
        Header.assertMagic();
        fileStr.write(reinterpret_cast<char *>(this), sizeof(struct SmSuperblock));

        /* There shouldn't be any truncation.  Typically no way to recover
         * in this case, since it's likely ENOSPACE.
         */
        fds_verify(fileStr.tellp() == sizeof(struct SmSuperblock));

        /* Must flush the stream immediately after writing to ensure
         * the superblock lands on the disk.
         */
        fileStr.flush();
    } else {
        LOGERROR << "Cannot open SM superblock for write on " << path;
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
#

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
        LOGERROR << "SM superblock: checksum validation failed";
        return ERR_SM_SUPERBLOCK_CHECKSUM_FAIL;
    }

    err = Header.validateSuperblockHeader();
    if (err != ERR_OK) {
        LOGERROR << "SM superblock: header validation failed";
        return ERR_SM_SUPERBLOCK_DATA_CORRUPT;
    }

    return err;
}


fds_bool_t
SmSuperblock::operator ==(const SmSuperblock& rhs) const {
    if (this == &rhs) {
        return true;
    }

    return (0 == memcmp(this, &rhs, sizeof(struct SmSuperblock)));
}

/************************************
 * SuperblockMgr Ifaces
 */

SmSuperblockMgr::SmSuperblockMgr()
{
}

SmSuperblockMgr::~SmSuperblockMgr()
{
    /*
     * TODO(sean):
     * When we have a mechanism to do a clean shutdown, do we need to
     * flush out superblock block destroying the master copy?
     * Or do we rely that whenever the superblock is modified, it will
     * be sync'ed out to disk.  TBD
     */
}

std::string
SmSuperblockMgr::getSuperblockPath(const std::string& dir_path) {
    return (dir_path + "/" + superblockName);
}

Error
SmSuperblockMgr::loadSuperblock(const DiskIdSet& hddIds,
                                const DiskIdSet& ssdIds,
                                const DiskLocMap& latestDiskMap,
                                SmTokenSet& smTokensOwned)
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
    LOGDEBUG << "Got disk map";

    /* Do initial state check.
     * If there is no superblock anywhere on the system, then this is the
     * first time SM started on this node.
     */
    bool pristineState = checkPristineState();
    if (pristineState) {
        /* If pristine state, where there is no SM superblock on the
         * node, then we are going to generate a new set of superblock
         * on each disk on the node.
         */
        superblockMaster.initSuperblock();
        SmTokenPlacement::compute(hddIds, ssdIds, &(superblockMaster.olt));

        /* After creating a new superblock, sync to disks.
         */
        err = syncSuperblock();
        fds_assert(err == ERR_OK);
    } else {
        /* Reconcile superblock().  Elect one "master" superblock, if
         * possible.  Failure to reconcile should be a *very* rare case.
         */
        err = reconcileSuperblock();
        if (err != ERR_OK) {
            fds_panic("Cannot reconcile SM superblocks");
        }

        /* Since, we have a "master" superblock, check if the disk topology
         * has changed from the last persisten state.  For example:
         * 1) disk(s) were added.
         * 2) disk(s) were removed.
         * 3) 1 and 2.
         * For now, if the topology has changed, just panic.  We don't have
         * a mechanism to handle this case, yet.
         */
        checkDiskTopology(hddIds, ssdIds);
    }


    // for now we are not keeping any persistent state about SM tokens
    // that SM owns. Since our current setup we support is 4-node domain
    // with 4-way replication, SM always owns all tokens, so here we just
    // save them and not do anything else (other modules use it for now
    // to only open token files, so not breaking anything if SM thinks it
    // owns more tokens than it does, in case of > 4 node domain); will
    // have to revisit very soon when we implement adding new nodes for
    // > 4 node domains, doing token migration and so on.
    ownedTokens.swap(smTokensOwned);

    return err;
}

/*
 * A function that returns the difference between the diskSet 1 and diskSet 2.
 * The difference is defined as element(s) in diskSet1, but not in diskSet2.
 */
DiskIdSet
SmSuperblockMgr::diffDiskSet(const DiskIdSet& diskSet1,
                             const DiskIdSet& diskSet2)
{
    DiskIdSet deltaDiskSet;

    std::set_difference(diskSet1.begin(), diskSet1.end(),
                        diskSet2.begin(), diskSet2.end(),
                        std::inserter(deltaDiskSet, deltaDiskSet.begin()));

    return deltaDiskSet;
}

/*
 * Determine if the node's disk topology has changed or not.
 *
 * TODO(sean)
 * This function will panic if the disk topology has changed on
 * this node.  In the future, we have to take an appropriate
 * action, which is yet defined.
 */
void
SmSuperblockMgr::checkDiskTopology(const DiskIdSet& newHDDs,
                                   const DiskIdSet& newSSDs)
{
    DiskIdSet persistentHDDs, persistentSSDs;
    DiskIdSet addedHDDs, removedHDDs;
    DiskIdSet addedSSDs, removedSSDs;

    /* Get the list of unique disk IDs from the OLT table.  This is
     * used to compare with the new set of HDDs and SSDs to determine
     * if any disk was added or removed for each tier.
     */
    persistentHDDs = superblockMaster.olt.getDiskSet(diskio::diskTier);
    persistentSSDs = superblockMaster.olt.getDiskSet(diskio::flashTier);

    /* Determine if any HDD was added or removed.
     */
    removedHDDs = diffDiskSet(persistentHDDs, newHDDs);
    addedHDDs = diffDiskSet(newHDDs, persistentHDDs);

    /* Determine if any HDD was added or removed.
     */
    removedSSDs = diffDiskSet(persistentSSDs, newSSDs);
    addedSSDs = diffDiskSet(newSSDs, persistentSSDs);

    /* For now, if the disk topology has changed, then just panic.
     *
     * TODO(sean)
     * In the future, we will probably have to do some handshaking
     * with platform manager and migrate tokens according to the new
     * disk topology???  TBD.
     */
    if ((removedHDDs.size() > 0) ||
        (addedHDDs.size() > 0) ||
        (removedSSDs.size() > 0) ||
        (addedSSDs.size() > 0)) {
        fds_panic("Disk Topology Changed: removed HDDs=%lu, added HDDs=%lu, "
                  "removed SSDs=%lu, added SSDs=%lu",
                  removedHDDs.size(),
                  addedHDDs.size(),
                  removedSSDs.size(),
                  addedSSDs.size());
    }
}

Error
SmSuperblockMgr::syncSuperblock()
{
    Error err(ERR_OK);
    std::string superblockPath;

    fds_assert(diskMap.size() > 0);

    /* At this point, in-memory superblock can be sync'ed to the disk.
     * Calculate and stuff the checksum, and write to disks.
     */
    superblockMaster.setSuperblockChecksum();

    /* Sync superblock to all devices in the disk map */
    for (auto cit = diskMap.begin(); cit != diskMap.end(); ++cit) {
        superblockPath = getSuperblockPath(cit->second.c_str());
        err = superblockMaster.writeSuperblock(superblockPath);
        if (err != ERR_OK) {
            return err;
        }
    }

    return err;
}

Error
SmSuperblockMgr::syncSuperblock(const std::set<uint16_t>& setSuperblock)
{
    Error err(ERR_OK);
    std::string superblockPath;

    fds_assert(diskMap.size() > 0);

    for (auto cit = setSuperblock.begin(); cit != setSuperblock.end(); ++cit) {
        superblockPath = getSuperblockPath(diskMap[*cit]);
        err = superblockMaster.writeSuperblock(superblockPath);
        if (err != ERR_OK) {
            return err;
        }
    }

    return err;
}

size_t
SmSuperblockMgr::countUniqChecksum(const std::multimap<fds_checksum32_t, uint16_t>& checksumMap)
{
    size_t uniqCount = 0;

    /* TODO(sean)
     * Is there more efficient way to count unique keys in multimap?
     */
    for (auto it = checksumMap.begin(), end = checksumMap.end();
         it != end;
         it = checksumMap.upper_bound(it->first)) {
        ++uniqCount;
    }

    return uniqCount;
}

bool
SmSuperblockMgr::checkPristineState()
{
    uint32_t noSuperblockCnt = 0;

    fds_assert(diskMap.size() > 0);

    for (auto cit = diskMap.begin(); cit != diskMap.end(); ++cit) {
        std::string superblockPath = getSuperblockPath(cit->second.c_str());

        /* Check if the file exists.
         */
        std::ifstream diskStr(superblockPath.c_str());
        if (diskStr.good()) {
            diskStr.close();
        } else {
            /* The superblock file doesn't exist in this directory.
             */
            ++noSuperblockCnt;
            diskStr.close();
        }
    }

    return (noSuperblockCnt == diskMap.size());
}

/*
 * This function tries to reconcile Superblocks.
 */
Error
SmSuperblockMgr::reconcileSuperblock()
{
    Error err(ERR_OK);
    std::unordered_map<uint16_t, fds_checksum32_t> diskGoodSuperblock;
    std::multimap<fds_checksum32_t, uint16_t> checksumToGoodDiskIdMap;
    std::set<uint16_t> diskBadSuperblock;

    fds_assert(diskMap.size() > 0);

    /* TODO(sean)
     * Make this loop a function.
     */
    for (auto cit = diskMap.begin(); cit != diskMap.end(); ++cit) {
        std::string superblockPath = getSuperblockPath(cit->second);
        SmSuperblock tmpSuperblock;

        /* Check if the file can be read.
         */
        err = tmpSuperblock.readSuperblock(superblockPath);
        if (err != ERR_OK) {
            LOGWARN << "SM superblock: read disk("
                    << cit->first
                    << ") => "
                    << superblockPath
                    << " failed with error"
                    << err;
            auto ret = diskBadSuperblock.emplace(cit->first);
            if (!ret.second) {
                /* diskID is not unique.  Failed to add to diskBadSuperblock
                 * set, because diskID already exists.  DiskID should be unique.
                 */
                fds_panic("Detected non-unique diskID entry");
            }
            continue;
        }

        /* Check if the SM superblock is a valid one.
         */
        err = tmpSuperblock.validateSuperblock();
        if (err != ERR_OK) {
            LOGWARN << "SM superblock: read disk("
                    << cit->first
                    << ") => "
                    << superblockPath
                    << " failed with error"
                    << err;
            auto ret = diskBadSuperblock.emplace(cit->first);
                /* diskID is not unique.  Failed to add to diskBadSuperblock
                 * set, because diskID already exists.  DiskID should be unique.
                 */
            if (!ret.second) {
                fds_panic("Detected non-unique diskID entry");
            }
            continue;
        }

        /* Everything looks good, so far.
         * First, add to the checksum=>{disks} mapping.  If there are more
         * than one checksum key, then we are in trouble.  Then, we need to
         * take additional steps to reconcile.
         * If the number of of "good superblocks" and number of disks matching
         * an unique checksum, that is the simplest case.
         */
        std::pair<fds_checksum32_t, uint16_t>entry(tmpSuperblock.getSuperblockChecksum(),
                                                   cit->first);
        checksumToGoodDiskIdMap.insert(entry);

        auto ret = diskGoodSuperblock.emplace(cit->first,
                                              tmpSuperblock.getSuperblockChecksum());
        if (!ret.second) {
                /* diskID is not unique.  Failed to add to diskGoodSuperblock
                 * set, because diskID already exists.  DiskID should be unique.
                 */
            fds_panic("Detected non-unique diskID entry");
        }
    }

    /* The number of good superblocks must be a simple majority (51%).  If the
     * number of bad superblocks is at least 50%, then we have two possibilities:
     * 1) panic -- too many bad superblocks.   Not sure about the state of the
     *             node.  Probably safe to panic then continue.
     * 2) re-compute and regen superblock -- throw away everything and create
     *             a new set of superblocks.
     *
     * For now, just panic.
     */
    if (diskGoodSuperblock.size() <= diskBadSuperblock.size()) {
        LOGERROR << "SM superblock: number of good superblocks: "
                 << diskGoodSuperblock.size()
                 << ", bad superblocks: "
                 << diskBadSuperblock.size();
        return ERR_SM_SUPERBLOCK_NO_RECONCILE;
    }

    /* Now we have enough "good" SM superblocks to reconcile, if possible.
     */
    size_t uniqChecksum = countUniqChecksum(checksumToGoodDiskIdMap);
    fds_verify(uniqChecksum > 0);

    /* Simple majority has quorum at this point.  Use one of the good superblock
     * as th "master."  And, if necessary, overwrite bad superblocks with
     * the "master."
     */
    if (1 == uniqChecksum) {
        fds_verify(diskGoodSuperblock.size() > 0);
        auto goodSuperblock = diskGoodSuperblock.begin();
        std::string goodSuperblockPath = getSuperblockPath(diskMap[goodSuperblock->first]);

        /* Just another round of validation.  Being paranoid.  Nothing
         * should fail at this point.  If it fails, not sure how to recover
         * from it.  It will be a bit complicated.
         * So, for now just verify that everything has gone smoothly.
         */
        err = superblockMaster.readSuperblock(goodSuperblockPath);
        fds_verify(ERR_OK == err);
        err = superblockMaster.validateSuperblock();
        fds_verify(ERR_OK == err);
        if (diskBadSuperblock.size() > 0) {
            err = syncSuperblock(diskBadSuperblock);
            fds_verify(ERR_OK == err);
        }
    } else {
        /* TODO(sean)
         * This error detection is a bit more involved.  Should break up this
         * function before working on this.
         * But, for reconciling this case is a bit tricky. Which set of
         * superblocks to trust????  This can theoretically happen if the system
         * goes down when syncing superblocks to disks.
         *
         * So, algorithm should look something like:
         * 1) keep the most recent superblock
         * 2) if most recent superblock still has simple majority (51%), then
         *    we elect that as the master.
         * 3) or if we can't elect, then we need to throw away everything
         *    and recompute and regen superblock.
         */
        fds_panic("more than one unique checksum detected with SM superblocks");
    }

    return err;
}

/* This iface is only used for the SmSuperblock unit test.  Should not
 * be called by anyone else.
 */
std::string
SmSuperblockMgr::SmSuperblockMgrTestGetFileName()
{
    return superblockName;
}

fds_uint16_t
SmSuperblockMgr::getDiskId(fds_token_id smTokId,
                        diskio::DataTier tier) const {
    return superblockMaster.olt.getDiskId(smTokId, tier);
}

SmTokenSet
SmSuperblockMgr::getSmOwnedTokens() const {
    // we will return a copy
    SmTokenSet tokens;
    tokens.insert(ownedTokens.begin(), ownedTokens.end());
    return tokens;
}

SmTokenSet
SmSuperblockMgr::getSmOwnedTokens(fds_uint16_t diskId) const {
    // get all tokens that can reside on disk diskId
    SmTokenSet diskToks = superblockMaster.olt.getSmTokens(diskId);
    // filter tokens that this SM owns
    SmTokenSet retToks;
    for (SmTokenSet::const_iterator cit = diskToks.cbegin();
         cit != diskToks.cend();
         ++cit) {
        if (ownedTokens.count(*cit) > 0) {
            retToks.insert(*cit);
        }
    }
    return retToks;
}

// So we can print class members for logging
std::ostream& operator<< (std::ostream &out,
                          const SmSuperblockMgr& sbMgr) {
    out << "Current disk map:\n" << sbMgr.diskMap;
    out << sbMgr.superblockMaster.olt;
    out << "SM tokens owned by this SM: " << sbMgr.ownedTokens;
    return out;
}

std::ostream& operator<< (std::ostream &out,
                          const DiskLocMap& diskMap) {
    for (auto cit = diskMap.begin(); cit != diskMap.end(); ++cit) {
        out << cit->first << " : " << cit->second << "\n";
    }
    return out;
}

// since DiskIdSet is typedef of std::set, need to specifically overlaod boost ostream
boost::log::formatting_ostream& operator<< (boost::log::formatting_ostream& out,
                                            const DiskIdSet& diskIds) {
    for (auto cit = diskIds.begin(); cit != diskIds.end(); ++cit) {
        out << "[" << *cit << "]";
    }
    out << "\n";
    return out;
}

}  // namespace fds
