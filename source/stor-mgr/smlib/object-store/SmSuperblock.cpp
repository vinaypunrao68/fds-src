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
#include <dlt.h>
#include <object-store/SmTokenPlacement.h>
#include <object-store/SmSuperblock.h>
extern "C" {
#include <fcntl.h>
}

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

    DLTVersion = DLT_VER_INVALID;
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

SmSuperblockMgr::SmSuperblockMgr(DiskChangeFnObj diskChangeFunc)
        : noDltReceived(true),
          diskChangeFn(diskChangeFunc)
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
SmSuperblockMgr::loadSuperblock(DiskIdSet& hddIds,
                                DiskIdSet& ssdIds,
                                const DiskLocMap& latestDiskMap,
                                const DiskLocMap& latestDiskDevMap)
{
    Error err(ERR_OK);

    SCOPEDWRITE(sbLock);

    /* There should be at least one disk in the map.
     */
    fds_assert(latestDiskMap.size() > 0);

    /* Cache the diskMap to a local storage.
     */
    diskMap = latestDiskMap;
    diskDevMap = latestDiskDevMap;
    setDiskHealthMap();
    LOGDEBUG << "Got disk map";

    /* Do initial state check.
     * If there is no superblock anywhere on the system, then this is the
     * first time SM started on this node.
     */
    bool pristineState = checkPristineState(hddIds, ssdIds);
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
        err = ERR_SM_NOERR_PRISTINE_STATE;
    } else {
        /* Reconcile superblock().  Elect one "master" superblock, if
         * possible.  Failure to reconcile should be a *very* rare case.
         */
        err = reconcileSuperblock();
        if (err != ERR_OK) {
            LOGERROR << "Error during superblock reconciliation error code = " << err;
            checkForHandledErrors(err);
        }

        /* Since, we have a "master" superblock, check if the disk topology
         * has changed from the last persisten state.  For example:
         * 1) disk(s) were added.
         * 2) disk(s) were removed.
         * 3) 1 and 2.
         */
        checkDiskTopology(hddIds, ssdIds);
    }

    return err;
}

void
SmSuperblockMgr::recomputeTokensForLostDisk(DiskIdSet& hddIds, DiskIdSet& ssdIds) {
    SCOPEDWRITE(sbLock);
    checkDiskTopology(hddIds, ssdIds);
}

Error
SmSuperblockMgr::updateNewSmTokenOwnership(const SmTokenSet& smTokensOwned,
                                           fds_uint64_t dltVersion) {
    Error err(ERR_OK);

    SCOPEDWRITE(sbLock);
    // Restarted SM may receive a greater version from OM if SM previously failed (shutdown)
    // and OM rotated DLT columns to move this SM to a secondary position.
    // Note that in this case (or in all cases when we receive DLT after restart),
    // SM must not gain any tokens than it already knows about in the superblock.
    // Restarted SM may also receive a lower version of DLT from OM if the domain
    // shutdown in the middle of token migration, specifically between DLT commit
    // and DLT close. In that case, OM will re-start migration later, but domain
    // starts on the previous version (as if we aborted)
    if ( (dltVersion == superblockMaster.DLTVersion) ||
         ( (superblockMaster.DLTVersion != DLT_VER_INVALID) &&
           noDltReceived ) ) {
        // this is restart case, otherwise all duplicate DLTs are catched at upper layers
        fds_verify(superblockMaster.DLTVersion != DLT_VER_INVALID);
        // this is the extra check that SM is just coming up from persistent state
        // and we received the first dlt
        if (!noDltReceived) {
            LOGNORMAL << "Superblock already handled this DLT version " << dltVersion;
            return ERR_DUPLICATE;
        }

        if (dltVersion < superblockMaster.DLTVersion) {
            LOGNOTIFY << "First DLT on SM restart is lower than stored in superblock "
                      << " most likely SM went down in the middle of migration, between"
                      << " DLT commit and DLT close. Will sync based on DLT we got from OM."
                      << " Received DLT version " << dltVersion
                      << " DLT version in superblock " << superblockMaster.DLTVersion;
        } else if (dltVersion > superblockMaster.DLTVersion) {
            LOGNOTIFY << "First DLT on SM restart is higher than stored in superblock."
                      << " Most likely OM rotated DLT due to this SM failure"
                      << " Received DLT version " << dltVersion
                      << " DLT version in superblock " << superblockMaster.DLTVersion;
        }

        // Check if superblock matches with this DLT
        // We care that all tokens that owned by this SM are "valid" in superblock
        // If SM went down between DLT update and DLT close, then we may have not
        // updated SM tokens that became invalid. I think that's fine. We
        // are going to check if all smTokensOwned are marked 'valid' in superblock
        // and invalidate all other tokens (invalidation will be done by the caller)
        // Similar SM may lose DLT tokens between restarts if SM failed and OM moved
        // this SM to secondary position or completely removed it from some DLT columns
        // However, SM must bever gain DLT tokens between restarts
        err = superblockMaster.tokTbl.checkSmTokens(smTokensOwned);
        if (err == ERR_SM_SUPERBLOCK_INCONSISTENT) {
            LOGERROR << "Superblock knows about this DLT version " << dltVersion
                     << ", but at least one token that must be valid in SM "
                     << " based on DLT version is marked invalid in superblock! ";
        } else if (err == ERR_SM_NOERR_LOST_SM_TOKENS) {
            LOGNOTIFY << "Superblock knows about this DLT version " << dltVersion
                      << ", but at least one token that should be invalid in SM "
                      << " based on DLT version is marked valid in superblock. ";
        } else {
            fds_verify(err.ok());
            // if this is the first token ownership notify since the start and SM not in
            // pristine state, tell the caller
            // dltVersion must match dlt version in superblock only in that case above
            err = ERR_SM_NOERR_NEED_RESYNC;
            LOGDEBUG << "Superblock knows about this DLT version " << dltVersion;
        }
        if (!err.ok()) {
            // if error, more logging for debugging
            for (fds_token_id tokId = 0; tokId < SMTOKEN_COUNT; ++tokId){
                LOGNOTIFY << "SM token " << tokId << " must be valid? " << (smTokensOwned.count(tokId) > 0)
                          << "; valid in superblock? " << superblockMaster.tokTbl.isValidOnAnyTier(tokId);
            }
        }
        if ((err != ERR_SM_SUPERBLOCK_INCONSISTENT) && (dltVersion != superblockMaster.DLTVersion)) {
            // make sure we save DLT version in superblock
            setDLTVersionLockHeld(dltVersion, true);
        }
    }
    noDltReceived = false;

    if (!err.ok()) {
        // we either already handled the update or error happened
        return err;
    }

    fds_bool_t initAtLeastOne = superblockMaster.tokTbl.initializeSmTokens(smTokensOwned);
    superblockMaster.DLTVersion = dltVersion;

    // sync superblock
    err = syncSuperblock();
    if (err.ok()) {
        LOGDEBUG << "SM persistent SM token ownership and DLT version="
                 << superblockMaster.DLTVersion;
        if (initAtLeastOne) {
            err = ERR_SM_NOERR_GAINED_SM_TOKENS;
        }
    } else {
        // TODO(Sean):  If the DLT version cannot be persisted, then for not, we will
        //              just ignore it.  We can retry, but the chance of failure is
        //              high at this point, since the disk is likely failed or full.
        //
        // TODO(Sean):  Make sure if the latest DLT version > persistent DLT version is ok.
        //              and change DLT and DISK mapping accordingly.
        LOGCRITICAL << "SM persistent DLT version failed to set: version "
                    << dltVersion;
    }

    return err;
}

SmTokenSet
SmSuperblockMgr::handleRemovedSmTokens(SmTokenSet& smTokensNotOwned,
                                       fds_uint64_t dltVersion) {
    SCOPEDWRITE(sbLock);

    // DLT version must be already set!
    fds_verify(dltVersion == superblockMaster.DLTVersion);

    // invalidate token state for tokens that are not owned
    SmTokenSet lostSmTokens = superblockMaster.tokTbl.invalidateSmTokens(smTokensNotOwned);

    // sync superblock
    Error err = syncSuperblock();
    if (!err.ok()) {
        // We couldn't persist tokens that are not valid anymore
        // For now ignoring it! We should be ok later recovering from this
        // inconsistency since we can always check SM ownership from DLT
        LOGCRITICAL << "Failed to persist newly invalidated SM tokens";
    }

    return lostSmTokens;
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

/**
 * This method tries to check if the disks passed
 * are accessible or not.
 * It creates a mount point and try to mount the
 * device path on it. If it fails with device not
 * found error. Then basically, device isn't there.
 * Secondly, it tries to create a file and write to
 * it and do a flush to the hardware. If the flush
 * raises io exception. Then disk is assumed to be
 * inaccessible.
 * Basically either of the test failure for a disk
 * results in marking disk as bad.
 */
void
SmSuperblockMgr::checkDisksAlive(DiskIdSet& HDDs,
                                 DiskIdSet& SSDs) {
    DiskIdSet badDisks;
    std::string tempMountDir = MODULEPROVIDER()->proc_fdsroot()->\
                               dir_fds_etc() + "testDevMount";
    FdsRootDir::fds_mkdir(tempMountDir.c_str());
    umount2(tempMountDir.c_str(), MNT_FORCE);

    LOGDEBUG << "Do mount test on disks";
    // check for unreachable HDDs first.
    for (auto& diskId : HDDs) {
        if (isDiskUnreachable(diskId, tempMountDir)) {
            badDisks.insert(diskId);
        }
    }
    for (auto& badDiskId : badDisks) {
        LOGDEBUG << "Disk with diskId = " << badDiskId << " is unaccessible";
        markDiskBad(badDiskId);
        HDDs.erase(badDiskId);
        diskMap.erase(badDiskId);
    }
    badDisks.clear();

    // check for unreachable SSDs.
    for (auto& diskId : SSDs) {
        if (isDiskUnreachable(diskId, tempMountDir)) {
            badDisks.insert(diskId);
        }
    }
    for (auto& badDiskId : badDisks) {
        LOGDEBUG << "Disk with diskId = " << badDiskId << " is unaccessible";
        markDiskBad(badDiskId);
        SSDs.erase(badDiskId);
        diskMap.erase(badDiskId);
    }
    deleteMount(tempMountDir);
}

bool
SmSuperblockMgr::diskFileTest(const std::string& path) {

    int fd = open(path.c_str(), O_RDWR | O_CREAT | O_SYNC, S_IRUSR | S_IWUSR);
    if (fd == -1 || fsync(fd) ||close(fd)) {
        LOGDEBUG << "File test for disk = " << path << " failed with errno = " << errno;
        return true;
    } else {
        return false;
    }
}

bool
SmSuperblockMgr::isDiskUnreachable(const fds_uint16_t& diskId,
                                   const std::string& mountPnt) {
    std::string path = diskMap[diskId] + "/.tempFlush";
    bool retVal = diskFileTest(path);
    std::remove(path.c_str());
    if (mount(diskDevMap[diskId].c_str(), mountPnt.c_str(), "xfs", MS_RDONLY, nullptr)) {
        if (errno == ENODEV) {
            LOGNOTIFY << "Disk " << diskId << " is not accessible ";
            return  (retVal | true);
        }
    } else {
        umount2(mountPnt.c_str(), MNT_FORCE);
    }
    return (retVal | false);
}

/*
 * Determine if the node's disk topology has changed or not.
 *
 */
void
SmSuperblockMgr::checkDiskTopology(DiskIdSet& newHDDs,
                                   DiskIdSet& newSSDs)
{
    Error err(ERR_OK);
    DiskIdSet persistentHDDs, persistentSSDs;
    DiskIdSet addedHDDs, removedHDDs;
    DiskIdSet addedSSDs, removedSSDs;
    bool recomputed = false;
    LOGDEBUG << "Checking disk topology";

    /**
     * Check for all HDDs and SSDs passed to SM via diskMap
     * are up and accessible. Remove the bad ones from SSD
     * and/or HDD DiskIdSet.
     */
    checkDisksAlive(newHDDs, newSSDs);

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

    /* Check if the disk topology has changed.
     */
    if ((removedHDDs.size() > 0) ||
        (addedHDDs.size() > 0)) {
        LOGNOTIFY << "Disk Topology Changed: removed HDDs=" << removedHDDs.size()
                  << ", added HDDs=" << addedHDDs.size();
        for (auto &removedDiskId : removedHDDs) {
            LOGNOTIFY <<"Disk HDD=" << removedDiskId << " removed";
            if (g_fdsprocess->
                    get_fds_config()->
                        get<bool>("fds.sm.testing.useSsdForMeta")) {
                SmTokenSet lostSmTokens = getTokensOfThisSM(removedDiskId);
                std::set<std::pair<fds_token_id, fds_uint16_t>> smTokenDiskIdPairs;
                for (auto& lostSmToken : lostSmTokens) {
                    auto metaDiskId = superblockMaster.olt.getDiskId(lostSmToken,
                                                                 diskio::flashTier);
                    changeTokenCompactionState(lostSmToken, diskio::diskTier, false, 0);
                    smTokenDiskIdPairs.insert(std::make_pair(lostSmToken, metaDiskId));
                }
                if (diskChangeFn) {
                    diskChangeFn(diskio::diskTier, smTokenDiskIdPairs);
                }
            }
        }
        recomputed |= SmTokenPlacement::recompute(persistentHDDs,
                                                  addedHDDs,
                                                  removedHDDs,
                                                  diskio::diskTier,
                                                  &(superblockMaster.olt),
                                                  err);
        if (!err.ok()) {
            LOGCRITICAL << "Redistribution of data failed with error " << err;
            return;
        }
    }

    if ((removedSSDs.size() > 0) ||
        (addedSSDs.size() > 0)) {
        LOGNOTIFY << "Disk Topology Changed: removed SSDs=" << removedSSDs.size()
                  << ", added SSDs=" << addedSSDs.size();
        for (auto &removedDiskId : removedSSDs) {
            LOGNOTIFY <<"Disk SSD=" << removedDiskId << " removed";
            if (g_fdsprocess->
                    get_fds_config()->
                        get<bool>("fds.sm.testing.useSsdForMeta")) {
                SmTokenSet lostSmTokens = getTokensOfThisSM(removedDiskId);
                std::set<std::pair<fds_token_id, fds_uint16_t>> smTokenDiskIdPairs;
                for (auto& lostSmToken : lostSmTokens) {
                    auto diskId = superblockMaster.olt.getDiskId(lostSmToken,
                                                                 diskio::diskTier);
                    smTokenDiskIdPairs.insert(std::make_pair(lostSmToken, diskId));
                }
                if (diskChangeFn) {
                    diskChangeFn(diskio::flashTier, smTokenDiskIdPairs);
                }
            }
        }
        recomputed |= SmTokenPlacement::recompute(persistentSSDs,
                                                  addedSSDs,
                                                  removedSSDs,
                                                  diskio::flashTier,
                                                  &(superblockMaster.olt),
                                                  err);
        if (!err.ok()) {
            LOGCRITICAL << "Redistribution of data failed with error " << err;
            return;
        }
    }

    /* Token mapping is recomputed.  Now sync out to disk. */
    if (recomputed) {
        err = syncSuperblock();
        /* For now, panic if cannot sync superblock to disks.
         * Need to understand why this can fail at this point.
         */
        fds_verify(err == ERR_OK);
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

void
SmSuperblockMgr::setDiskHealthMap()
{ 
    for (auto cit = diskMap.begin(); cit != diskMap.end(); ++cit) {
        diskHealthMap[cit->first] = true;
    }
}

void
SmSuperblockMgr::markDiskBad(const fds_uint16_t& diskId) {
    diskHealthMap[diskId] = false;
}

bool
SmSuperblockMgr::isDiskHealthy(const fds_uint16_t& diskId) {
    return diskHealthMap[diskId];
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
SmSuperblockMgr::checkPristineState(DiskIdSet& HDDs,
                                    DiskIdSet& SSDs)
{
    uint32_t noSuperblockCnt = 0;
    fds_assert(diskMap.size() > 0);

    /**
     * Check for all HDDs and SSDs passed to SM via diskMap
     * are up and accessible. Remove the bad ones from SSD
     * and/or HDD DiskIdSet.
     * Only checking for file path could sometimes be incorrect
     * since the filepath check request could've been served from
     * filesystem cache, when the actual underlying device is
     * gone.
     */
    std::string tempMountDir = getTempMount();

    for (auto cit = diskMap.begin(); cit != diskMap.end(); ++cit) {
        std::string superblockPath = getSuperblockPath(cit->second.c_str());

        /* Check if the file exists.
         */
        int fd = open(superblockPath.c_str(), O_RDWR | O_SYNC, S_IRUSR | S_IWUSR);
        if (fd == -1 || fsync(fd) || close(fd)) {
            LOGDEBUG << "Superblock file " << superblockPath << " is not accessible";
            ++noSuperblockCnt;
        }
    }
    deleteMount(tempMountDir);
    return (noSuperblockCnt == diskMap.size());
}

std::string
SmSuperblockMgr::getTempMount() {

    std::string tempMountDir = MODULEPROVIDER()->proc_fdsroot()->\
                               dir_fds_etc() + "testMount";
    FdsRootDir::fds_mkdir(tempMountDir.c_str());
    umount2(tempMountDir.c_str(), MNT_FORCE);
    return tempMountDir;
}

void
SmSuperblockMgr::deleteMount(std::string& path) {
    umount2(path.c_str(), MNT_FORCE);
    boost::filesystem::remove_all(path.c_str());
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
            checkForHandledErrors(err);
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
            checkForHandledErrors(err);
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
            checkForHandledErrors(err);
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

void
SmSuperblockMgr::checkForHandledErrors(Error& err) {
    switch (err.GetErrno()) {
        case ERR_SM_SUPERBLOCK_READ_FAIL:
        case ERR_SM_SUPERBLOCK_WRITE_FAIL:
        case ERR_SM_SUPERBLOCK_MISSING_FILE:
            /**
             * Disk failures are handled in the code. So
             * if a superblock file is not reachable just
             * ignore the error.
             */
            err = ERR_OK;
            break;
        default:
            break;
    }
}

Error
SmSuperblockMgr::setDLTVersionLockHeld(fds_uint64_t dltVersion, bool syncImmediately)
{
    Error err(ERR_OK);
    superblockMaster.DLTVersion = dltVersion;

    if (syncImmediately) {
        // sync superblock
        err = syncSuperblock();

        if (err.ok()) {
            LOGDEBUG << "SM persistent DLT version=" << superblockMaster.DLTVersion;
        } else {
            // TODO(Sean):  If the DLT version cannot be persisted, then for not, we will
            //              just ignore it.  We can retry, but the chance of failure is
            //              high at this point, since the disk is likely failed or full.
            //
            // TODO(Sean):  Make sure if the latest DLT version > persistent DLT version is ok.
            //              and change DLT and DISK mapping accordingly.
            LOGCRITICAL << "SM persistent DLT version failed to set.";
        }
    }

    return err;
}

Error
SmSuperblockMgr::setDLTVersion(fds_uint64_t dltVersion, bool syncImmediately) {
    SCOPEDWRITE(sbLock);
    return setDLTVersionLockHeld(dltVersion, syncImmediately);
}

fds_uint64_t
SmSuperblockMgr::getDLTVersion()
{
    SCOPEDREAD(sbLock);

    return superblockMaster.DLTVersion;
}


fds_uint16_t
SmSuperblockMgr::getDiskId(fds_token_id smTokId,
                        diskio::DataTier tier) {
    SCOPEDREAD(sbLock);
    return superblockMaster.olt.getDiskId(smTokId, tier);
}

fds_uint16_t
SmSuperblockMgr::getWriteFileId(fds_token_id smToken,
                                diskio::DataTier tier) {
    SCOPEDREAD(sbLock);
    return superblockMaster.tokTbl.getWriteFileId(smToken, tier);
}

fds_bool_t
SmSuperblockMgr::compactionInProgress(fds_token_id smToken,
                                      diskio::DataTier tier) {
    SCOPEDREAD(sbLock);
    return superblockMaster.tokTbl.isCompactionInProgress(smToken, tier);
}

Error
SmSuperblockMgr::changeCompactionState(fds_token_id smToken,
                                       diskio::DataTier tier,
                                       fds_bool_t inProg,
                                       fds_uint16_t newFileId) {
    SCOPEDWRITE(sbLock);
    return changeTokenCompactionState(smToken, tier, inProg, newFileId);
}

Error
SmSuperblockMgr::changeTokenCompactionState(fds_token_id smToken,
                                            diskio::DataTier tier,
                                            fds_bool_t inProg,
                                            fds_uint16_t newFileId) {
    Error err(ERR_OK);
    superblockMaster.tokTbl.setCompactionState(smToken, tier, inProg);
    if (inProg) {
        superblockMaster.tokTbl.setWriteFileId(smToken, tier, newFileId);
    }
    // sync superblock
    err = syncSuperblock();
    return err;
}

SmTokenSet
SmSuperblockMgr::getSmOwnedTokens() {
    SCOPEDREAD(sbLock);
    return superblockMaster.tokTbl.getSmTokens();
}

SmTokenSet
SmSuperblockMgr::getSmOwnedTokens(fds_uint16_t diskId) {
    SCOPEDREAD(sbLock);
    return getTokensOfThisSM(diskId);
}

SmTokenSet
SmSuperblockMgr::getTokensOfThisSM(fds_uint16_t diskId) {
    // get all tokens that can reside on disk diskId
    SmTokenSet diskToks = superblockMaster.olt.getSmTokens(diskId);
    // filter tokens that this SM owns
    SmTokenSet ownedTokens = superblockMaster.tokTbl.getSmTokens();
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
    out << "Current DLT Version=" << sbMgr.superblockMaster.DLTVersion << "\n";
    out << "Current disk map:\n" << sbMgr.diskMap;
    out << sbMgr.superblockMaster.olt;
    out << sbMgr.superblockMaster.tokTbl;
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
std::ostream& operator<< (std::ostream& out,
                          const DiskIdSet& diskIds) {
    for (auto cit = diskIds.begin(); cit != diskIds.end(); ++cit) {
        out << "[" << *cit << "] ";
    }
    out << "\n";
    return out;
}

// since DiskIdSet is typedef of std::set, need to specifically overlaod boost ostream
boost::log::formatting_ostream& operator<< (boost::log::formatting_ostream& out,
                                            const DiskIdSet& diskIds) {
    for (auto cit = diskIds.begin(); cit != diskIds.end(); ++cit) {
        out << "[" << *cit << "] ";
    }
    out << "\n";
    return out;
}

/* This iface is only used for the SmSuperblock unit test.  Should not
 * be called by anyone else.
 */
std::string
SmSuperblockMgr::SmSuperblockMgrTestGetFileName()
{
    return superblockName;
}

}  // namespace fds
