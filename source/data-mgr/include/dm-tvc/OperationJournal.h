/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_DM_TVC_OPERATIONJOURNAL_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_TVC_OPERATIONJOURNAL_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string>

#include <boost/shared_ptr.hpp>

#include <fds_types.h>
#include <serialize.h>
#include <ObjectLogger.h>
#include <dm-tvc/CommitLog.h>

namespace fds {

/**
 * Journal of all write operations pushed to VolumeCatalog
 */
class DmTvcOperationJournal {
  public:
    typedef boost::shared_ptr<DmTvcOperationJournal> ptr;
    typedef boost::shared_ptr<const DmTvcOperationJournal> const_ptr;

    // ctor & dtor
    DmTvcOperationJournal(fds_volid_t volId, fds_uint32_t filesize = ObjectLogger::FILE_SIZE,
            fds_uint32_t maxFiles = ObjectLogger::MAX_FILES);

    virtual ~DmTvcOperationJournal() {}

    inline fds_volid_t volId() const {
        return volId_;
    }

    inline const fds_uint32_t filesize() const {
        fds_assert(logger_);
        return logger_->filesize();
    }

    inline const fds_uint32_t maxFiles() const {
        fds_assert(logger_);
        return logger_->filesize();
    }

    // log operation
    virtual void log(CommitLogTx::const_ptr tx, fds_uint64_t index = 0);

    // retrieve operations
    // TODO(umesh): implement this

  private:
    fds_rwlock logLock_;
    fds_rwlock rotateLock_;

    const fds_volid_t volId_;
    ObjectLogger::ptr logger_;
};

struct OpJournalIndexEntry {
    fds_uint64_t id;
    off_t offset;
    fds_int32_t fileIndex;
    fds_bool_t deleted;

    // Methods
    OpJournalIndexEntry() : id(0), offset(0), fileIndex(-1), deleted(false) {}
};
}  /* namespace fds */

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_TVC_OPERATIONJOURNAL_H_
