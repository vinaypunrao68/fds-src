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

    // log operation
    virtual void log(CommitLogTx::const_ptr tx);

    // retrieve operations
    // TODO(umesh): implement this

  private:
    const fds_volid_t volId_;
    ObjectLogger::ptr logger_;
};
}  /* namespace fds */

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_TVC_OPERATIONJOURNAL_H_
