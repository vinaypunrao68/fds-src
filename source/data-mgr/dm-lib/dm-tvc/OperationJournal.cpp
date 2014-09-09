/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>

#include <boost/scoped_ptr.hpp>

#include <fds_module.h>
#include <fds_process.h>
#include <dm-tvc/OperationJournal.h>
#include <serialize.h>

namespace fds {

DmTvcOperationJournal::DmTvcOperationJournal(fds_volid_t volId,
        fds_uint32_t filesize /* = ObjectLogger::FILE_SIZE */,
        fds_uint32_t maxFiles /* = ObjectLogger::MAX_FILES */) : volId_(volId) {
    std::ostringstream oss;

    const FdsRootDir * root = g_fdsprocess->proc_fdsroot();
    oss << root->dir_user_repo_dm() << volId;
    FdsRootDir::fds_mkdir(oss.str().c_str());

    oss << "/" << volId << ".journal";
    logger_.reset(new ObjectLogger(oss.str(), filesize, maxFiles));
}

void DmTvcOperationJournal::log(CommitLogTx::const_ptr tx, fds_uint64_t index /* = 0 */) {
    SCOPEDWRITE(logLock_);
    off_t pos = logger_->log(tx.get());
    if (pos >= 0) {
        return;
    } else {
        SCOPEDWRITE(rotateLock_);
        logger_->rotate();
    }
    pos = logger_->log(tx.get());
    fds_assert(pos >= 0);
}
}   // namespace fds
