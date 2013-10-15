#include <fds_request.h>

namespace fdsio {

// \RequestQueue
// -------------
//
RequestQueue::RequestQueue(int nr_queue, int max_depth)
{
}

// \~RequestQueue
// --------------
RequestQueue::~RequestQueue()
{
}

// \rq_enqueue
// -----------
//
void
RequestQueue::rq_enqueue(Request *rqt, int queue_idx)
{
}

// \rq_move_queue
// --------------
//
void
RequestQueue::rq_move_queue(Request *rqt, int new_idx)
{
}

// \rq_request_wait
// ----------------
//
void
RequestQueue::rq_request_wait(Request *rqt)
{
}

// \rq_request_wakeup
// ------------------
//
void
RequestQueue::rq_request_wakeup(Request *rqt)
{
}

// \rq_request_done
// ----------------
//
void
RequestQueue::rq_request_done(Request *rqt)
{
}

// \rq_dequeue
// -----------
//
Request *
RequestQueue::rq_dequeue(int queue_idx, int new_idx)
{
    return nullptr;
}

// \RequestStatus
// --------------
//
RequestStatus::RequestStatus()
{
}

// ~RequestStatus
// --------------
//
RequestStatus::~RequestStatus()
{
}

// \Request
// --------
//
Request::Request(const RequestQueue &queue, bool block)
    : req_link(this), req_queue(queue), req_state(0), req_res()
{
    if (block) {
        req_state |= req_block;
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
Request::req_wait()
{
}

// req_complete
// ------------
//
void
Request::req_complete()
{
}

// req_wait_cond
// -------------
//
bool
Request::req_wait_cond()
{
    return (req_state & req_state_done) != 0;
}

// req_set_wakeup_cond
// -------------------
//
void
Request::req_set_wakeup_cond()
{
    req_state |= req_state_done;
}

// req_status
// ----------
//
void
Request::req_status(RequestStatus *status)
{
}

} // namespace fdsio
