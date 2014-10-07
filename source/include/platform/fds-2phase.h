/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_FDS_2PHASE_H_
#define SOURCE_INCLUDE_NET_FDS_2PHASE_H_

#include <vector>
#include <fds-fsm.h>
#include <fdsp/fds_service_types.h>

namespace bo  = boost;
namespace fpi = FDS_ProtocolInterface;

namespace fds {
namespace two {
    const int phase_noop         = 0;
    const int phase_prepare      = 1;
    const int phase_prepare_ack  = 2;
    const int phase_commit       = 3;
    const int phase_commit_ack   = 4;
    const int phase_abort        = 5;
    const int phase_abort_ack    = 6;
}

class NodeAgent;

struct PhaseEvent : public EventObj
{
    typedef boost::intrusive_ptr<PhaseEvent> ptr;

    fpi::AsyncHdrPtr              evt_hdr;
    fpi::PhaseSyncPtr             evt_msg;

    PhaseEvent(int evt_id, fpi::AsyncHdrPtr &hdr, fpi::PhaseSyncPtr &pkt)
        : EventObj(evt_id), evt_hdr(hdr), evt_msg(pkt) {}
};

/**
 * Client side object receiving 2-phase commit from the master.
 */
class PhaseSync : public StateObj
{
  public:
    typedef boost::intrusive_ptr<PhaseSync> ptr;

    virtual ~PhaseSync();
    PhaseSync(fpi::SvcUuid &, boost::intrusive_ptr<NodeAgent>, FsmTable::pointer);

    /**
     * Generic action functions, provided by derrived class.
     */
    virtual void act_phase_prepare(PhaseEvent::ptr);
    virtual void act_phase_commit(PhaseEvent::ptr);
    virtual void act_phase_abort(PhaseEvent::ptr);

    /**
     * Send data back to the orgininator.
     */
    virtual void act_fmt_prepare_ack(fpi::PhaseSyncPtr &);
    virtual void act_fmt_commit_ack(fpi::PhaseSyncPtr &);
    virtual void act_fmt_abort(fpi::PhaseSyncPtr &);
    virtual void act_fmt_abort_ack(fpi::PhaseSyncPtr &);

    virtual void act_send_prepare_ack(fpi::SvcUuid *, fpi::PhaseSyncPtr &);
    virtual void act_send_commit_ack(fpi::SvcUuid *, fpi::PhaseSyncPtr &);
    virtual void act_send_abort(fpi::SvcUuid *, fpi::PhaseSyncPtr &);
    virtual void act_send_abort_ack(fpi::SvcUuid *, fpi::PhaseSyncPtr &);

    /**
     * Receive network message.
     */
    virtual void act_recv_sync(fpi::AsyncHdrPtr &, fpi::PhaseSyncPtr &);

  protected:
    fpi::SvcUuid                     ph_master_uuid;
    boost::intrusive_ptr<NodeAgent>  ph_owner;
};

typedef std::vector<boost::intrusive_ptr<NodeAgent>> AgentArray;

/**
 * Master object initiates 2-phase commit to update a list of clients.
 */
class PhaseSyncMaster : public PhaseSync
{
  public:
    typedef boost::intrusive_ptr<PhaseSyncMaster> ptr;

    virtual ~PhaseSyncMaster();
    PhaseSyncMaster(fpi::SvcUuid &, boost::intrusive_ptr<NodeAgent>, FsmTable::pointer);

    /**
     * Master 2-phase commit flow:
     * Master calls............ act_commit_start(arr);
     * ... is called with .. act_phase_prepare() when all clients are done.
     * ...... calls ........ act_commit_abort() to abort the update
     * ... is called with .. act_phase_commit() when all clients are done.
     */
    virtual void act_commit_start(AgentArray *arr);
    virtual void act_commit_abort();

    /**
     * Send data to clients.
     */
    virtual void act_fmt_phase_prepare(fpi::PhaseSyncPtr &);
    virtual void act_fmt_phase_commit(fpi::PhaseSyncPtr &);
    virtual void act_fmt_phase_abort(fpi::PhaseSyncPtr &);

    virtual void act_send_prepare(fpi::SvcUuid *, fpi::PhaseSyncPtr &);
    virtual void act_send_commit(fpi::SvcUuid *, fpi::PhaseSyncPtr &);

  public:
    AgentArray                   *ph_commit;
    AgentArray                    ph_sync_ack;
};

/**
 * Two phase commit sync. table.
 */
class PhaseSyncTab : public FsmTable
{
  public:
    PhaseSyncTab();
    virtual ~PhaseSyncTab();

    /* Hookup with module methods. */
    void mod_init();
    void mod_startup();
    void mod_shutdown();

    /* Factory method creates and binds the 2-phase sync to node agent. */
    virtual void phase_sync_create(fpi::SvcUuid &peer, boost::intrusive_ptr<NodeAgent>);

    /* Entry to process network messages. */
    virtual void ph_recv_sync(fpi::AsyncHdrPtr &, fpi::PhaseSyncPtr &);

    /* The master obj can be NULL. */
    inline PhaseSyncMaster::ptr  ph_sync_master() { return ph_master; }

  protected:
    fpi::SvcUuid                  ph_om_dest;
    PhaseSyncMaster::ptr          ph_master;

    void ph_sync_submit(fpi::AsyncHdrPtr &, PhaseEvent::ptr);

    /* Factory allocator method. */
    virtual PhaseSync::ptr ph_sync_alloc(fpi::SvcUuid &, boost::intrusive_ptr<NodeAgent>);
    virtual PhaseSyncMaster::ptr ph_master_alloc(boost::intrusive_ptr<NodeAgent>);
};

}  // namespace fds
#endif  // SOURCE_INCLUDE_NET_FDS_2PHASE_H_
