/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#define DONTLOGLINE 1
#include <fds-fsm.h>
#include <fds_assert.h>
#include <concurrency/Mutex.h>
#include <util/Log.h>
#include <util/TraceBuffer.h>

namespace fds {

#if 0
#define DEBUG_FSM_ST(str, obj)    DBG(obj->st_trace(NULL) << ":" << str << '\n')
#define DEBUG_FSM(str, evt, obj)  DBG(obj->st_trace(evt) << ":" << str << '\n')
#endif
#define DEBUG_FSM_ST(str, obj)
#define DEBUG_FSM(str, evt, obj)

std::ostream &
operator << (std::ostream &os, const EventObj::pointer evt)
{
    os << " evt: " << evt.get() << ": " << evt->st_evt;
    evt->evt_name(os);
    return os;
}

std::ostream &
operator << (std::ostream &os, const StateObj::pointer st)
{
    st->st_owner_info(os);
    os << " st: " << st.get() << " [";
    os << st->st_curr_name() << "]";
    return os;
}

// req_complete
// ------------
void
EventObj::req_complete()
{
    fdsio::Request::req_complete();
}

// st_enter
// --------
//
void
StateEntry::st_enter(EventObj::pointer evt, StateObj::pointer cur) const
{
    if (evt != NULL) {
        DEBUG_FSM(" e-enter " << st_name() << ":", evt, cur);
    } else {
        DEBUG_FSM_ST(" enter " << st_name() << ":", cur);
    }
}

// st_exit
// -------
//
void
StateEntry::st_exit(EventObj::pointer evt, StateObj::pointer cur) const
{
    if (evt != NULL) {
        DEBUG_FSM(" e-exit " << st_name() << ":", evt, cur);
    } else {
        DEBUG_FSM_ST(" exit " << st_name() << ":", cur);
    }
}

// st_handle
// ---------
//
int
StateEntry::st_handle(EventObj::pointer evt, StateObj::pointer cur) const
{
    cur->st_verify();
    if (st_parent_idx != -1) {
        FsmTable::pointer tab = cur->st_fsm();
        return tab->st_entries[st_parent_idx]->st_handle(evt, cur);
    }
    /* We don't have any matching parent states that can process this event. */
    return StateEntry::st_no_change;
}

StateObj::StateObj(int v, FsmTable::pointer tab) : st_refcnt(0), st_idx(v), st_tab(tab)
{
    st_status   = 0;
    st_db_idx   = 0;
    st_cur_evt  = NULL;
    for (int i = 0; i < StateObj::st_max_hist; i++) {
        st_db_hist[i] = 0;
    }
}

StateObj::StateObj(int v, FsmTable::pointer tab, int ref)
    : st_refcnt(ref), st_idx(v), st_tab(tab)
{
    st_status   = 0;
    st_db_idx   = 0;
    st_cur_evt  = NULL;
    for (int i = 0; i < StateObj::st_max_hist; i++) {
        st_db_hist[i] = 0;
    }
}

// st_bind_evt
// -----------
//
void
StateObj::st_bind_evt(EventObj::pointer evt)
{
    if ((evt != NULL) && (evt != st_cur_evt)) {
        fds_assert(st_cur_evt == NULL);
        fds_assert(evt->st_owner == NULL);
        fds_verify(st_status & StateObj::st_busy);

        evt->evt_bind_owner(this);
        st_cur_evt = evt;
    }
}

// st_mk_next_evt
// --------------
// Default method to return event to submit to the next state.
//
EventObj::pointer
StateObj::st_mk_next_evt(EventObj::pointer cur, int next_st)
{
    return NULL;
}

// st_verify
// ---------
//
void
StateObj::st_verify(bool locked) const
{
}

// st_change_mtx
// -------------
//
void
StateObj::st_change_mtx(int new_st)
{
    fds_verify(new_st < st_tab->st_cnt);
    fds_verify(st_status & StateObj::st_locked);

    st_db_hist[st_db_idx] = st_idx & 0xff;
    st_db_idx = (st_db_idx + 1) & StateObj::st_max_hist_mask;
    st_idx    = new_st;
}

// st_clone_defer_evt
// ------------------
//
EventObj::pointer
StateObj::st_clone_defer_evt(EventObj::pointer evt) const
{
    return new EventObj(evt->evt_current());
}

fds_mutex  FsmTable::st_obj_mtx[FsmTable::st_mtx_arr];

FsmTable::~FsmTable()
{
    delete st_queues;
}

FsmTable::FsmTable(int cnt, StateEntry const *const *const e)
    : st_refcnt(0), st_cnt(cnt), st_entries(e)
{
    st_queues = new fdsio::RequestQueue(cnt + 1, -1);
}

// st_input
// --------
//
void
FsmTable::st_input(EventObj::pointer evt, StateObj::pointer obj)
{
    DEBUG_FSM(" input sync ", evt, obj);
    /*
     * If this assertion is true, the event was recycled by sub-states while the main
     * state requested a blocking call.
     */
    fds_verify(evt->req_owner_queue() == NULL);
    evt->req_set_blocking_mode();
    st_in_async_priv(evt, obj);
    evt->req_wait();

    DEBUG_FSM(" input sync done ", evt, obj);
}

// st_in_async
// -----------
//
void
FsmTable::st_in_async(EventObj::pointer evt, StateObj::pointer obj)
{
    if (evt != NULL) {
        DEBUG_FSM(" input async ", evt, obj);
        st_in_async_priv(evt, obj);
    }
}

// st_push_defer_evt
// -----------------
//
void
FsmTable::st_push_defer_evt(EventObj::pointer evt,
                            StateObj::pointer obj, int state, int tick)
{
}

// st_process_defer_evt
// --------------------
//
void
FsmTable::st_process_defer_evt(StateObj::pointer obj)
{
}

// st_in_async_priv
// ----------------
//
void
FsmTable::st_in_async_priv(EventObj::pointer evt, StateObj::pointer obj)
{
    fds_mutex         *mtx;
    EventObj::pointer  sav;

    sav = evt;
    mtx = obj->st_mtx();
    mtx->lock();
    if (evt == NULL) {
        evt = st_get_defer_event_mtx(obj);
        if (evt != NULL) {
            evt->obj_dec_refcnt();
        }
    }
    int loop = 0;
    while ((evt != NULL) && (st_in_async_mtx(evt, obj) == true)) {
        evt = st_get_defer_event_mtx(obj);
        DBG(
        if (evt != NULL) {
            evt->obj_dec_refcnt();
            DEBUG_FSM(" process pending ", evt, obj);
        });
        if (loop++ > 1000) {
            obj->st_trace(evt) << "bullshit happened, need to double check." << '\n';
        }
    }
    mtx->unlock();
}

// st_in_async_mtx
// ---------------
//
bool
FsmTable::st_in_async_mtx(EventObj::pointer evt, StateObj::pointer obj)
{
    bool       ret;
    int        nxt_st, cur_st;
    fds_mutex *mtx;

    // fds_assert((evt->st_owner == obj) || (evt->st_owner == NULL));
    ret = true;
    mtx = obj->st_mtx();

    cur_st = obj->st_idx;
    fds_assert(cur_st < st_cnt);
    if (obj->st_status == StateObj::st_no_op) {
        obj->st_set_lock_busy_mtx();
        obj->st_bind_evt_mtx(evt);
        mtx->unlock();
        if (evt->req_owner_queue() == NULL) {
            st_queues->rq_enqueue(evt.get(), cur_st);
        }
        /* Do state transition and carry out actions. */
        for (auto loop = 0; evt != NULL; loop++) {
            fds_verify((loop < 1000) &&
                       ((obj->st_status & StateObj::st_busy) != 0));

            cur_st = obj->st_idx;
            nxt_st = st_entries[cur_st]->st_handle(evt, obj);
            if (nxt_st == StateEntry::st_busy_no_trans) {
                /* Can't assume anything about evt or obj's state. */
                DEBUG_FSM(" obj non-block busy ", evt, obj);
                ret = false;
                break;
            }
            evt = st_do_resume(obj, evt, nxt_st);
        }
        mtx->lock();
    } else {
        st_defer_event_mtx(evt, obj);
        ret = false;
    }
    if (ret == true) {
        fds_verify(obj->st_status & StateObj::st_locked);
    }
    return ret;
}

// st_do_resume
// ------------
//
EventObj::pointer
FsmTable::st_do_resume(StateObj::pointer obj, EventObj::pointer evt, int nxt_st)
{
    int                cur_st;
    fds_mutex         *mtx;
    EventObj::pointer  nxt_evt;

    /* The event may be chained to different queue. */
    if (nxt_st == StateEntry::st_busy_no_trans) {
        return NULL;
    }
    mtx    = obj->st_mtx();
    cur_st = obj->st_idx;

    if ((nxt_st >= 0) && (nxt_st != cur_st) && (nxt_st != StateEntry::st_no_change)) {
        fds_verify(nxt_st < st_cnt);
        st_entries[cur_st]->st_exit(evt, obj);

        nxt_evt = obj->st_mk_next_evt(evt, nxt_st);
        mtx->lock();
        obj->st_change_mtx(nxt_st);

        if ((nxt_evt == NULL) || (nxt_evt != evt)) {
            /* The current event is considered done. */
            obj->st_unbind_evt_mtx();
            if (nxt_evt == NULL) {
                obj->st_clr_busy_mtx();  /* important: still in locked state. */
            } else {
                fds_verify(nxt_evt->req_owner_queue() == NULL);
                st_queues->rq_enqueue(nxt_evt.get(), nxt_st);
            }
        } else if ((evt != NULL) && (evt->req_owner_queue() == st_queues)) {
            st_queues->rq_move_queue(evt.get(), nxt_st);
        }
        mtx->unlock();

        obj->st_verify(true);
        if (evt != NULL && nxt_evt != evt) {
            /* The current event is considered done. */
            DEBUG_FSM(" complete old evt ", evt, obj);
            if (evt->req_owner_queue() == st_queues) {
                st_queues->rq_detach(evt.get());
            }
            evt->req_complete();
        }
        evt = nxt_evt;
        st_entries[nxt_st]->st_enter(evt, obj);
    } else {
        /* No state transition, we're done.  Clean up the state obj. */
        mtx->lock();
        obj->st_clr_busy_mtx();          /* important: still in locked state. */

        if ((evt != NULL) && (evt->st_owner == obj)) {
            obj->st_unbind_evt_mtx();
        }
        mtx->unlock();

        if (evt != NULL) {
            DEBUG_FSM(" complete notif ", evt, obj);
            evt->req_complete();
            evt = NULL;
        }
    }
    if (nxt_st == StateEntry::st_defer_evt) {
        fds_assert(evt == NULL);
    }
    if (nxt_st == StateEntry::st_handle_defer_evt) {
        fds_assert(evt == NULL);
    }
    return evt;
}

// st_resume
// ---------
//
void
FsmTable::st_resume(StateObj::pointer obj, int nxt_st)
{
    EventObj::pointer  evt;

    fds_assert((nxt_st >= 0) && (nxt_st < st_cnt));
    if ((obj->st_status & StateObj::st_busy) == 0) {
        return;
    }
    fds_assert(obj->st_status & StateObj::st_locked);
    evt = st_do_resume(obj, obj->st_cur_evt, nxt_st);
    st_in_async_priv(evt, obj);
}

// st_verify_wiring
// ----------------
//
void
FsmTable::st_verify_wiring()
{
    for (int i = 0; i < st_cnt; i++) {
        StateEntry const *const st = st_entries[i];
        fds_verify(st->st_my_idx == i);
        fds_verify((st->st_parent_idx == -1) ||
                   ((st->st_parent_idx < st_cnt) && (st->st_parent_idx != i)));
    }
}

// st_unbind_event
// ---------------
//
EventObj::pointer
FsmTable::st_unbind_event(StateObj::pointer obj)
{
    fds_mutex         *mtx;
    EventObj::pointer  evt;

    mtx = obj->st_mtx();

    mtx->lock();
    evt = obj->st_cur_evt;
    fds_verify(evt != NULL);

    obj->st_unbind_evt_mtx();
    mtx->unlock();

    return evt;
}

StateObj::pointer
FsmTable::st_unbind_event(StateObj::pointer me, EventObj::pointer evt)
{
    fds_mutex         *mtx;
    StateObj::pointer  obj;

    obj = evt->st_owner;
    if ((obj == NULL) || (obj != me)) {
        /* The event already detached or I'm not the current owner. */
        return (NULL);
    }
    mtx = obj->st_mtx();
    mtx->lock();
    obj->st_unbind_evt_mtx();
    mtx->unlock();

    return obj;
}

// st_defer_event_mtx
// ------------------
//
void
FsmTable::st_defer_event_mtx(EventObj::pointer evt, StateObj::pointer obj)
{
    DEBUG_FSM(" defer ", evt, obj);

    /* Use the last st_cnt as the pending request queue. */
    fds_verify(evt->st_owner == NULL);
    fds_verify(evt->req_owner_queue() == NULL);
    fds_verify(obj->st_status & StateObj::st_locked);

    evt->obj_inc_refcnt();
    evt->evt_bind_owner(obj);
    st_queues->rq_enqueue(evt.get(), st_cnt);
}

class EventObjIter : public fdsio::RequestIter
{
  public:
    virtual ~EventObjIter() {}
    explicit EventObjIter(StateObj::pointer obj) : st_obj(obj), st_evt(NULL) {}

    bool rq_iter_fn(fdsio::Request *req) override
    {
        EventObj::pointer evt = evt_cast_ptr<EventObj>(req);
        fds_verify(evt->st_owner != NULL);
        if (evt->st_owner == st_obj) {
            st_evt = evt;
            return true;
        }
        return false;
    }
    StateObj::pointer        st_obj;
    EventObj::pointer        st_evt;
};

// st_get_defer_event_mtx
// ----------------------
//
EventObj::pointer
FsmTable::st_get_defer_event_mtx(StateObj::pointer obj)
{
    EventObjIter iter(obj);

    st_queues->rq_iter_queue(&iter, st_cnt);

    /* We only clear the lock bit here under the lock. */
    obj->st_unlock_obj_mtx();
    if (iter.st_evt != NULL) {
        DEBUG_FSM(" process defer ", iter.st_evt, obj);
    }
    return iter.st_evt;
}

// st_trace
// --------
//
std::stringstream &
FsmTable::st_trace(StateObj::pointer sobj, EventObj::pointer evt)
{
    TraceBuffer *trace;

    st_tab_mtx.lock();
    try {
        trace = st_traces.at(sobj.get());
    } catch(...) {
        trace = new TraceBuffer(FsmTable::st_trace_buf_sz);
        st_traces[sobj.get()] = trace;
    }
    st_tab_mtx.unlock();
    trace->tr_buffer() << sobj;
    if (evt != NULL) {
        trace->tr_buffer() << '[' << evt << "] ";
    }
    return trace->tr_buffer();
}

// st_dump_state_trans
// -------------------
//
void
FsmTable::st_dump_state_trans()
{
    TraceBuffer *trace;

    st_tab_mtx.lock();
    for (auto it = st_traces.begin(); it != st_traces.end(); it++) {
        trace = it->second;
        LOGDEBUG << '\n' << trace->str();
        trace->tr_buffer().clear();
    }
    st_tab_mtx.unlock();
}

}  // namespace fds
