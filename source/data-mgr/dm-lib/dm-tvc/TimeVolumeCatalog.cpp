/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <dm-tvc/TimeVolumeCatalog.h>
#include <string>

namespace fds {

DmTimeVolCatalog::DmTimeVolCatalog(const std::string &name)
        : Module(name.c_str()) {
}

DmTimeVolCatalog::~DmTimeVolCatalog() {
}

/**
 * Module initialization
 */
int
DmTimeVolCatalog::mod_init(SysParams const *const p) {
    Module::mod_init(p);
    return 0;
}

/**
 * Module startup
 */
void
DmTimeVolCatalog::mod_startup() {
}

/**
 * Module shutdown
 */
void
DmTimeVolCatalog::mod_shutdown() {
}

Error
DmTimeVolCatalog::startBlobTx(const std::string &blobName,
                              BlobTxId::const_ptr txDesc) {
    LOGDEBUG << "Starting " << blobName
             << "transaction " << *txDesc;
    return ERR_OK;
}

DmTimeVolCatalog gl_DmTvcMod("Global DM TVC");

}  // namespace fds
