/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_DM_TVC_OPERATIONJOURNAL_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_TVC_OPERATIONJOURNAL_H_

#include <string>

#include <fds_types.h>

namespace fds {

struct CommitLogTx;

/**
 * Journal of all write operations pushed to VolumeCatalog
 */
class DmTvcOperationJournal {
  public:
    typedef boost::shared_ptr<DmTvcOperationJournal> ptr;
    typedef boost::shared_ptr<const DmTvcOperationJournal> const_ptr;

    explicit DmTvcOperationJournal(fds_volid_t volId) : volId_(volId) {}
    virtual ~DmTvcOperationJournal() {}

    inline fds_volid_t volId() const {
        return volId_;
    }

    // log operation
    virtual void log(const CommitLogTx & tx);

    // retrieve operations
    // TODO(umesh): implement this

  private:
    const fds_volid_t volId_;
};
}  /* namespace fds */

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_TVC_OPERATIONJOURNAL_H_
