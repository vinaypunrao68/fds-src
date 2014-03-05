/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_OMVOLUME_H_
#define SOURCE_ORCH_MGR_INCLUDE_OMVOLUME_H_

#include <string>
#include <vector>
#include <fds_volume.h>
#include <platform/node-inventory.h>

namespace fds {

class OmDiscoveryMod;

/**
 * TODO(Vy): temp. interface for now to define generic volume message.
 */
typedef struct om_vol_msg_s
{
    fpi::FDSP_MsgCodeType            vol_msg_code;
    fpi::FDSP_MsgHdrTypePtr         *vol_msg_hdr;
    union {
        FDSP_BucketStatType         *vol_stats;
        FdspNotVolPtr               *vol_notif;
        FdspAttVolPtr               *vol_attach;
    } u;
} om_vol_msg_t;

/**
 * Volume Object.
 */
class VolumeInfo : public Resource
{
  public:
    typedef boost::intrusive_ptr<VolumeInfo> pointer;

    ~VolumeInfo();
    explicit VolumeInfo(const ResourceUUID &uuid);

    static inline VolumeInfo::pointer vol_cast_ptr(Resource::pointer ptr) {
        return static_cast<VolumeInfo *>(get_pointer(ptr));
    }

    void vol_mk_description(const fpi::FDSP_VolumeInfoType &info);
    void vol_fmt_desc_pkt(FDSP_VolumeDescType *pkt) const;
    void vol_fmt_message(om_vol_msg_t *out);
    void vol_send_message(om_vol_msg_t *out, NodeAgent::pointer dest);

    void setDescription(const VolumeDesc &desc);
    void vol_attach_node(const std::string &node_name);
    void vol_detach_node(const std::string &node_name);

    NodeAgent::pointer vol_am_agent(const std::string &am_node);

    inline std::string &vol_get_name() {
        return vol_properties->name;
    }
    inline VolumeDesc *vol_get_properties() {
        return vol_properties;
    }

    /**
     * Iter plugin to apply the function through each NodeAgent in the vol.
     */
    template <typename T>
    void vol_foreach_am(T a, void (*fn)(T, VolumeInfo::pointer, NodeAgent::pointer)) {
        // TODO(Vy): not thread safe for now...
        for (uint32_t i = 0; i < vol_am_nodes.size(); i++) {
            NodeAgent::pointer am = vol_am_agent(vol_am_nodes[i]);
            if (am != NULL) {
                (*fn)(a, this, am);
            }
        }
    }
    template <typename T1, typename T2>
    void vol_foreach_am(T1 a1, T2 a2,
                        void (*fn)(T1, T2,
                                   VolumeInfo::pointer, NodeAgent::pointer)) {
        // TODO(Vy): not thread safe for now...
        for (uint32_t i = 0; i < vol_am_nodes.size(); i++) {
            NodeAgent::pointer am = vol_am_agent(vol_am_nodes[i]);
            if (am != NULL) {
                (*fn)(a1, a2, this, am);
            }
        }
    }
    template <typename T1, typename T2, typename T3>
    void vol_foreach_am(T1 a1, T2 a2, T3 a3,
                        void (*fn)(T1, T2, T3,
                                   VolumeInfo::pointer, NodeAgent::pointer)) {
        // TODO(Vy): not thread safe for now...
        for (uint32_t i = 0; i < vol_am_nodes.size(); i++) {
            NodeAgent::pointer am = vol_am_agent(vol_am_nodes[i]);
            if (am != NULL) {
                (*fn)(a1, a2, a3, this, am);
            }
        }
    }
    template <typename T1, typename T2, typename T3, typename T4>
    void vol_foreach_am(T1 a1, T2 a2, T3 a3, T4 a4,
                        void (*fn)(T1, T2, T3, T4,
                                   VolumeInfo::pointer, NodeAgent::pointer)) {
        // TODO(Vy): not thread safe for now...
        for (uint32_t i = 0; i < vol_am_nodes.size(); i++) {
            NodeAgent::pointer am = vol_am_agent(vol_am_nodes[i]);
            if (am != NULL) {
                (*fn)(a1, a2, a3, a4, this, am);
            }
        }
    }

  protected:
    std::string               vol_name;
    fds_volid_t               volUUID;
    VolumeDesc               *vol_properties;
    std::vector<std::string>  vol_am_nodes;
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
    template <typename T>
    void vol_foreach(T arg, void (*fn)(T arg, VolumeInfo::pointer elm)) {
        for (fds_uint32_t i = 0; i < rs_cur_idx; i++) {
            VolumeInfo::pointer vol = VolumeInfo::vol_cast_ptr(rs_array[i]);
            if (rs_array[i] != NULL) {
                (*fn)(arg, vol);
            }
        }
    }
    template <typename T1, typename T2>
    void vol_foreach(T1 a1, T2 a2, void (*fn)(T1, T2, VolumeInfo::pointer elm)) {
        for (fds_uint32_t i = 0; i < rs_cur_idx; i++) {
            VolumeInfo::pointer vol = VolumeInfo::vol_cast_ptr(rs_array[i]);
            if (rs_array[i] != NULL) {
                (*fn)(a1, a2, vol);
            }
        }
    }
    template <typename T1, typename T2, typename T3>
    void vol_foreach(T1 a1, T2 a2, T3 a3,
                     void (*fn)(T1, T2, T3, VolumeInfo::pointer elm)) {
        for (fds_uint32_t i = 0; i < rs_cur_idx; i++) {
            VolumeInfo::pointer vol = VolumeInfo::vol_cast_ptr(rs_array[i]);
            if (rs_array[i] != NULL) {
                (*fn)(a1, a2, a3, vol);
            }
        }
    }
    template <typename T1, typename T2, typename T3, typename T4>
    void vol_foreach(T1 a1, T2 a2, T3 a3, T4 a4,
                     void (*fn)(T1, T2, T3, T4, VolumeInfo::pointer elm)) {
        for (fds_uint32_t i = 0; i < rs_cur_idx; i++) {
            VolumeInfo::pointer vol = VolumeInfo::vol_cast_ptr(rs_array[i]);
            if (rs_array[i] != NULL) {
                (*fn)(a1, a2, a3, a4, vol);
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
    virtual int om_create_vol(const FDSP_MsgHdrTypePtr  &hdr,
                              const FdspCrtVolPtr       &creat_msg);
    virtual int om_delete_vol(const FdspDelVolPtr &del_msg);
    virtual int om_modify_vol(const FdspModVolPtr &mod_msg);
    virtual int om_attach_vol(const FDSP_MsgHdrTypePtr  &hdr,
                              const FdspAttVolCmdPtr    &attach);
    virtual int om_detach_vol(const FDSP_MsgHdrTypePtr  &hdr,
                              const FdspAttVolCmdPtr    &detach);
    virtual void om_test_bucket(const FdspMsgHdrPtr     &hdr,
                                const FdspTestBucketPtr &req);

    bool addVolume(const VolumeDesc& volumeDesc);

  protected:
    OmDiscoveryMod           *vol_disc_mgr;

    virtual Resource *rs_new(const ResourceUUID &uuid) {
        return new VolumeInfo(uuid);
    }
};

}  // namespace fds

#endif  // SOURCE_ORCH_MGR_INCLUDE_OMVOLUME_H_
