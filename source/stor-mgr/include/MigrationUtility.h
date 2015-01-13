/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_MIGRATIONUTILITY_H_
#define SOURCE_STOR_MGR_INCLUDE_MIGRATIONUTILITY_H_

#include <mutex>
#include <set>
#include <iostream>

namespace fds {

class MigrationSeqNumReceiver {
  public:
    MigrationSeqNumReceiver();
    ~MigrationSeqNumReceiver();

    bool
    setSeqNum(uint64_t _seqNum, bool _lastSeqNum);

    friend
    std::ostream& operator<< (std::ostream& out,
                              const MigrationSeqNumReceiver& seqNumRecv);

    friend
    boost::log::formatting_ostream& operator<< (boost::log::formatting_ostream& out,
                                                const MigrationSeqNumReceiver& seqNumRecv);

  private:
    /**
     * update to sequence and previously handled sequence numbers.
     */
    std::mutex seqNumLock;

    /**
     * current sequence number.
     */
    uint64_t seqNum;

    /**
     * last sequence number.  Only set if the sequence is set
     * with the "last" flag set.
     */
    uint64_t lastSeqNum;

    /**
     * list of sequence numbers so far has been received.
     */
    std::set<uint64_t> seqNumList;
};  // class MigrationSeqNum


}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_MIGRATIONUTILITY_H_
