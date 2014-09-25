/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
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
}

void SmDiskMap::mod_shutdown() {
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
