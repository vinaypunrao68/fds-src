/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_REQUEST_H_
#define SOURCE_INCLUDE_FDS_REQUEST_H_

#include <string>
#include <boost/thread/condition.hpp>
#include <cpplist.h>
#include <concurrency/Mutex.h>

namespace fdsio {

class Request;
class RequestIter
{
  public:
    RequestIter() {}
    virtual ~RequestIter() {}

    // Return true will terminate the iter loop.
    //
    virtual bool rq_iter_fn(Request *req) = 0;
};

//
// Generic Request Queue
// ---------------------
// Factor out common queue mechanism to keep track of request while it's
// waiting for response/completion from a software layer.
//
// The queue has number of queues to chain a request object.  Request can move
// among queues for specific operation associated with the queue or better
// tracking.
//
// Request object's life cycle inside the queue:
//    Thread 1              Thread i
//     )   ^                   )
//     (   |                   (
//  rq_enqueue                 )
//       :                     v
// rq_request_wait()    rq_move_queue()
//                      rq_dequeue()
//                      rq_request_wakeup()
// rq_request_done()
//
// Note that thread(i) can be the same context as thread 1, depending on how
// the queue was used inside a controller.
//
class Request;
class RequestQueue
{
  public:
    RequestQueue(int nr_queue, int max_depth);
    virtual ~RequestQueue();

    virtual void rq_enqueue(Request *rqt, int queue_idx, fds_uint32_t flag = 0);
    virtual void rq_enqueue_mtx(Request *rqt, int queue_idx, fds_uint32_t flag = 0);

    virtual void rq_detach(Request *rqt);
    virtual void rq_detach_mtx(Request *rqt);

    virtual void rq_move_queue(Request *rqt, int new_idx, fds_uint32_t flag = 0);
    virtual void rq_move_queue_mtx(Request *rqt, int new_idx, fds_uint32_t flag = 0);

    virtual void rq_iter_queue(RequestIter *it, int queue_idx);
    virtual void rq_iter_queue_mtx(RequestIter *it, int queue_idx);

    // \rq_request_wait
    // ----------------
    // Block the caller on the request until a rq_request_wakeup() is
    // called.  Depend on methods provided by the request to synchronize
    // the wait/wakeup.  Mutex and condition variable are provided by
    // the queue.
    //
    virtual void rq_request_wait(Request *rqt);

    // \rq_request_wakeup
    // ------------------
    // Wakeup the caller blocking on rq_request_wait().
    //
    virtual void rq_request_wakeup(Request *rqt);

    // \rq_request_done
    // ----------------
    // Called by the service layer to notify the queue when the given request
    // is done.
    //
    virtual void rq_request_done(Request *rqt);

    // \rq_dequeue
    // -----------
    // @param queue_idx : index to dequeue a request.
    // @param new_idx : the new index to chain the request.
    // @return: nullptr if the queue is empty.
    //
    virtual Request *rq_dequeue(int queue_idx, int new_idx);
    virtual Request *rq_dequeue_mtx(int queue_idx, int new_idx);

    // rq_peek
    // -------
    // @return FIFO element from the queue at the given index.
    //
    virtual Request *rq_peek_set_flag(int queue_idx, fds_uint32_t flag);
    virtual Request *rq_peek_set_flag_mtx(int queue_idx, fds_uint32_t flag);

    inline Request *rq_peek(int queue_idx) {
        return rq_peek_set_flag(queue_idx, 0);
    }
    inline Request *rq_peek_mtx(int queue_idx) {
        return rq_peek_set_flag_mtx(queue_idx, 0);
    }
    inline fds::fds_mutex *rq_get_mtx() {
        return &rq_mutex;
    }

  private:
    int                      rq_nr_queue;
    int                      rq_max_depth;
    int                      rq_pending;
    fds::ChainList           *rq_lists;
    fds::fds_mutex           rq_mutex;
};

//
// Generic Request Object
// ----------------------
// Factor out common problems to enqueue, dequeue, or track a request object.
// when it is submited to a SW layer.
//
//        Thread 1                     Thread i
//           (                            (
//           )          ^                 )
//      Create Object   |                 (
//           |          |            req_status()        req_abort()
//           V          |                 |                   |
//      Enqueue to a SW layer             |                   |
//          ...                           V                   V
//                                         call req_submit()
//                                         call req_complete()
//
// Note that thread(1) and thread(i) may be the same thread and context.
//
class Request
{
  public:
    explicit Request(bool block);
    virtual ~Request();

    static const fds_uint32_t req_state_wait = 0x20000000;
    static const fds_uint32_t req_state_done = 0x40000000;
    static const fds_uint32_t req_block      = 0x80000000;
    static const fds_uint32_t req_user_flag  = 0x0000ffff;

    // \req_abort
    // ----------
    // Called to abort the request after submitting it to the queue.
    //
    virtual void req_abort();

    // \req_submit
    // -----------
    // Called by the service SW layer to notify the owner when its request is
    // out of a queue and submitted to the processing layer.  It can be in the
    // same context of the calling thread if there's no queueing.
    //
    virtual void req_submit();

    // \req_wait
    // ---------
    // Block the caller until the request is done.
    //
    virtual void req_wait(bool set_block = false);

    // \req_complete
    // -------------
    // The default action to notify the request owner when the processing layer
    // completes the request.  It is the wrapper call to the queue's method to
    // wakeup calling thread.
    //
    virtual void req_complete();

    // \req_wait_cond
    // --------------
    // Used by queue's synchronization code to test for request's waiting
    // conditions.  This call is made while hoding RequestQueue's mutex,
    // it's important that the implementation must be simple and lock free.
    //
    virtual bool req_wait_cond();

    // \req_set_wakeup_cond
    // --------------------
    // Used by the default req_complete() call to set the wakeup condition
    // to wake up the owner's thread.  This call is also made while holding
    // Request Queue's mutex.
    //
    virtual void req_set_wakeup_cond();

    // Must be called under's queue mutex if it's linked to that queue.
    //
    void req_set_uflags(fds_uint32_t flags);

    // \req_blocking_mode
    // ------------------
    // Return true if the request is in the blocking mode.
    //
    inline bool req_blocking_mode() {
        return ((req_queue != nullptr) && (req_state & Request::req_block));
    }
    inline void req_set_blocking_mode() {
        req_state |= Request::req_block;
    }
    inline fds_uint32_t req_get_uflags() const {
        return (req_state & Request::req_user_flag);
    }
    // \req_owner_queue
    // ----------------
    //
    inline const RequestQueue *req_owner_queue() const {
        return req_queue;
    }

  private:
    friend class RequestQueue;
    fds::ChainLink           req_link;
    RequestQueue             *req_queue;
    boost::condition         *req_waitq;

  protected:
    fds_uint32_t             req_state;
};

}  // namespace fdsio

#endif  // SOURCE_INCLUDE_FDS_REQUEST_H_
