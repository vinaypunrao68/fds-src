/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_OMDMTDEPLOY_H_
#define SOURCE_ORCH_MGR_INCLUDE_OMDMTDEPLOY_H_

#include <vector>
#include <string>
#include <boost/msm/back/state_machine.hpp>
#include <OmResources.h>
#include <OmDataPlacement.h>

namespace fds {

class ClusterMap;
struct DmtDplyFSM;
typedef boost::msm::back::state_machine<DmtDplyFSM> FSM_DplyDMT;

/**
 * OM DMT deployment events.
 */
class DmtDeployEvt
{
  public:
    explicit DmtDeployEvt(fds_bool_t _inReSync) : dmResync(_inReSync) {}
    std::string logString() const {
    	if (dmResync) {
    		return "DmtDeployEvt with DmResync";
    	} else {
    		return "DmtDeployEvt";
    	}
    }

    fds_bool_t dmResync;  // if true, DMT computation will be for DM Resync
};

/**
 * Event deployed to check to see if there's ongoing migrations, to see if we need
 * to error out
 */
class DmtUpEvt
{
    public:
        explicit DmtUpEvt(const NodeUuid& _uuid) : uuid(_uuid) {}
        std::string logString() const {
            return "DmtUpEvt with node: " + std::to_string(uuid.uuid_get_val());
        }

        NodeUuid uuid;
};

class DmtRecoveryEvt
{
  public:
    DmtRecoveryEvt(fds_bool_t abortAck,
                   const NodeUuid& uuid,
                   const Error& err)
            : ackForAbort(abortAck), svcUuid(uuid), ackError(err) {}
    std::string logString() const {
        return "DmtRecoveryEvt";
    }

    fds_bool_t ackForAbort;  // otherwise dlt commit ack
    NodeUuid svcUuid;
    Error ackError;
};

class DmtLoadedDbEvt
{
  public:
    DmtLoadedDbEvt() {}
    std::string logString() const {
        return "DmtLoadedDbEvt";
    }
};

class DmtPushMetaAckEvt
{
  public:
    explicit DmtPushMetaAckEvt(const NodeUuid& uuid)
      : dm_uuid(uuid) {}
    std::string logString() const {
        return "DmtPushMetaAckEvt";
    }

    NodeUuid dm_uuid;
};

class DmtCommitAckEvt
{
  public:
    explicit DmtCommitAckEvt(fds_uint64_t dmt_ver,
                             fpi::FDSP_MgrIdType type)
            : dmt_version(dmt_ver),
            svc_type(type) {}
    std::string logString() const {
        return "DmtCommitAckEvt";
    }

    fds_uint64_t dmt_version;
    fpi::FDSP_MgrIdType svc_type;
};

class DmtCloseOkEvt
{
  public:
    explicit DmtCloseOkEvt(fds_uint64_t dmt_ver)
            : dmt_version(dmt_ver) {}
    std::string logString() const {
        return "DmtCloseOkEvt";
    }

    fds_uint64_t dmt_version;
};

class DmtVolAckEvt
{
  public:
    explicit DmtVolAckEvt(const NodeUuid& uuid)
            : dm_uuid(uuid) {}
    std::string logString() const {
        return "DDmtVolAckEvt";
    }

    NodeUuid dm_uuid;
};
class DmtTimeoutEvt
{
  public:
    DmtTimeoutEvt() {}
    std::string logString() const {
      return "DmtTimeoutEvt";
    }
};

struct DmtErrorFoundEvt
{
    DmtErrorFoundEvt(const NodeUuid& svc_uuid,
                     const Error& err)
            : svcUuid(svc_uuid), error(err) {
    }

    std::string logString() const {
        return "DmtErrorFoundEvt";
    }

    NodeUuid svcUuid;  // service uuid where error happened
    Error error;
};

struct DmtEndErrorEvt
{
    DmtEndErrorEvt() {}
    std::string logString() const {
        return "DmtEndErrorEvt";
    }
};

/**
 * Main vector to initialize the DMT module.
 */
class OM_DMTMod : public Module
{
  public:
    explicit OM_DMTMod(char const *const name);
    ~OM_DMTMod();

    /**
     * The following are used for volume group mode, where we will
     * only calculate a DMT if a minimal DM cluster count is met.
     * Since these methods are called only by the timer in a sequential
     * fashion, we shouldn't run into race condition.
     * NOTE: If these are to be used outside of the timer schedule context, then
     * locks/atomics may be needed.
     */
    inline bool volumeGrpMode() {
        return volume_grp_mode;
    }

    /**
     * If a DM cluster is already present, then this is a no-op.
     * Behavior will change once we support multiple DM clusters.
     * If DM cluster isn't present, then this will increment the counter
     * that counts the DMs awaiting to form a DM cluster.
     */
    void addWaitingDMs();

    inline void removeWaitingDMs() {
        if (waitingDMs > 0) {
            --waitingDMs;
        }
    }

    inline uint32_t getWaitingDMs() {
        return waitingDMs;
    }

    inline void clearWaitingDMs() {
        waitingDMs = 0;
    }

    // used for unit test
    inline void setVolumeGrpMode(fds_bool_t value) {
        volume_grp_mode = value;
    }

    /**
     * Return the current state of the DMT deployment FSM.
     */
    char const *const dmt_deploy_curr_state();

    /**
     * Apply an event to DMT deploy state machine.
     */
    void dmt_deploy_event(DmtDeployEvt const &evt);
    void dmt_deploy_event(DmtPushMetaAckEvt const &evt);
    void dmt_deploy_event(DmtCommitAckEvt const &evt);
    void dmt_deploy_event(DmtCloseOkEvt const &evt);
    void dmt_deploy_event(DmtVolAckEvt const &evt);
    void dmt_deploy_event(DmtLoadedDbEvt const &evt);
    void dmt_deploy_event(DmtTimeoutEvt const &evt);
    void dmt_deploy_event(DmtErrorFoundEvt const &evt);
    void dmt_deploy_event(DmtRecoveryEvt const &evt);
    void dmt_deploy_event(DmtUpEvt const &evt);

    /**
     * Module methods
     */
    virtual int  mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

  private:
    FSM_DplyDMT     *dmt_dply_fsm;
    // to protect access to msm process_event
    fds_mutex       fsm_lock;
    // Toggles for service replica mode
    bool            volume_grp_mode;
    // Batch add for dm cluster
    uint32_t        waitingDMs;
};

extern OM_DMTMod             gl_OMDmtMod;

}  // namespace fds

#endif  // SOURCE_ORCH_MGR_INCLUDE_OMDMTDEPLOY_H_
