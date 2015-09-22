/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_MIGRATIONUTILITY_H_
#define SOURCE_INCLUDE_MIGRATIONUTILITY_H_

#include <mutex>
#include <chrono>
#include <condition_variable>
#include <set>
#include <iostream>

#include <fds_timer.h>

namespace fds {

class MigrationSeqNum {
  public:
    typedef std::shared_ptr<MigrationSeqNum> ptr;

    MigrationSeqNum();

    /* Constructor with progress checker (timeout interval)
     */
    MigrationSeqNum(FdsTimerPtr& timer,
                    uint32_t interval,  // in sec
                    const std::function<void()> &func);

    ~MigrationSeqNum();

    /* interface to set the sequence number.
     */
    bool setSeqNum(uint64_t _seqNum, bool _lastSeqNum);

    /* interface to get current sequence number.
     */
    uint64_t getSeqNum();

    /* Simple boolean to determine if the sequence is complete or not.
     */
    bool isSeqNumComplete();

    /* Reset the sequence number to start from 0.
     */
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

    /**
     * Frequency to check if the sequence number is progressing forward.
     * If the sequence number has not moved during this interval, then
     * call the timeout handler registered with this sequence checker.
     */
    uint32_t seqNumTimerInterval;  // in secs

    /**
     * Determine if the timer is enabled or not.
     */
    bool seqNumTimerEnabled;

    /**
     * Determine if the timer has started or not.
     */
    bool seqNumTimerStarted;

    /**
     * Timer object used to schedule progress checker.  Passed in
     * as an arg to constructor.
     */
    FdsTimerPtr seqNumTimer;

    /**
     * Task object for the progress checker.  Allocated when the
     * start progress check is requested.
     */
    FdsTimerTaskPtr seqNumTimerTask;

    /**
     * monotonic time to record last time the sequence number of set/updated
     */
    std::chrono::steady_clock::time_point lastSetTime;

    /**
     * If the progress is halted, then invoke the timeout handler.
     */
    std::function<void()> seqNumTimeoutHandler;

    /**
     * Internal routine to check the progress.  If the progress is not made
     * during the interval, then the seqNumTimeoutHandler is called.
     */
    void checkProgress();

    /* Start the progress check on the sequence
     */
    bool startProgressCheck(bool isLastNum);

    /* Stop the progress check on the sequence.
     */
    bool stopProgressCheck(bool isLastNum);

};  // class MigrationSeqNum


class MigrationDoubleSeqNum {
  public:
    typedef std::shared_ptr<MigrationDoubleSeqNum> ptr;

    MigrationDoubleSeqNum();
    MigrationDoubleSeqNum(FdsTimerPtr& timer,
                          uint32_t interval,  // in sec
                          const std::function<void()> &func);
    ~MigrationDoubleSeqNum();

    /* interface to set the sequence number.
     */
    bool setDoubleSeqNum(uint64_t seqNum1, bool isLastSeqNum1,
                         uint64_t seqNum2, bool isLastSeqNum2);

    /* Simple boolean to determine if the sequence is complete or not.
     */
    bool isDoubleSeqNumComplete();

    /* Reset the double sequence.
     */
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

    /** Progress check.
     */
    // Interval frequency to check for progress.
    uint32_t seqNumTimerInterval;  // in secs

    // Determine if the timer has started or not.
    bool seqNumTimerEnabled;

    // Determine if the timer has started or not.
    bool seqNumTimerStarted;

    // Timer object.  Passed into the constructor.
    FdsTimerPtr seqNumTimer;

    // TimeTask.  Created when the progress is tracked.
    FdsTimerTaskPtr seqNumTimerTask;

    // monotonic time to record last time the sequence number of set/updated
    std::chrono::steady_clock::time_point lastSetTime;

    // timeout routine, if the progress is not made during the specified interval.
    std::function<void()> seqNumTimeoutHandler;

    // internal routine to check the progress.  invoked at specified interval.
    void checkProgress();

    // Start the progress check on the sequence
    bool startProgressCheck(bool isNum1Last, bool isNum2Last);

    // Stop the progress check on the sequence.
    bool stopProgressCheck(bool isNum1Last, bool isNum2Last);

};  // MigrationSeqNumChoppped


class MigrationTrackIOReqs {
  public:
    MigrationTrackIOReqs();
    ~MigrationTrackIOReqs();


    bool startTrackIOReqs();
    void finishTrackIOReqs();

    void waitForTrackIOReqs();
    inline uint64_t debugCount() {
    	return numTrackIOReqs;
    }

    class ScopedTrackIOReqs : boost::noncopyable {
      public:
        MigrationTrackIOReqs * scopedReq;
        bool success = {false};
        explicit ScopedTrackIOReqs(MigrationTrackIOReqs &req)
                                  : scopedReq(&req) {
          if (scopedReq) {
            success = scopedReq->startTrackIOReqs();
          }
        }

        ~ScopedTrackIOReqs() {
          if (scopedReq && success) {
            scopedReq->finishTrackIOReqs();
            scopedReq = nullptr;
          }
        }
    };

  private:
    std::mutex  trackReqsMutex;
    std::condition_variable trackReqsCondVar;
    uint64_t numTrackIOReqs;
    bool waitingTrackIOReqsCompletion;
    bool denyTrackIOReqs;

};  // MigrationTrackIOReqs
}  // namespace fds

#endif  // SOURCE_INCLUDE_MIGRATIONUTILITY_H_
