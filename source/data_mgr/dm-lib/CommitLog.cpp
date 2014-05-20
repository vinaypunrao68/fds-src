/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <CommitLog.h>

namespace fds {

DmCommitLog::DmCommitLog(const std::string &name)
        : Module(name.c_str()) {
}

DmCommitLog::~DmCommitLog() {
}

/**
 * Module initialization
 */
int
DmCommitLog::mod_init(SysParams const *const p) {
    Module::mod_init(p);
    return 0;
}

/**
 * Module startup
 */
void
DmCommitLog::mod_startup() {
}

/**
 * Module shutdown
 */
void
DmCommitLog::mod_shutdown() {
}

Error
DmCommitLog::openTrans(BlobTxId::const_ptr txDesc) {
    return ERR_OK;
}

Error
DmCommitLog::commitTrans(BlobTxId::const_ptr txDesc) {
    return ERR_OK;
}

}  // namespace fds
