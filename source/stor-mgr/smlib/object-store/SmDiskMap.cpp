/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <fds_process.h>
#include <fiu-local.h>
#include <fiu-control.h>
#include <net/SvcMgr.h>
#include <object-store/SmDiskMap.h>
#include <sys/statvfs.h>
#include <fiu-control.h>
#include <fiu-local.h>
#include <utility>
#include <net/SvcMgr.h>
#include <object-store/ObjectMetaDb.h>


namespace fds {

SmDiskMap::SmDiskMap(const std::string& modName, DiskChangeFnObj diskChangeFn)
        : Module(modName.c_str()),
          bitsPerToken_(0),
          superblock(new SmSuperblockMgr(std::move(diskChangeFn))),
          ssdIdxMap(nullptr),
          test_mode(false) {
    for (int i = 0; i < 60; ++i) {
        maxSSDCapacity[i] = ATOMIC_VAR_INIT(0ull);
        consumedSSDCapacity[i] = ATOMIC_VAR_INIT(0ull);
    }
}

SmDiskMap::~SmDiskMap() {
}

int SmDiskMap::mod_init(SysParams const *const param) {
    Module::mod_init(param);
    test_mode = g_fdsprocess->get_fds_config()->get<bool>("fds.sm.testing.standalone");

    // get list of HDD and SSD devices
    getDiskMap();

    // Create mapping from ssd idx # to array idx #
    ssdIdxMap = new std::unordered_map<fds_uint16_t, fds_uint8_t>();
    fds_uint8_t arrIdx = 0;
    for (auto disk_id : ssd_ids) {
        struct statvfs statbuf;
        std::string diskPath = getDiskPath(disk_id);
        if (statvfs(diskPath.c_str(), &statbuf) < 0) {
            LOGERROR << "Could not read disk " << diskPath;
        }
        // Add the mapping
        ssdIdxMap->emplace(disk_id, arrIdx);

        // Populate the capacityMap for this diskID
        fds_uint64_t totalSize = statbuf.f_blocks * statbuf.f_frsize;
        fds_uint64_t consumedSize = totalSize - (statbuf.f_bfree * statbuf.f_bsize);
        maxSSDCapacity[arrIdx] = totalSize;
        consumedSSDCapacity[arrIdx] = consumedSize;
        // Now increase arrIdx
        ++arrIdx;
    }

    return 0;
}

void
SmDiskMap::removeDiskAndRecompute(DiskId& diskId, const diskio::DataTier& tier) {
    /**
     * TODO(Gurpreet) Make access to maps concurrency-safe.
     */
    superblock->recomputeTokensForLostDisk(diskId, hdd_ids, ssd_ids);
    disk_map.erase(diskId);
    diskDevMap.erase(diskId);
    switch (tier) {
        case diskio::diskTier:
            hdd_ids.erase(diskId);
            break;
        case diskio::flashTier:
            ssd_ids.erase(diskId);
            break;
        default:
            LOGDEBUG << "Invalid tier information";
            break;
    }

    /**
     * TODO(Gurpreet): Handle capacity related changes(if required)
     * due to the failed disk.
     */
}

Error
SmDiskMap::loadPersistentState() {

    // Load superblock, tell superblock about disk map
    // it will handle changes in diskmap (vs. its persisted state)
    Error err = superblock->loadSuperblock(hdd_ids, ssd_ids, disk_map, diskDevMap);
    if (err.ok()) {
        LOGDEBUG << "Loaded superblock " << *superblock;
    } else if (err == ERR_SM_NOERR_PRISTINE_STATE) {
        LOGNOTIFY << "SM is coming up from CLEAN state";
    } else {
        LOGERROR << "Failed to load superblock " << err;
    }
    return err;
}

DiskUtils::capacity_tuple SmDiskMap::getDiskConsumedSize(fds_uint16_t disk_id)
{

    // Cause method to return capacity
    fiu_do_on("sm.diskmap.cause_used_capacity_error", \
              fiu_disable("sm.diskmap.cause_used_capacity_alert"); \
              fiu_disable("sm.diskmap.cause_used_capacity_warn"); \
              LOGDEBUG << "Err injection: (" << DISK_CAPACITY_ERROR_THRESHOLD
                       << ", 100). This should cause an alert."; \
              DiskUtils::capacity_tuple retVals(DISK_CAPACITY_ERROR_THRESHOLD, 100); \
              return retVals; \
    );

    fiu_do_on("sm.diskmap.cause_used_capacity_alert", \
              fiu_disable("sm.diskmap.cause_used_capacity_error"); \
              fiu_disable("sm.diskmap.cause_used_capacity_warn"); \
              LOGDEBUG << "Err injection: (" << DISK_CAPACITY_ALERT_THRESHOLD + 1
                       << ", 100). This should cause an alert."; \
              DiskUtils::capacity_tuple retVals(DISK_CAPACITY_ALERT_THRESHOLD + 1, 100); \
              return retVals; \
    );

    fiu_do_on("sm.diskmap.cause_used_capacity_warn", \
              fiu_disable("sm.diskmap.cause_used_capacity_error"); \
              fiu_disable("sm.diskmap.cause_used_capacity_alert"); \
              LOGDEBUG << "Err injection: (" << DISK_CAPACITY_WARNING_THRESHOLD + 1
                       << ", 100). This should cause a warning."; \
              DiskUtils::capacity_tuple retVals(DISK_CAPACITY_WARNING_THRESHOLD + 1, 100); \
              return retVals; \
    );

    bool ssdMetadata = g_fdsprocess->get_fds_config()->get<bool>("fds.sm.testing.useSsdForMeta");

    // Get the total size info for the disk regardless of type
    std::string diskPath = getDiskPath(disk_id);

    DiskUtils::capacity_tuple out = DiskUtils::getDiskConsumedSize(diskPath);

    // If we got an SSD disk id and have HDDs we still need to check metadata size
    if ((ssd_ids.find(disk_id) != ssd_ids.end()) && (hdd_ids.size() != 0)) {
        // Only calculate this value if we've got ssd metadata turned on
        if (ssdMetadata) {
            LOGDEBUG << "SM using SSD for metadata, checking metadata size...";

            // Accumulator
            fds_uint64_t acc = 0;

            SmTokenSet smToks = getSmTokens();
            // Find the LevelDBs
            for (SmTokenSet::const_iterator cit = smToks.cbegin();
                 cit != smToks.cend();
                 ++cit) {
                // Calculate a consumedSize based on the size of the level DBs
                std::string filename = ObjectMetadataDb::getObjectMetaFilename(diskPath, *cit);
                DiskUtils::capacity_tuple tmp = DiskUtils::getDiskConsumedSize(filename, true);

                acc += tmp.first;
            }
            LOGDEBUG << "Returning " << acc << "/" << out.second << " after calculating SSD metadata size.";
            return DiskUtils::capacity_tuple(acc, out.second);
        }
    }

    return out;
}


fds_bool_t SmDiskMap::ssdTrackCapacityAdd(ObjectID oid,
        fds_uint64_t writeSize, fds_uint32_t fullThreshold) {
    fds_uint16_t diskId = getDiskId(oid, diskio::flashTier);
    if (!ObjectLocationTable::isDiskIdValid(diskId)) {
        LOGDEBUG << "No disks on flash tier";
        return false;
    }

    // Get the capacity information for this disk
    fds_uint8_t arrId = ssdIdxMap->at(diskId);
    // Check if we're over threshold now
    fds_uint64_t newConsumed = consumedSSDCapacity[arrId] + writeSize;
    fds_uint64_t capThresh = maxSSDCapacity[arrId] * (fullThreshold / 100.);
    // we use fullThreshold to keep some space on SSD for tiering engine
    // to move hot data there. However, if this is an all-SSD config, there is
    // no tiering and we want to use full SSD capacity
    if (getTotalDisks(diskio::diskTier) == 0) {
        capThresh = maxSSDCapacity[arrId];
    }
    if (newConsumed > capThresh) {
        LOGDEBUG << "SSD write would exceed full threshold: Threshold: "
                    << capThresh << " current usage: " << consumedSSDCapacity[arrId]
                    << " of " << maxSSDCapacity[arrId] << " Write size: " << writeSize;
        return false;
    }
    // Add the writeSize
    consumedSSDCapacity[arrId] += writeSize;
    LOGDEBUG << "new SSD utilized cap for diskID[" << diskId << "]: " << consumedSSDCapacity[arrId];
    return true;
}

void SmDiskMap::ssdTrackCapacityDelete(ObjectID oid, fds_uint64_t writeSize) {
    fds_uint16_t diskId = getDiskId(oid, diskio::flashTier);
    fds_uint8_t arrId = ssdIdxMap->at(diskId);
    // Get the capacity information for this disk
    consumedSSDCapacity[arrId] -= writeSize;
}

void SmDiskMap::mod_startup() {
    Module::mod_startup();
}

void SmDiskMap::mod_shutdown() {
    if (ssdIdxMap) {
        delete ssdIdxMap;
        ssdIdxMap = nullptr;
    }

    Module::mod_shutdown();
}

void SmDiskMap::getDiskMap() {
    int           idx;
    fds_uint64_t  uuid;
    std::string   path, dev;

    const FdsRootDir *dir = g_fdsprocess->proc_fdsroot();
    std::ifstream map(dir->dir_dev() + DISK_MAP_FILE, std::ifstream::in);

    fds_verify(map.fail() == false);
    while (!map.eof()) {
        map >> dev >> idx >> std::hex >> uuid >> std::dec >> path;
        if (map.fail()) {
            break;
        }
        LOGNORMAL << "dev " << dev << ", path " << path << ", uuid " << uuid
                  << ", idx " << idx;
        if (strstr(path.c_str(), "hdd") != NULL) {
            fds_verify(hdd_ids.count(idx) == 0);
            hdd_ids.insert(idx);
        } else if (strstr(path.c_str(), "ssd") != NULL) {
            fds_verify(ssd_ids.count(idx) == 0);
            ssd_ids.insert(idx);
        } else {
            fds_panic("Unknown path: %s\n", path.c_str());
        }
        fds_verify(disk_map.count(idx) == 0);
        disk_map[idx] = path;
        diskDevMap[idx] = dev;
        diskState[idx] = DISK_ONLINE;
    }
    if (disk_map.size() == 0) {
        LOGCRITICAL << "Can't find any devices!";
    }
}

Error SmDiskMap::handleNewDlt(const DLT* dlt)
{
    // get list of DLT tokens that this SM is responsible for
    // according to the DLT
    NodeUuid mySvcUuid;
    if (!test_mode) {
        mySvcUuid = NodeUuid(MODULEPROVIDER()->getSvcMgr()->getSelfSvcUuid());
    } else {
        mySvcUuid = 1;
    }

    return handleNewDlt(dlt, mySvcUuid);
}

Error SmDiskMap::handleNewDlt(const DLT* dlt, NodeUuid& mySvcUuid)
{
    Error err(ERR_OK);
    fds_bool_t firstDlt = false;
    if (bitsPerToken_ == 0) {
        bitsPerToken_ = dlt->getNumBitsForToken();
        firstDlt = true;
    } else {
        // we already got DLT and set initial disk map
        // dlt width should not change
        fds_verify(bitsPerToken_ == dlt->getNumBitsForToken());
    }
    LOGDEBUG << "Will handle new DLT, bits per token " << bitsPerToken_;
    LOGTRACE << *dlt;

    // get DLT tokens that belong to this SM
    const TokenList& dlt_toks = dlt->getTokens(mySvcUuid);

    // here we handle only gaining of ownership of DLT tokens
    // we will handle losing of DLT tokens on DLT close

    // get list of SM tokens that this SM is responsible for
    SmTokenSet sm_toks;
    for (TokenList::const_iterator cit = dlt_toks.cbegin();
         cit != dlt_toks.cend();
         ++cit) {
        sm_toks.insert(smTokenId(*cit));
    }

    // tell superblock about current list of SM tokens this SM
    // owns, and superblock will update token state for all SM
    // tokens that this SM just gained ownership
    // this methods also sets DLT version (even in case when SM does not
    // own any SM tokens)
    err = superblock->updateNewSmTokenOwnership(sm_toks, dlt->getVersion());
    if (err == ERR_SM_NOERR_LOST_SM_TOKENS) {
        // this should only happen on first DLT -- SM may go down between
        // DLT update and DLT close; but for subsequent DLTs that should not happen!
        fds_verify(firstDlt);
    }
    if (dlt_toks.empty()) {
        LOGNOTIFY << "DLT received does not contain this SM, not updating"
                 << " token ownership in superblock";
        return ERR_SM_NOERR_NOT_IN_DLT;
    }

    return err;
}

SmTokenSet SmDiskMap::handleDltClose(const DLT* dlt)
{
    // get list of DLT tokens that this SM is responsible for
    // according to the DLT
    NodeUuid mySvcUuid;
    if (!test_mode) {
        mySvcUuid = NodeUuid(MODULEPROVIDER()->getSvcMgr()->getSelfSvcUuid());
    } else {
        mySvcUuid = 1;
    }

    return handleDltClose(dlt, mySvcUuid);
}

SmTokenSet SmDiskMap::handleDltClose(const DLT* dlt, NodeUuid& mySvcUuid)
{
    // get DLT tokens that belong to this SM
    const TokenList& dlt_toks = dlt->getTokens(mySvcUuid);

    // here we are handling losing ownership of DLT / SM tokens
    SmTokenSet smToksNotOwned;
    // get list of SM tokens that SM does not own
    for (fds_token_id tok = 0; tok < SMTOKEN_COUNT; ++tok) {
        smToksNotOwned.insert(tok);
    }
    for (TokenList::const_iterator cit = dlt_toks.cbegin();
         cit != dlt_toks.cend();
         ++cit) {
        smToksNotOwned.erase(smTokenId(*cit));
    }

    // tell superblock about current list of SM tokens this SM
    // owns, and superblock will update token state for all SM
    // tokens that this SM just gained ownership
    // this methods also sets DLT version (even in case when SM does not
    // own any SM tokens)
    SmTokenSet rmToks = superblock->handleRemovedSmTokens(smToksNotOwned,
                                                          dlt->getVersion());

    // return SM tokens that were just invalidated
    return rmToks;
}

fds_uint64_t
SmDiskMap::getDLTVersion()
{
    return superblock->getDLTVersion();
}

fds_token_id
SmDiskMap::smTokenId(fds_token_id tokId) {
    return tokId & SMTOKEN_MASK;
}

fds_token_id
SmDiskMap::smTokenId(const ObjectID& objId) const {
    fds_token_id tokId = DLT::getToken(objId, bitsPerToken_);
    return tokId & SMTOKEN_MASK;
}

fds_token_id
SmDiskMap::smTokenId(const ObjectID& objId,
                     fds_uint32_t bitsPerToken) {
    fds_token_id tokId = DLT::getToken(objId, bitsPerToken);
    return tokId & SMTOKEN_MASK;
}

SmTokenSet
SmDiskMap::getSmTokens() const {
    return superblock->getSmOwnedTokens();
}

SmTokenSet
SmDiskMap::getSmTokens(fds_uint16_t diskId) const {
    return superblock->getSmOwnedTokens(diskId);
}

fds_uint16_t
SmDiskMap::getDiskId(const ObjectID& objId,
                     diskio::DataTier tier) const {
    fds_token_id smTokId = smTokenId(objId);
    return superblock->getDiskId(smTokId, tier);
}

fds_uint16_t
SmDiskMap::getDiskId(fds_token_id smTokId,
                     diskio::DataTier tier) const {
    return superblock->getDiskId(smTokId, tier);
}

std::string
SmDiskMap::getDiskPath(fds_token_id smTokId,
                       diskio::DataTier tier) const {
    fds_uint16_t diskId = superblock->getDiskId(smTokId, tier);
    if (disk_map.count(diskId) == 0) {
        return std::string();
    }
    return disk_map.at(diskId);
}

std::string
SmDiskMap::getDiskPath(fds_uint16_t diskId) const {
    if (disk_map.count(diskId) == 0) {
        return std::string();
    }
    return disk_map.at(diskId);
}

DiskIdSet
SmDiskMap::getDiskIds(diskio::DataTier tier) const {
    DiskIdSet diskIds;
    if (tier == diskio::diskTier) {
        diskIds.insert(hdd_ids.begin(), hdd_ids.end());
    } else if (tier == diskio::flashTier) {
        diskIds.insert(ssd_ids.begin(), ssd_ids.end());
    } else {
        fds_panic("Unknown tier request from SM disk map\n");
    }
    return diskIds;
}

DiskIdSet
SmDiskMap::getDiskIds() const {
    DiskIdSet diskIds;

    if (hdd_ids.size() > 0) {
        diskIds.insert(hdd_ids.begin(), hdd_ids.end());
    }

    if (ssd_ids.size() > 0) {
        diskIds.insert(ssd_ids.begin(), ssd_ids.end());
    }

    return diskIds;
}

fds_uint32_t
SmDiskMap::getTotalDisks() const {
    return (hdd_ids.size() + ssd_ids.size());
}

fds_uint32_t
SmDiskMap::getTotalDisks(diskio::DataTier tier) const {
    if (tier == diskio::flashTier) {
        return ssd_ids.size();
    } else if (tier == diskio::diskTier) {
        return hdd_ids.size();
    }
    return 0;
}

bool
SmDiskMap::isAllDisksSSD() const {
    return ((ssd_ids.size() > 0) && (hdd_ids.size() == 0));
}

diskio::DataTier
SmDiskMap::diskMediaType(const DiskId& diskId) const {
    if (hdd_ids.find(diskId) != hdd_ids.end()) {
        return diskio::diskTier;
    } else if (ssd_ids.find(diskId) != ssd_ids.end()) {
        return diskio::flashTier;
    } else {
        return diskio::maxTier;
    }
}

}  // namespace fds
