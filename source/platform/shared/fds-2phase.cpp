/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <platform/fds-2phase.h>
#include <platform/node-inventory.h>

namespace fds {

class PhaseIdleSt : public StateEntry
{
  public:
    static inline int st_index() { return 0; }
    virtual char const *const st_name() const { return "2PhaseIdle"; }

    PhaseIdleSt() : StateEntry(st_index(), -1) {}
    int st_handle(EventObj::pointer e, StateObj::pointer cur) const override
    {
        return StateEntry::st_no_change;
    }
};

class PhasePrepare : public StateEntry
{
  public:
    static inline int st_index() { return 1; }
    virtual char const *const st_name() const { return "2PhasePrepare"; }

    PhasePrepare() : StateEntry(st_index(), -1) {}
    int st_handle(EventObj::pointer e, StateObj::pointer cur) const override
    {
        PhaseSync::ptr   obj;
        PhaseEvent::ptr  evt;

        evt = evt_cast_ptr<PhaseEvent>(e);
        obj = state_cast_ptr<PhaseSync>(cur);
        obj->act_phase_prepare(evt);
        return StateEntry::st_no_change;
    }
};

class PhaseCommit : public StateEntry
{
  public:
    static inline int st_index() { return 2; }
    virtual char const *const st_name() const { return "2PhaseCommit"; }

    PhaseCommit() : StateEntry(st_index(), -1) {}
    int st_handle(EventObj::pointer e, StateObj::pointer cur) const override
    {
        PhaseSync::ptr   obj;
        PhaseEvent::ptr  evt;

        evt = evt_cast_ptr<PhaseEvent>(e);
        obj = state_cast_ptr<PhaseSync>(cur);
        obj->act_phase_commit(evt);
        return StateEntry::st_no_change;
    }
};

class PhaseAbort : public StateEntry
{
  public:
    static inline int st_index() { return 3; }
    virtual char const *const st_name() const { return "2PhaseAbort"; }

    PhaseAbort() : StateEntry(st_index(), -1) {}
    virtual void st_enter(EventObj::pointer evt, StateObj::pointer cur) const override;
    int st_handle(EventObj::pointer e, StateObj::pointer cur) const override
    {
        PhaseSync::ptr   obj;
        PhaseEvent::ptr  evt;

        evt = evt_cast_ptr<PhaseEvent>(e);
        obj = state_cast_ptr<PhaseSync>(cur);
        obj->act_phase_abort(evt);
        return StateEntry::st_no_change;
    }
};

/*
 * -------------------------------------------------------------------------------------
 *  Phase Sync Client Side.
 * -------------------------------------------------------------------------------------
 */
PhaseSync::~PhaseSync() {}
PhaseSync::PhaseSync(fpi::SvcUuid &master,
                     NodeAgent::pointer owner, FsmTable::pointer tab)
    : StateObj(PhaseIdleSt::st_index(), tab), ph_master_uuid(master), ph_owner(owner) {}

void
PhaseSync::act_phase_prepare(PhaseEvent::ptr evt)
{
}

void
PhaseSync::act_phase_commit(PhaseEvent::ptr evt)
{
}

void
PhaseSync::act_phase_abort(PhaseEvent::ptr evt)
{
}

void
PhaseSync::act_fmt_prepare_ack(fpi::PhaseSyncPtr &pkt)
{
}

void
PhaseSync::act_fmt_commit_ack(fpi::PhaseSyncPtr &pkt)
{
}

void
PhaseSync::act_fmt_abort(fpi::PhaseSyncPtr &pkt)
{
}

void
PhaseSync::act_fmt_abort_ack(fpi::PhaseSyncPtr &pkt)
{
}

void
PhaseSync::act_send_prepare_ack(fpi::SvcUuid *dst, fpi::PhaseSyncPtr &pkt)
{
}

void
PhaseSync::act_send_commit_ack(fpi::SvcUuid *dst, fpi::PhaseSyncPtr &pkt)
{
}

void
PhaseSync::act_send_abort(fpi::SvcUuid *dst, fpi::PhaseSyncPtr &pkt)
{
}

void
PhaseSync::act_send_abort_ack(fpi::SvcUuid *dst, fpi::PhaseSyncPtr &pkt)
{
}

void
PhaseSync::act_recv_sync(fpi::AsyncHdrPtr &hdr, fpi::PhaseSyncPtr &pkt)
{
}

/*
 * -------------------------------------------------------------------------------------
 *  Phase Sync Master Side.
 * -------------------------------------------------------------------------------------
 */
PhaseSyncMaster::~PhaseSyncMaster() {}
PhaseSyncMaster::PhaseSyncMaster(fpi::SvcUuid &self,
                                 NodeAgent::pointer owner, FsmTable::pointer tab)
    : PhaseSync(self, owner, tab), ph_commit(NULL), ph_sync_ack() {}

void
PhaseSyncMaster::act_commit_start(AgentArray *arr)
{
}

void
PhaseSyncMaster::act_commit_abort()
{
}

void
PhaseSyncMaster::act_fmt_phase_prepare(fpi::PhaseSyncPtr &pkt)
{
}

void
PhaseSyncMaster::act_fmt_phase_commit(fpi::PhaseSyncPtr &pkt)
{
}

void
PhaseSyncMaster::act_fmt_phase_abort(fpi::PhaseSyncPtr &pkt)
{
}

void
PhaseSyncMaster::act_send_prepare(fpi::SvcUuid *dst, fpi::PhaseSyncPtr &pkt)
{
}

void
PhaseSyncMaster::act_send_commit(fpi::SvcUuid *dst, fpi::PhaseSyncPtr &pkt)
{
}

/*
 * -------------------------------------------------------------------------------------
 *  2-Phase Sync Table
 * -------------------------------------------------------------------------------------
 */
static const state_switch_t sgl_2phase_flow[] =
{
    { PhaseIdleSt::st_index(),   two::phase_prepare,     PhasePrepare::st_index() },
    { PhaseIdleSt::st_index(),   two::phase_prepare_ack, PhaseIdleSt::st_index() },
    { PhaseIdleSt::st_index(),   two::phase_commit,      PhaseIdleSt::st_index() },
    { PhaseIdleSt::st_index(),   two::phase_commit_ack,  PhaseIdleSt::st_index() },
    { PhaseIdleSt::st_index(),   two::phase_abort,       PhaseIdleSt::st_index() },
    { PhaseIdleSt::st_index(),   two::phase_abort_ack,   PhaseIdleSt::st_index() },

    { PhasePrepare::st_index(),  two::phase_prepare_ack, PhaseCommit::st_index() },
    { PhasePrepare::st_index(),  two::phase_commit,      PhaseCommit::st_index() },
    { PhasePrepare::st_index(),  two::phase_prepare,     PhasePrepare::st_index() },
    { PhasePrepare::st_index(),  two::phase_commit_ack,  PhaseAbort::st_index() },
    { PhasePrepare::st_index(),  two::phase_abort,       PhaseAbort::st_index() },
    { PhasePrepare::st_index(),  two::phase_abort_ack,   PhaseAbort::st_index() },

    { PhaseCommit::st_index(),   two::phase_commit_ack,  PhaseIdleSt::st_index() },
    { PhaseCommit::st_index(),   two::phase_commit,      PhaseCommit::st_index() },
    { PhaseCommit::st_index(),   two::phase_prepare,     PhaseAbort::st_index() },
    { PhaseCommit::st_index(),   two::phase_prepare_ack, PhaseAbort::st_index() },
    { PhaseCommit::st_index(),   two::phase_abort,       PhaseAbort::st_index() },
    { PhaseCommit::st_index(),   two::phase_abort_ack,   PhaseAbort::st_index() },

    { PhaseAbort::st_index(),    two::phase_abort,       PhaseIdleSt::st_index() },
    { PhaseAbort::st_index(),    two::phase_abort_ack,   PhaseIdleSt::st_index() },
    { -1,                        two::phase_noop,        -1 }
};

static const PhaseIdleSt        sgt_phase_idle;
static const PhasePrepare       sgt_phase_prepare;
static const PhaseCommit        sgt_phase_commit;
static const PhaseAbort         sgt_phase_abort;

static StateEntry const *const sgt_2phase_tab[] =
{
    &sgt_phase_idle,
    &sgt_phase_prepare,
    &sgt_phase_commit,
    &sgt_phase_abort,
    NULL
};

PhaseSyncTab::~PhaseSyncTab() {}
PhaseSyncTab::PhaseSyncTab()
    : FsmTable(FDS_ARRAY_ELEM(sgt_2phase_tab), sgt_2phase_tab), ph_master(NULL)
{
    st_set_switch_board(sgl_2phase_flow, FDS_ARRAY_ELEM(sgl_2phase_flow));
}

void
PhaseSyncTab::mod_init()
{
    gl_OmUuid.uuid_assign(&ph_om_dest);
}

void
PhaseSyncTab::mod_startup()
{
}

void
PhaseSyncTab::mod_shutdown()
{
}

void
PhaseSyncTab::phase_sync_create(fpi::SvcUuid &peer, NodeAgent::pointer owner)
{
}

void
PhaseSyncTab::ph_recv_sync(fpi::AsyncHdrPtr &hdr, fpi::PhaseSyncPtr &pkt)
{
}

void
PhaseSyncTab::ph_sync_submit(fpi::AsyncHdrPtr &hdr, PhaseEvent::ptr evt)
{
}

PhaseSync::ptr
PhaseSyncTab::ph_sync_alloc(fpi::SvcUuid &svc, NodeAgent::pointer owner)
{
    return new PhaseSync(svc, owner, this);
}

PhaseSyncMaster::ptr
PhaseSyncTab::ph_master_alloc(NodeAgent::pointer owner)
{
    fpi::SvcUuid om;

    gl_OmUuid.uuid_assign(&om);
    return new PhaseSyncMaster(om, owner, this);
}

}  // namespace fds
