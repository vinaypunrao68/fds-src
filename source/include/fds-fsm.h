/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_FSM_H_
#define SOURCE_INCLUDE_FDS_FSM_H_

#include <sstream>
#include <string>
#include <unordered_map>
#include <fds_ptr.h>
#include <fds_assert.h>
#include <fds_request.h>
#include <shared/fds_types.h>

namespace fds {

class fds_mutex;
class StateObj;
class StateEntry;
class FsmTable;
class EventObjIter;
class NodeAgent;
class TraceBuffer;
class OmDomainEvt;

/**
 * Event object to send request to a state object.
 */
class EventObj : public fdsio::Request
{
  public:
    typedef boost::intrusive_ptr<EventObj> pointer;
    typedef boost::intrusive_ptr<const EventObj> const_ptr;

    virtual ~EventObj() {}
    explicit EventObj(int val)
        : st_refcnt(0), Request(false), st_evt(val), st_owner(NULL) {}

    inline  int  evt_current() const { return st_evt; }
    inline  void evt_change_code(int evt) { st_evt = evt; }

    virtual void evt_name(std::ostream &os) const { os << typeid(*this).name(); }
    virtual void req_complete() override;

    INTRUSIVE_PTR_OPS(st_refcnt);
    friend std::ostream &operator << (std::ostream &os, const EventObj::pointer evt);

  protected:
    int                             st_evt;

  private:
    friend class StateObj;
    friend class FsmTable;
    friend class StateEntry;
    friend class EventObjIter;

    boost::intrusive_ptr<StateObj>  st_owner;
    INTRUSIVE_PTR_DEFS(EventObj, st_refcnt);

    inline void evt_bind_owner(boost::intrusive_ptr<StateObj> owner) {
        st_owner = owner;
    }
    inline void evt_swap_owner(boost::intrusive_ptr<StateObj> *owner)
    {
        auto tmp = *owner;
        *owner   = st_owner;
        st_owner = tmp;
    }
};

/**
 * Cast different types of the inheritant tree to the desired type.
 */
template <class T>
static inline T *evt_cast_ptr(EventObj::pointer evt)
{
    return static_cast<T *>(get_pointer(evt));
}

template <class T>
static inline T *evt_cast_ptr(fdsio::Request *req)
{
    return reinterpret_cast<T *>(req);
}

/**
 * Encode state transition entry operated as RO const object.
 */
class StateEntry
{
  public:
    /* Return value from st_handle not to change the state object. */
    static const int st_no_change         = -1;
    static const int st_busy_no_trans     = -2;
    static const int st_defer_evt         = -3;
    static const int st_handle_defer_evt  = -4;

    virtual ~StateEntry() {}
    StateEntry(int my_idx, int parent) : st_my_idx(my_idx), st_parent_idx(parent) {}

    virtual char const *const st_name() const = 0;

    /**
     * Do any action/log before entering/leaving the state.
     */
    virtual void
    st_enter(EventObj::pointer evt, boost::intrusive_ptr<StateObj> cur) const;

    virtual void
    st_exit(EventObj::pointer evt, boost::intrusive_ptr<StateObj> cur) const;

    /**
     * Decide the next state and carry out any action needed for the given input.
     * @return:
     *   1) Next state to transit.
     *   2) st_no_change : no state transition.
     *   3) st_busy_no_trans : the action requires more time to complete before we
     *      can change to the new state.  New events will be in the pending queue.
     *      When the action is done, it needs to call obj->st_resume() method to
     *      resume the next state transition and process any defer events.
     */
    virtual int
    st_handle(EventObj::pointer evt, boost::intrusive_ptr<StateObj> cur) const;

  protected:
    friend class FsmTable;

    int                      st_my_idx;
    int                      st_parent_idx;
};

typedef std::unordered_map<StateObj *, TraceBuffer *> StateDebugMap;

/**
 * Static switch board.
 */
typedef struct state_switch state_switch_t;
struct state_switch
{
    int                      st_cur_state;
    int                      st_evt_code;
    int                      st_nxt_state;
};

/**
 * The state machine table.
 */
class FsmTable
{
  public:
    typedef boost::intrusive_ptr<FsmTable> pointer;

    virtual ~FsmTable();
    FsmTable(int cnt, StateEntry const *const *const e);

    /**
     * Process events, sync and async inputs.
     */
    void st_input(EventObj::pointer evt, boost::intrusive_ptr<StateObj> obj);
    void st_in_async(EventObj::pointer evt, boost::intrusive_ptr<StateObj> obj);
    void st_push_defer_evt(EventObj::pointer evt,
                           boost::intrusive_ptr<StateObj> obj, int state, int tick);
    void st_process_defer_evt(boost::intrusive_ptr<StateObj> obj);

    EventObj::pointer st_unbind_event(boost::intrusive_ptr<StateObj> obj);

    boost::intrusive_ptr<StateObj>
    st_unbind_event(boost::intrusive_ptr<StateObj> obj, EventObj::pointer evt);

    /**
     * There are two ways to process input:
     * 1) Apply the input to the current state, invoke the st_handler function, switch
     *    to the next state.
     * 2) Determine the next state based on current state & input, switch to the next
     *    state and invoke the st_handler function.  Give the state machine this
     *    table to follow option (2).
     */
    inline void st_set_switch_board(const state_switch_t *disp, int cnt) {
        st_switch     = disp;
        st_switch_cnt = cnt;
    }

    /**
     * Return mutex hashed by a pointer object.
     * Lock order: this lock -> st_queues's lock.
     */
    inline static fds_mutex *st_get_obj_mtx(const void *ptr) {
        auto v = reinterpret_cast<fds_uint64_t>(ptr) >> 8;
        return &st_obj_mtx[v % st_mtx_arr];
    }
    /**
     * Dump the history of state transitions of all states using the table.
     */
    void st_dump_state_trans(std::stringstream *st);
    std::stringstream &st_trace(boost::intrusive_ptr<StateObj>, EventObj::pointer);

  protected:
    friend class StateObj;
    friend class StateEntry;
    static const int                st_mtx_arr = 0x8;
    static const int                st_trace_buf_sz = 1024;
    static fds_mutex                st_obj_mtx[st_mtx_arr];

    int                             st_cnt;
    StateDebugMap                   st_traces;
    fds_mutex                       st_tab_mtx;
    fdsio::RequestQueue            *st_queues;
    StateEntry const *const *const  st_entries;

    int                             st_switch_cnt;
    const state_switch_t           *st_switch;

    void st_verify_wiring();
    void st_resume(boost::intrusive_ptr<StateObj> obj, int nxt_st);
    void st_defer_event_mtx(EventObj::pointer evt, boost::intrusive_ptr<StateObj> obj);
    void st_in_async_priv(EventObj::pointer evt, boost::intrusive_ptr<StateObj> obj);
    bool st_in_async_mtx(EventObj::pointer evt, boost::intrusive_ptr<StateObj> obj);
    void st_switch_state(EventObj::pointer evt, boost::intrusive_ptr<StateObj> obj);

    EventObj::pointer st_get_defer_event_mtx(boost::intrusive_ptr<StateObj> obj);

    EventObj::pointer
    st_do_resume(boost::intrusive_ptr<StateObj> obj,
                 boost::intrusive_ptr<EventObj> evt, int nxt_st);

  private:
    INTRUSIVE_PTR_DEFS(FsmTable, st_refcnt);
};

template <class T>
static inline T *fsm_cast_ptr(FsmTable::pointer st)
{
    return static_cast<T *>(get_pointer(st));
}

/**
 * Active state objects, we can have many such objects operate on the state table.
 */
class StateObj
{
  public:
    typedef boost::intrusive_ptr<StateObj> pointer;
    typedef boost::intrusive_ptr<const StateObj> const_ptr;

    static const int st_max_hist_mask = 0x7;
    static const int st_max_hist      = st_max_hist_mask + 1;
    static const int st_no_op         = 0x0000;
    static const int st_locked        = 0x0001;
    static const int st_busy          = 0x0002;
    static const int st_defer_evt     = 0x0004;
    static const int st_busy_locked   = (st_locked | st_busy);

    virtual ~StateObj() {}
    StateObj(int val, FsmTable::pointer tab);
    StateObj(int val, FsmTable::pointer tab, int refcnt);

    inline int st_current() const { return st_idx; }
    inline int st_previous() const {
        return st_db_hist[(st_db_idx - 1) & st_max_hist_mask];
    }
    inline char const *const st_curr_name() const {
        return st_tab->st_entries[st_idx]->st_name();
    }
    /* Resume the state transition, called by a callback requested by an action. */
    inline void st_resume(int nxt_st) { st_tab->st_resume(this, nxt_st); }

    /* Sync and async input to the state obj. */
    inline void st_input(EventObj::pointer evt) { st_tab->st_input(evt, this); }
    inline void st_in_async(EventObj::pointer evt) { st_tab->st_in_async(evt, this); }

    /* Bind/unbind event from the state object. */
    void st_bind_evt(EventObj::pointer evt);
    inline EventObj::pointer st_unbind_event(boost::intrusive_ptr<StateObj> obj) {
        return st_tab->st_unbind_event(obj);
    }
    inline boost::intrusive_ptr<StateObj> st_unbind_event(EventObj::pointer evt) {
        return st_tab->st_unbind_event(this, evt);
    }

    /* Lock to protect fields in this obj. */
    inline fds_mutex *st_mtx() const { return FsmTable::st_get_obj_mtx(this); }

    /* The state machine governs life-cycle of this obj. */
    inline FsmTable::pointer st_fsm() const { return st_tab; }

    /* Verify the consistency of the obj, panic if found any. */
    void st_verify(bool locked = false) const;

    /**
     * Clone a deferred event because the current state can't take it.  We use this
     * method when we want the caller to complete the event.
     */
    virtual EventObj::pointer st_clone_defer_evt(EventObj::pointer evt) const;

    /**
     * Sometimes we can't process an event at a certain state.  Defer the event until
     * we can process it.
     * @param evt - the deferred event.  It must be allocated on heap.
     * @param state - the state to process the event; st_no_change if don't care.
     * @param tick - the tick (sec) from now to repost the event; -1 no repost.
     */
    inline void st_push_defer_evt(EventObj::pointer evt, int state, int tick = -1) {
        st_tab->st_push_defer_evt(evt, this, state, tick);
    }
    inline void st_process_defer_evt() { st_tab->st_process_defer_evt(this); }

    /**
     * Given the current state, next state, and current event, return the next event
     * to feed in the next state.  The code can return the current event to reuse with
     * the next state.  Return NULL means the state transition is done and the original
     * event is considered to be completed.
     * */
    virtual EventObj::pointer st_mk_next_evt(EventObj::pointer cur, int next_st);

    /**
     * Debug utilities.
     */
    virtual void st_owner_info(std::ostream &os) const {}
    std::stringstream &st_trace(EventObj::pointer evt) {
        return st_tab->st_trace(this, evt);
    }
    friend std::ostream &operator << (std::ostream &os, const StateObj::pointer obj);

  protected:
    EventObj::pointer        st_cur_evt;
    FsmTable::pointer        st_tab;

  private:
    friend class FsmTable;

    /* These fields are protected by the lock inside the state machine. */
    int                      st_idx;
    fds_uint32_t             st_status : 16;
    fds_uint32_t             st_db_idx : 16;
    fds_uint8_t              st_db_hist[st_max_hist];
    INTRUSIVE_PTR_DEFS(StateObj, st_refcnt);

    /* These functions must be called with the obj's mutex held. */
    void st_change_mtx(int new_st);
    inline void st_set_lock_busy_mtx()
    {
        fds_assert(st_cur_evt == NULL);
        fds_assert(st_status == StateObj::st_no_op);
        st_status = StateObj::st_busy_locked;
    }
    inline void st_unlock_obj_mtx()
    {
        fds_assert(st_cur_evt == NULL);
        fds_assert(st_status == StateObj::st_locked);
        st_status = StateObj::st_no_op;
    }
    inline void st_clr_busy_mtx()
    {
        fds_verify((st_status & st_busy_locked) == st_busy_locked);
        st_status &= ~StateObj::st_busy;
    }
    inline void st_bind_evt_mtx(EventObj::pointer evt)
    {
        fds_assert(st_cur_evt == NULL);
        evt->evt_bind_owner(this);
        st_cur_evt = evt;
    }
    inline void st_unbind_evt_mtx()
    {
        if (st_cur_evt != NULL) {
            fds_assert(st_cur_evt->st_owner == this);
            st_cur_evt->evt_bind_owner(NULL);
            st_cur_evt = NULL;
        }
    }
};

template <class T>
static inline T *state_cast_ptr(StateObj::pointer st)
{
    return static_cast<T *>(get_pointer(st));
}

}  // namespace fds
#endif  // SOURCE_INCLUDE_FDS_FSM_H_
