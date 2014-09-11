/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <fds_assert.h>
#include <fds_request.h>

namespace fdsio {

// \RequestQueue
// -------------
//
RequestQueue::RequestQueue(int nr_queue, int max_depth)
    : rq_nr_queue(nr_queue), rq_max_depth(max_depth), rq_mutex("req queue")
{
    rq_lists = new fds::ChainList [nr_queue];
}

// \~RequestQueue
// --------------
RequestQueue::~RequestQueue()
{
    delete [] rq_lists;
}

// \rq_enqueue
// -----------
//
void
RequestQueue::rq_enqueue(Request *rqt, int queue_idx, fds_uint32_t uflag)
{
    rq_mutex.lock();
    rq_enqueue_mtx(rqt, queue_idx, uflag);
    rq_mutex.unlock();
}

void
RequestQueue::rq_enqueue_mtx(Request *rqt, int queue_idx, fds_uint32_t uflag)
{
    fds_assert(queue_idx < rq_nr_queue);
    fds_verify(rqt->req_link.chain_empty());

    if (rqt->req_queue != nullptr) {
        fds_verify(rqt->req_queue == this);
        rqt->req_link.chain_rm();
    } else {
        rqt->req_queue = this;
        rq_pending++;
    }
    rqt->req_state |= (uflag & Request::req_user_flag);
    rq_lists[queue_idx].chain_add_back(&rqt->req_link);
}

// \rq_detach
// ----------
//
void
RequestQueue::rq_detach(Request *rqt)
{
    rq_mutex.lock();
    rq_detach_mtx(rqt);
    rq_mutex.unlock();
}

void
RequestQueue::rq_detach_mtx(Request *rqt)
{
    fds_verify(rqt->req_queue == this);
    rqt->req_queue = nullptr;
    rqt->req_link.chain_rm();
    rq_pending--;
}

// \rq_move_queue
// --------------
//
void
RequestQueue::rq_move_queue(Request *rqt, int new_idx, fds_uint32_t uflag)
{
    rq_mutex.lock();
    rq_move_queue_mtx(rqt, new_idx, uflag);
    rq_mutex.unlock();
}

void
RequestQueue::rq_move_queue_mtx(Request *rqt, int new_idx, fds_uint32_t uflag)
{
    fds_assert(new_idx < rq_nr_queue);
    fds_verify(rqt->req_queue == this);

    rqt->req_link.chain_rm();
    rqt->req_state &= ~Request::req_user_flag;
    rqt->req_state |= (uflag & Request::req_user_flag);
    rq_lists[new_idx].chain_add_back(&rqt->req_link);
}

// rq_iter_queue
// -------------
//
void
RequestQueue::rq_iter_queue(RequestIter *it, int idx)
{
    rq_mutex.lock();
    rq_iter_queue_mtx(it, idx);
    rq_mutex.unlock();
}

// rq_iter_queue_mtx
// -----------------
//
void
RequestQueue::rq_iter_queue_mtx(RequestIter *it, int idx)
{
    fds::ChainIter iter;

    chain_foreach(&rq_lists[idx], iter) {
        if (it->rq_iter_fn(rq_lists[idx].chain_iter_current<Request>(iter)) == true) {
            break;
        }
    }
}

// \rq_request_wait
// ----------------
//
void
RequestQueue::rq_request_wait(Request *rqt)
{
    boost::condition waitq;

    fds_verify(rqt->req_queue == this);
    fds_verify(rqt->req_waitq == nullptr);

    rqt->req_waitq = &waitq;
    rq_mutex.lock();
    while (rqt->req_wait_cond() == false) {
        waitq.wait(rq_mutex);
    }
    rq_mutex.unlock();

    fds_verify(rqt->req_waitq == &waitq);
    rqt->req_waitq = nullptr;
}

// \rq_request_wakeup
// ------------------
//
void
RequestQueue::rq_request_wakeup(Request *rqt)
{
    boost::condition *waitq;

    rq_mutex.lock();
    rqt->req_set_wakeup_cond();

    waitq = rqt->req_waitq;
    if (waitq != nullptr) {
        waitq->notify_one();
        fds_verify(rqt->req_queue == this);
    }
    rq_pending--;
    rqt->req_queue = nullptr;
    rqt->req_link.chain_rm_init();
    rq_mutex.unlock();
}

// \rq_request_done
// ----------------
//
void
RequestQueue::rq_request_done(Request *rqt)
{
    fds_verify(rqt->req_queue == this);

    rq_mutex.lock();
    rq_pending--;
    rqt->req_link.chain_rm_init();
    rqt->req_queue = nullptr;
    rq_mutex.unlock();
}

// \rq_dequeue
// -----------
//
Request *
RequestQueue::rq_dequeue(int queue_idx, int new_idx)
{
    Request *rqt;

    rq_mutex.lock();
    rqt = rq_dequeue_mtx(queue_idx, new_idx);
    rq_mutex.unlock();

    return rqt;
}

Request *
RequestQueue::rq_dequeue_mtx(int queue_idx, int new_idx)
{
    Request *rqt;

    fds_assert((queue_idx < rq_nr_queue) && (new_idx < rq_nr_queue));
    rqt = rq_lists[queue_idx].chain_rm_front<Request>();
    if (rqt != nullptr) {
        fds_verify(rqt->req_queue == this);
        rq_lists[new_idx].chain_add_back(&rqt->req_link);
    }
    return rqt;
}

// rq_peek_set_flag
// ----------------
//
Request *
RequestQueue::rq_peek_set_flag(int queue_idx, fds_uint32_t flag)
{
    Request *rqt;

    rq_mutex.lock();
    rqt = rq_peek_set_flag_mtx(queue_idx, flag);
    rq_mutex.unlock();

    return rqt;
}

Request *
RequestQueue::rq_peek_set_flag_mtx(int queue_idx, fds_uint32_t flag)
{
    Request *rqt;

    fds_assert(queue_idx < rq_nr_queue);
    rqt = rq_lists[queue_idx].chain_peek_front<Request>();
    if (rqt != NULL) {
        rqt->req_state |= (flag & Request::req_user_flag);
    }
    return rqt;
}

// \Request
// --------
//
Request::Request(bool block)
    : req_link(this), req_queue(nullptr), req_waitq(nullptr), req_state(0)
{
    if (block) {
        req_state |= Request::req_block;
    }
}

// ~Request
// --------
//
Request::~Request()
{
}

// req_abort
// ---------
//
void
Request::req_abort()
{
}

// req_submit
// ----------
//
void
Request::req_submit()
{
}

// req_wait
// --------
//
void
Request::req_wait(bool set_block)
{
    if (set_block == true) {
        req_state |= Request::req_block;
    }
    // Only block if the request was created with blocking mode.
    if (req_blocking_mode()) {
        req_queue->rq_request_wait(this);
    }
}

// req_complete
// ------------
//
void
Request::req_complete()
{
    if (req_queue != nullptr) {
        if (req_state & Request::req_block) {
            req_queue->rq_request_wakeup(this);
        } else {
            req_queue->rq_request_done(this);
        }
    }
}

// req_wait_cond
// -------------
// This call is made while holding queue's mutex.
//
bool
Request::req_wait_cond()
{
    return (req_state & req_state_done) != 0;
}

// req_set_wakeup_cond
// -------------------
// This call is made while holding queue's mutex.
//
void
Request::req_set_wakeup_cond()
{
    req_state |= req_state_done;
}

// req_set_uflags
// --------------
//
void
Request::req_set_uflags(fds_uint32_t flags)
{
    if (req_queue != NULL) {
        /* Assert the lock here! */
        req_state |= (flags & Request::req_user_flag);
    }
}

}  // namespace fdsio
