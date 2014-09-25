/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <fds_process.h>
#include <object-store/SmDiskMap.h>

namespace fds {

SmDiskMap::SmDiskMap(const std::string& modName)
        : Module(modName.c_str()),
          bitsPerToken_(0) {
}

SmDiskMap::~SmDiskMap() {
}

int SmDiskMap::mod_init(SysParams const *const param) {
    Module::mod_init(param);
    return 0;
}

void SmDiskMap::mod_startup() {
    // get list of HDD and SSD devices
    getDiskMap();

    // read superblock; if we bring up SM from clean
    // state, we should populate ObjectLocationTable
}

void SmDiskMap::mod_shutdown() {
}

void SmDiskMap::getDiskMap() {
    int           idx;
    fds_uint64_t  uuid;
    std::string   path, dev;

    const FdsRootDir *dir = g_fdsprocess->proc_fdsroot();
    std::ifstream map(dir->dir_dev() + std::string("/disk-map"), std::ifstream::in);

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

void SmDiskMap::handleNewDlt(const DLT* dlt) {
    if (bitsPerToken_ == 0) {
        bitsPerToken_ = dlt->getNumBitsForToken();
    } else {
        // dlt width should not change
        fds_verify(bitsPerToken_ == dlt->getNumBitsForToken());
    }
    LOGDEBUG << "Will handle new DLT, bits per token " << bitsPerToken_;
}

fds_token_id
SmDiskMap::smTokenId(fds_token_id tokId) {
    return tokId & SM_TOKEN_MASK;
}

fds_token_id
SmDiskMap::smTokenId(const ObjectID& objId) {
    fds_token_id tokId = DLT::getToken(objId, bitsPerToken_);
    return tokId & SM_TOKEN_MASK;
}

fds_uint16_t
SmDiskMap::getDiskId(const ObjectID& objId,
                     diskio::DataTier tier) {
    return 0;
}

fds_uint16_t
SmDiskMap::getDiskId(fds_token_id tokId,
                     diskio::DataTier tier) {
    return 0;
}

const char*
SmDiskMap::getDiskPath(fds_token_id tokId,
                       diskio::DataTier tier) {
    return NULL;
}

}  // namespace fds
