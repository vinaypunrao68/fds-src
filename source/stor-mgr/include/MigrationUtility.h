/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_MIGRATIONUTILITY_H_
#define SOURCE_STOR_MGR_INCLUDE_MIGRATIONUTILITY_H_

#include <mutex>
#include <set>
#include <iostream>

namespace fds {

class MigrationSeqNum {
  public:
    typedef std::shared_ptr<MigrationSeqNum> ptr;

    MigrationSeqNum();
    ~MigrationSeqNum();

    /* interface to set the sequence number.
     */
    bool setSeqNum(uint64_t _seqNum, bool _lastSeqNum);

    /* Simple boolean to determine if the sequence is complete or not.
     */
    bool isSeqNumComplete();

    void resetSeqNum();

    /* ostream */
    friend
    std::ostream& operator<< (std::ostream& out,
                              const MigrationSeqNum& seqNumRecv);

    /* to log file */
    friend
    boost::log::formatting_ostream& operator<< (boost::log::formatting_ostream& out,
                                                const MigrationSeqNum& seqNumRecv);

  private:
    /**
     * update to sequence and previously handled sequence numbers.
     */
    std::mutex seqNumLock;

    /**
     * current sequence number.
     */
    uint64_t curSeqNum;

    /**
     * last sequence number.  Only set if the sequence is set
     * with the "last" flag set.
     */
    uint64_t lastSeqNum;

    /**
     * Simple boolean to determine if the sequence is complete or not.
     */
    bool seqComplete;

    /**
     * list of sequence numbers so far has been received in out of order.
     */
    std::set<uint64_t> seqNumList;
};  // class MigrationSeqNum


class MigrationDoubleSeqNum {
  public:
    MigrationDoubleSeqNum();
    ~MigrationDoubleSeqNum();

    /* interface to set the sequence number.
     */
    bool setDoubleSeqNum(uint64_t seqNum1, bool isLastSeqNum1,
                         uint64_t seqNum2, bool isLastSeqNum2);

    /* Simple boolean to determine if the sequence is complete or not.
     */
    bool isDoubleSeqNumComplete();

    void resetDoubleSeqNum();

    /* ostream */
    friend
    std::ostream& operator<< (std::ostream& out,
                              const MigrationDoubleSeqNum& seqNumRecv);

    /* to log file */
    friend
    boost::log::formatting_ostream& operator<< (boost::log::formatting_ostream& out,
                                                const MigrationDoubleSeqNum& seqNumRecv);

  private:
    std::mutex seqNumLock;
    uint64_t curSeqNum1;
    uint64_t lastSeqNum1;
    bool completeSeqNum1;
    std::map<uint64_t, MigrationSeqNum::ptr> mapSeqNum2;

    // std::map<uint64_t, std::set<uint64_t>> seqNumMap;
};  // MigrationSeqNumChoppped

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_MIGRATIONUTILITY_H_
