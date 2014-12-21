/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_NODE_INVENTORY_X_H_
#define SOURCE_INCLUDE_PLATFORM_NODE_INVENTORY_X_H_

#include <string>

#include "fds-shmobj.h"
#include "fds_resource.h"

#include "typedefs.h"





namespace fds
{
    /**
     * Basic info about a peer node.
     */
    class NodeInventory : public Resource
    {
        public:
            typedef boost::intrusive_ptr<NodeInventory> pointer;
            typedef boost::intrusive_ptr<const NodeInventory> const_ptr;

            std::string get_node_name() const
            {
                return nd_node_name;
            }

            std::string get_ip_str() const;
            std::string get_node_root() const;
            const struct node_stor_cap   *node_capability() const;
            void node_get_shm_rec(struct node_data *ndata) const;

            /* Temp. solution before we can replace netsession */
            fds_uint32_t node_base_port() const;

            void set_node_state(FdspNodeState state);
            void set_node_dlt_version(fds_uint64_t dlt_version);

            inline NodeUuid get_uuid() const
            {
                return rs_get_uuid();
            }

            inline FdspNodeState node_state() const
            {
                return nd_my_node_state;
            }

            inline fds_uint64_t node_dlt_version() const
            {
                return nd_my_dlt_version;
            }

            inline FdspNodeType node_get_svc_type() const
            {
                return node_svc_type;
            }

            /**
             * Fill in the inventory for this agent based on data provided by the message.
             */
            void svc_info_frm_shm(fpi::SvcInfo *svc) const;
            void node_info_frm_shm(struct node_data *out) const;
            void node_fill_shm_inv(const ShmObjRO *shm, int ro, int rw, FdspNodeType id);
            void node_fill_inventory(const FdspNodeRegPtr msg);
            void node_update_inventory(const FdspNodeRegPtr msg);

            /**
             * Format the node info pkt with data from this agent obj.
             */
            virtual void init_msg_hdr(fpi::FDSP_MsgHdrTypePtr msgHdr) const;
            virtual void init_node_info_pkt(fpi::FDSP_Node_Info_TypePtr pkt) const;
            virtual void init_node_reg_pkt(fpi::FDSP_RegisterNodeTypePtr pkt) const;
            virtual void init_stor_cap_msg(fpi::StorCapMsg *msg) const;
            virtual void init_plat_info_msg(fpi::NodeInfoMsg *msg) const;
            virtual void init_node_info_msg(fpi::NodeInfoMsg *msg) const;
            virtual void init_om_info_msg(fpi::NodeInfoMsg *msg);
            virtual void init_om_pm_info_msg(fpi::NodeInfoMsg *msg);

            /**
             * Convert from message format to POD type used in shared memory.
             */
            static void node_info_msg_to_shm(const fpi::NodeInfoMsg *msg, struct node_data *rec);
            static void node_info_msg_frm_shm(bool am, int ro_idx, fpi::NodeInfoMsg *msg);
            static void node_stor_cap_to_shm(const fpi::StorCapMsg *msg, struct node_stor_cap *);
            static void node_stor_cap_frm_shm(fpi::StorCapMsg *msg, const struct node_stor_cap *);

        protected:
            FdspNodeType     node_svc_type;
            int              node_ro_idx;
            int              node_rw_idx;
            fds_uint64_t     nd_gbyte_cap;
            fds_uint64_t     nd_my_dlt_version;
            std::string      node_root;

            /* Will be removed */
            FdspNodeState    nd_my_node_state;
            std::string      nd_node_name;

            virtual ~NodeInventory()
            {
            }

            explicit NodeInventory(const NodeUuid &uuid)
                : Resource(uuid), node_svc_type(fpi::FDSP_PLATFORM),
                node_ro_idx(-1), node_rw_idx(-1), nd_gbyte_cap(0),
                nd_my_dlt_version(0), nd_my_node_state(fpi::FDS_Node_Discovered)
            {
            }

            const ShmObjRO *node_shm_ctrl() const;
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_NODE_INVENTORY_X_H_
