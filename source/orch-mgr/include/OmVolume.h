/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_OMVOLUME_H_
#define SOURCE_ORCH_MGR_INCLUDE_OMVOLUME_H_

#include <string>
#include <vector>
#include <boost/msm/back/state_machine.hpp>
#include <fds_volume.h>
#include "has_state.h"
#include "platform/node_agent.h"
#include "fdsp/om_api_types.h"
#include <fdsp/common_types.h>
namespace fds {

class OmDiscoveryMod;
struct VolumeFSM;
class VolumeInfo;
typedef boost::msm::back::state_machine<VolumeFSM> FSM_Volume;

typedef enum {
    om_notify_vol_default = 0,
    om_notify_vol_add     = 1,
    om_notify_vol_rm      = 2,
    om_notify_vol_rm_chk  = 3,
    om_notify_vol_mod     = 4,
    om_notify_vol_attach  = 5,
    om_notify_vol_detach  = 6,
    om_notify_vol_max
} om_vol_notify_t;

/**
 * OM Volume life cycle events
 */
class VolCreateEvt
{
 public:
    explicit VolCreateEvt(VolumeInfo* vol)
            : vol_ptr(vol) {}

    VolumeInfo* vol_ptr;
};


class VolCrtOkEvt
{
 public:
    explicit VolCrtOkEvt(fds_bool_t b_ack, VolumeInfo* vol)
            : got_ack(b_ack), vol_ptr(vol) {}

    // if true, actual ack, false is used when want to check if no need
    // to wait for acks and can get out of the current state
    fds_bool_t got_ack;
    VolumeInfo* vol_ptr;
};

class VolOpEvt
{
 public:
    VolOpEvt(VolumeInfo* vol,
             FDS_ProtocolInterface::FDSP_MsgCodeType type,
             const NodeUuid& uuid)
            : vol_ptr(vol), op_type(type), tgt_uuid(uuid) {}

    VolOpEvt(VolumeInfo* vol,
             FDS_ProtocolInterface::FDSP_MsgCodeType type,
             boost::shared_ptr<VolumeDesc> vdesc)
            : vol_ptr(vol), op_type(type), vdesc_ptr(vdesc) {}

    VolumeInfo* vol_ptr;
    FDS_ProtocolInterface::FDSP_MsgCodeType op_type;
    NodeUuid tgt_uuid;
    boost::shared_ptr<VolumeDesc> vdesc_ptr;
};

class VolOpRespEvt
{
 public:
    VolOpRespEvt(const ResourceUUID& uuid,
                 VolumeInfo* vol_ptr,
                 om_vol_notify_t type,
                 Error err)
            : vol_uuid(uuid), vol_ptr(vol_ptr), resp_type(type), op_err(err) {}
    VolumeInfo* vol_ptr;
    ResourceUUID vol_uuid;
    om_vol_notify_t resp_type;
    Error op_err;
};

class VolDeleteEvt
{
 public:
    VolDeleteEvt(const ResourceUUID& uuid, VolumeInfo* vol)
            : vol_uuid(uuid), vol_ptr(vol) {}

    ResourceUUID vol_uuid;
    VolumeInfo* vol_ptr;
};

class ResumeDelEvt
{
  public:
    ResumeDelEvt(const ResourceUUID& uuid, VolumeInfo* vol)
            : vol_uuid(uuid), vol_ptr(vol) {}
    ResourceUUID vol_uuid;
    VolumeInfo* vol_ptr;
};


class DelRespEvt
{
 public:
    /**
     * @param recvd_ack is true if this event is called in
     * response to ack for delete check, and false otherwise.
     * Example of false: if we delayed delete operation because
     * rebalancing was going on, then we call this event when
     * rebalancing is finished so that we can continue vol delete
     */
    DelRespEvt(VolumeInfo* vol,
                 Error err,
                 fds_bool_t ack = true)
            : vol_ptr(vol), chk_err(err), recvd_ack(ack) {}

    VolumeInfo* vol_ptr;
    Error chk_err;
    fds_bool_t recvd_ack;
};

class DelNotifEvt
{
 public:
    explicit DelNotifEvt(VolumeInfo* vol)
            : vol_ptr(vol) {}

    VolumeInfo* vol_ptr;
};


/**
 * TODO(Vy): temp. interface for now to define generic volume message.
 */
typedef struct om_vol_msg_s
{
    fpi::FDSP_MsgCodeType            vol_msg_code;
    fpi::FDSP_MsgHdrTypePtr         *vol_msg_hdr;
    union {
        fpi::FDSP_BucketStatType    *vol_stats;
        // TODO(Andrew): This struct is not used but references
        // these removed variables. Removing the entire struct,
        // though useful, is not a ball of yarn I want to unwind
        // right now...
        // FdspNotVolPtr               *vol_notif;
    } u;
} om_vol_msg_t;

/**
 * Volume Object.
 */
class VolumeInfo : public Resource, public HasState
{
  public:
    typedef boost::intrusive_ptr<VolumeInfo> pointer;

    ~VolumeInfo();
    explicit VolumeInfo(const ResourceUUID &uuid);

    static inline VolumeInfo::pointer vol_cast_ptr(Resource::pointer ptr) {
        return static_cast<VolumeInfo *>(get_pointer(ptr));
    }

    void vol_mk_description(const fpi::FDSP_VolumeDescType &info);
    // Sets the volume descriptor to have the information of this volume info.
    void vol_fmt_desc_pkt(fpi::FDSP_VolumeDescType *pkt) const;
    void vol_fmt_message(om_vol_msg_t *out);

    void setDescription(const VolumeDesc &desc);
    Error vol_modify(const boost::shared_ptr<VolumeDesc>& vdesc_ptr);

    /**
     * Start volume delete process -- update admission control
     * and send volume detach to all AMs that have this vol attached
     * @return number of AMs we sent vol detach message
     */
    fds_uint32_t vol_start_delete();

    NodeAgent::pointer vol_am_agent(const NodeUuid &am_node);

    inline std::string &vol_get_name() {
        return vol_properties->name;
    }
    inline fds_volid_t vol_get_id() {
        return volUUID;
    }
    inline VolumeDesc *vol_get_properties() {
        return vol_properties;
    }

    fpi::ResourceState getState() const {
        if (vol_properties) return vol_properties->getState();
        return fpi::ResourceState::Unknown;
    }

    void setState(fpi::ResourceState state) {
        if (vol_properties) vol_properties->setState(state);
    }

    /**
     * Return the string containing current state of the volume
     */
    char const *const vol_current_state();
    void initSnapshotVolInfo(VolumeInfo::pointer vol, const fpi::Snapshot& snapshot);
    /**
     * Apply an event to volume lifecycle state machine
     */
    void vol_event(VolCreateEvt const &evt);
    void vol_event(VolCrtOkEvt const &evt);
    void vol_event(VolOpEvt const &evt);
    void vol_event(VolOpRespEvt const &evt);
    void vol_event(VolDeleteEvt const &evt);
    void vol_event(DelRespEvt const &evt);
    void vol_event(ResumeDelEvt const &evt);
    void vol_event(DelNotifEvt const &evt);
    fds_bool_t isVolumeInactive();
    fds_bool_t isDeletePending();
    fds_bool_t isCheckDelete();

  protected:
    std::string               vol_name;
    fds_volid_t               volUUID;
    VolumeDesc               *vol_properties;
    FSM_Volume *volume_fsm;
    // to protect access to msm process_event
    fds_mutex  fsm_lock;
};

class VolumeContainer : public RsContainer
{
  public:
    typedef boost::intrusive_ptr<VolumeContainer> pointer;

    virtual ~VolumeContainer();
    VolumeContainer();

    static inline VolumeInfo::pointer vol_from_iter(RsContainer::const_iterator it) {
        return static_cast<VolumeInfo *>(get_pointer(*it));
    }
    void vol_foreach(void (*fn)(VolumeInfo::pointer elm)) {
        VolumeInfo::pointer vol;
        for (fds_uint32_t i = 0; i < rs_cur_idx; i++) {
            vol = VolumeInfo::vol_cast_ptr(rs_array[i]);
            if (vol != NULL) {
                (*fn)(vol);
            }
        }
    }
    template <typename T>
    void vol_foreach(T arg, void (*fn)(T arg, VolumeInfo::pointer elm)) {
        VolumeInfo::pointer vol;
        for (fds_uint32_t i = 0; i < rs_cur_idx; i++) {
            vol = VolumeInfo::vol_cast_ptr(rs_array[i]);
            if (vol != NULL) {
                (*fn)(arg, vol);
            }
        }
    }
    template <typename T1, typename T2>
    void vol_foreach(T1 a1, T2 a2, void (*fn)(T1, T2, VolumeInfo::pointer elm)) {
        VolumeInfo::pointer vol;
        for (fds_uint32_t i = 0; i < rs_cur_idx; i++) {
            vol = VolumeInfo::vol_cast_ptr(rs_array[i]);
            if (vol != NULL) {
                (*fn)(a1, a2, vol);
            }
        }
    }
    template <typename T1, typename T2, typename T3>
    void vol_foreach(T1 a1, T2 a2, T3 a3,
                     void (*fn)(T1, T2, T3, VolumeInfo::pointer elm)) {
        VolumeInfo::pointer vol;
        for (fds_uint32_t i = 0; i < rs_cur_idx; i++) {
            vol = VolumeInfo::vol_cast_ptr(rs_array[i]);
            if (vol != NULL) {
                (*fn)(a1, a2, a3, vol);
            }
        }
    }
    template <typename T1, typename T2, typename T3, typename T4>
    void vol_foreach(T1 a1, T2 a2, T3 a3, T4 a4,
                     void (*fn)(T1, T2, T3, T4, VolumeInfo::pointer elm)) {
        VolumeInfo::pointer vol;
        for (fds_uint32_t i = 0; i < rs_cur_idx; i++) {
            vol = VolumeInfo::vol_cast_ptr(rs_array[i]);
            if (vol != NULL) {
                (*fn)(a1, a2, a3, a4, vol);
            }
        }
    }

    /**
     * Iteration over volumes that are not in 'delete pending' state
     */
    template <typename T>
    void vol_up_foreach (T arg, void (*fn)(T arg, VolumeInfo::pointer elm)) {
        VolumeInfo::pointer vol;
        for (fds_uint32_t i = 0; i < rs_cur_idx; i++) {
            vol = VolumeInfo::vol_cast_ptr(rs_array[i]);
            if (vol != NULL) {
                if (!vol->isDeletePending()) {
                    (*fn)(arg, vol);
                }
            }
        }
    }

    /**
     * Register the discovery manager for volume discovery tasks.
     */
    void vol_reg_disc_mgr(OmDiscoveryMod *disc_mgr) {
        vol_disc_mgr = disc_mgr;
    }

    /**
     * Volume functions.
     */
    virtual VolumeInfo::pointer get_volume(const std::string& vol_name);
    virtual VolumeInfo::pointer get_volume(const fds_volid_t volId);
    virtual Error om_create_vol(const fpi::FDSP_MsgHdrTypePtr &hdr,
                                const FdspCrtVolPtr           &creat_msg);
    virtual Error om_snap_vol(const fpi::FDSP_MsgHdrTypePtr &hdr,
                              const FdspCrtVolPtr           &snap_msg);
    virtual Error om_delete_vol(const fpi::FDSP_MsgHdrTypePtr &hdr,
                                const FdspDelVolPtr           &del_msg);
    Error om_delete_vol(fds_volid_t volId);
    virtual Error om_modify_vol(const FdspModVolPtr &mod_msg);
    virtual Error om_get_volume_descriptor(const boost::shared_ptr<fpi::AsyncHdr>     &hdr,
                                           const std::string& vol_name,
                                           VolumeDesc& desc);
    void om_vol_cmd_resp(VolumeInfo::pointer vol,
        fpi::FDSPMsgTypeId cmd_type, const Error & error, NodeUuid from_svc);

    virtual Error getVolumeStatus(const std::string& volumeName);

    Error addSnapshot(const fpi::Snapshot& snapshot);

    /**
     * Called by DMT state machine when rebalance is finished, and
     * volumes that are waiting for rebalance to finish (to be created)
     * can continue create process. In this method, we can assume
     * that rebalancing is off, because the state machine does not
     * continue until this method returns.
     */
    void continueCreateDeleteVolumes();

    /**
     * Handling responses for volume events
     */
    virtual void om_notify_vol_resp(om_vol_notify_t type,
                                    fpi::FDSP_MsgHdrTypePtr& fdsp_msg,
                                    const std::string& vol_name,
                                    const ResourceUUID& vol_uuid);

    virtual void om_notify_vol_resp(om_vol_notify_t type,
                                    NodeUuid from_src, Error err,
                                    const std::string& vol_name,
                                    const ResourceUUID& vol_uuid);
    /**
     * Handle final deletion of the volume
     */
    virtual void om_cleanup_vol(const ResourceUUID& vol_uuid);

    bool addVolume(const VolumeDesc& volumeDesc);
    bool createSystemVolume(int32_t tenantID = -1);
    void addToDeleteVols(const VolumeDesc volumeDesc);
    
    std::vector<VolumeDesc> getVolumesToDelete();
    

  protected:
    OmDiscoveryMod           *vol_disc_mgr;
    std::vector<VolumeDesc>   volumesToBeDeleted;

    virtual Resource *rs_new(const ResourceUUID &uuid) {
        return new VolumeInfo(uuid);
    }
};

}  // namespace fds

#endif  // SOURCE_ORCH_MGR_INCLUDE_OMVOLUME_H_
