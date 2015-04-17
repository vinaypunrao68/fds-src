/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <fds_process.h>
#include <net/SvcMgr.h>
#include <object-store/SmDiskMap.h>
#include <sys/statvfs.h>
#include <utility>
#include <net/SvcMgr.h>

namespace fds {

SmDiskMap::SmDiskMap(const std::string& modName)
        : Module(modName.c_str()),
          bitsPerToken_(0),
          superblock(new SmSuperblockMgr()),
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

Error
SmDiskMap::loadPersistentState() {
    // Load superblock, tell superblock about disk map
    // it will handle changes in diskmap (vs. its persisted state)
    Error err = superblock->loadSuperblock(hdd_ids, ssd_ids, disk_map);
    if (err.ok()) {
        LOGDEBUG << "Loaded superblock " << *superblock;
    } else {
        LOGERROR << "Failed to load superblock " << err;
    }
    return err;
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
    }
    if (disk_map.size() == 0) {
        fds_panic("Can't find any devices\n");
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
    if (bitsPerToken_ == 0) {
        bitsPerToken_ = dlt->getNumBitsForToken();
    } else {
        // we already got DLT and set initial disk map
        // dlt width should not change
        fds_verify(bitsPerToken_ == dlt->getNumBitsForToken());
    }
    LOGDEBUG << "Will handle new DLT, bits per token " << bitsPerToken_;
    LOGTRACE << *dlt;


    // get DLT tokens that belong to this SM
    const TokenList& dlt_toks = dlt->getTokens(mySvcUuid);

    // If this is restart, DLT version stored in superblock must be the same
    // as DLT version we received from OM now. If OM currently tries to update
    // DLT and at least one SM node is down, DLT will be aborted and DLT change
    // will not happen. TODO(Anna) make sure to handle the case when DLT changes
    // during SM down if OM code changes...
    fds_uint64_t storedDltVersion = getDLTVersion();
    if (storedDltVersion == dlt->getVersion()) {
        // this is restart case
        // TODO(Anna) FS-1605 check if superblock matches with this DLT
        LOGDEBUG << "Nothing to do if superblock knows about this DLT version "
                 << storedDltVersion;
        return err;
    }
    
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
    fds_verify(disk_map.count(diskId) > 0);
    return disk_map.at(diskId);
}

std::string
SmDiskMap::getDiskPath(fds_uint16_t diskId) const {
    fds_verify(disk_map.count(diskId) > 0);
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

}  // namespace fds
