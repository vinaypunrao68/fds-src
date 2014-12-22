/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_PLATFORM_H_
#define SOURCE_INCLUDE_PLATFORM_PLATFORM_H_

#include <string>

#include "platform/plat_node_data.h"
#include "platform/domain_container.h"
#include "platform/domain_resources.h"
#include "platform/domain_cluster_map.h"
#include "platform/flags_map.h"

namespace fpi = FDS_ProtocolInterface;

class netSessionTbl;

namespace fds
{
    class BaseAsyncSvcHandler;
    class Platform;

    extern Platform         *gl_PlatformSvc;
    extern const NodeUuid    gl_OmUuid;

    /**
     * Enums of different services sharing the same UUID EP.
     */
    const int    NET_SVC_CTRL        = 1;
    const int    NET_SVC_CONFIG      = 2;
    const int    NET_SVC_DATA        = 3;
    const int    NET_SVC_MIGRATION   = 4;
    const int    NET_SVC_META_SYNC   = 5;

    class Platform : public Module
    {
        public:
            static const int    plat_svc_types[];
            static fds_uint32_t plf_get_my_max_svc_ports();


            typedef Platform const *const ptr;

            virtual ~Platform();
            Platform(char const *const name, fpi::FDSP_MgrIdType node_type,
                     DomainContainer::pointer node_inv, DomainClusterMap::pointer cluster,
                     DomainResources::pointer resources, OmAgent::pointer master);

            /**
             * Short cuts to retrieve important objects.
             */
            static inline void platf_assign_singleton(Platform *ptr)
            {
                gl_PlatformSvc = ptr;
            }

            static inline Platform     *platf_singleton()
            {
                return gl_PlatformSvc;
            }

            static inline Platform::ptr platf_const_singleton()
            {
                return gl_PlatformSvc;
            }

            inline DomainContainer::pointer  plf_node_inventory()
            {
                return plf_node_inv;
            }

            inline DomainClusterMap::pointer plf_cluster_map()
            {
                return plf_clus_map;
            }

            inline DomainResources::pointer  plf_node_resoures()
            {
                return plf_resources;
            }

            static inline SmContainer::pointer plf_sm_nodes()
            {
                return platf_singleton()->plf_node_inv->dc_get_sm_nodes();
            }

            static inline SmContainer::pointer plf_sm_cluster()
            {
                return platf_singleton()->plf_clus_map->dc_get_sm_nodes();
            }

            static inline DmContainer::pointer plf_dm_nodes()
            {
                return platf_singleton()->plf_node_inv->dc_get_dm_nodes();
            }

            static inline DmContainer::pointer plf_dm_cluster()
            {
                return platf_singleton()->plf_clus_map->dc_get_dm_nodes();
            }

            static inline AmContainer::pointer plf_am_nodes()
            {
                return platf_singleton()->plf_node_inv->dc_get_am_nodes();
            }

            static inline AmContainer::pointer plf_am_cluster()
            {
                return platf_singleton()->plf_clus_map->dc_get_am_nodes();
            }

            static inline PmContainer::pointer plf_pm_nodes()
            {
                return platf_singleton()->plf_node_inv->dc_get_pm_nodes();
            }

            static inline NodeUuid const *const plf_get_my_node_uuid()
            {
                return &platf_singleton()->plf_my_uuid;
            }

            static inline NodeUuid const *const plf_get_my_svc_uuid()
            {
                return &platf_singleton()->plf_my_svc_uuid;
            }

            static inline void plf_om_svc_ep(fpi::SvcUuid *mine, fpi::SvcUuid *om)
            {
                gl_OmUuid.uuid_assign(om);
                platf_singleton()->plf_my_svc_uuid.uuid_assign(mine);
            }

            static inline NodeAgent::pointer plf_find_node_agent(const NodeUuid &nd_uuid)
            {
                return platf_singleton()->plf_node_inv->dc_find_node_agent(nd_uuid);
            }

            static inline NodeAgent::pointer plf_find_node_agent(const fpi::SvcUuid &svc)
            {
                NodeUuid    nd_uuid(svc.svc_uuid);
                return plf_find_node_agent(nd_uuid);
            }

            /**
             * Return service uuid and port from node uuid and service type.
             */
            static int  plf_svc_port_from_node(int platform, fpi::FDSP_MgrIdType);
            static void plf_svc_uuid_from_node(const NodeUuid &, NodeUuid *, fpi::FDSP_MgrIdType);
            static void plf_svc_uuid_to_node(NodeUuid *, const NodeUuid &, fpi::FDSP_MgrIdType);

            /**
             * Return the base port and uuid of the service running in this node.
             */
            static int  plf_get_my_node_svc_uuid(fpi::SvcUuid *uuid, fpi::FDSP_MgrIdType);

            static inline int plf_get_my_am_svc_uuid(fpi::SvcUuid *uuid)
            {
                return plf_get_my_node_svc_uuid(uuid, fpi::FDSP_STOR_HVISOR);
            }

            static inline int plf_get_my_sm_svc_uuid(fpi::SvcUuid *uuid)
            {
                return plf_get_my_node_svc_uuid(uuid, fpi::FDSP_STOR_MGR);
            }

            static inline int plf_get_my_dm_svc_uuid(fpi::SvcUuid *uuid)
            {
                return plf_get_my_node_svc_uuid(uuid, fpi::FDSP_DATA_MGR);
            }

            static inline int plf_get_my_om_svc_uuid(fpi::SvcUuid *uuid)
            {
                return plf_get_my_node_svc_uuid(uuid, fpi::FDSP_ORCH_MGR);
            }

            /**
             * Platform methods.
             */
            bool plf_is_om_node();
            void plf_rpc_om_handshake(fpi::FDSP_RegisterNodeTypePtr pkt);
            void plf_change_info(const plat_node_data_t *ndata);

            /**
             * Platform node/cluster inventory methods.
             */
            virtual void plf_reg_node_info(const NodeUuid &uuid, const FdspNodeRegPtr msg);
            virtual void plf_del_node_info(const NodeUuid &uuid, const std::string &name);
            virtual void plf_update_cluster();

            /**
             * Module methods.
             */
            virtual int  mod_init(SysParams const *const param) override;
            virtual void mod_startup() override;
            virtual void mod_shutdown() override;
            virtual void mod_enable_service() override;

            inline void plf_set_my_base_port(fds_uint32_t port)
            {
                plf_my_base_port = port;
            }

            /**
             * Pull out common platform data.
             */
            inline fpi::FDSP_MgrIdType plf_get_node_type() const
            {
                return plf_node_type;
            }

            inline fds_uint32_t   plf_get_om_ctrl_port() const
            {
                return plf_om_ctrl_port;
            }

            inline fds_uint32_t   plf_get_om_svc_port()  const
            {
                return plf_om_svc_port;
            }

            inline fds_uint32_t   plf_get_my_base_port() const
            {
                return plf_my_base_port;
            }

            inline fds_uint32_t   plf_get_my_node_port() const
            {
                return plf_my_node_port;
            }

            inline fds_uint32_t   plf_get_my_ctrl_port(fds_uint32_t base = 0) const
            {
                return (base == 0 ? plf_my_base_port : base) + NET_SVC_CTRL;
            }

            inline fds_uint32_t   plf_get_my_conf_port(fds_uint32_t base = 0) const
            {
                return (base == 0 ? plf_my_base_port : base) + NET_SVC_CONFIG;
            }

            inline fds_uint32_t   plf_get_my_data_port(fds_uint32_t base = 0) const
            {
                return (base == 0 ? plf_my_base_port : base) + NET_SVC_DATA;
            }

            inline fds_uint32_t   plf_get_my_migration_port(fds_uint32_t base = 0) const
            {
                return (base == 0 ? plf_my_base_port : base) + NET_SVC_MIGRATION;
            }

            inline fds_uint32_t   plf_get_my_metasync_port(fds_uint32_t base = 0) const
            {
                return (base == 0 ? plf_my_base_port : base) + NET_SVC_META_SYNC;
            }

            inline std::string const *const plf_get_my_name() const
            {
                return &plf_my_node_name;
            }

            inline std::string const *const plf_get_my_ip() const
            {
                return &plf_my_ip;
            }

            inline std::string const *const plf_get_om_ip() const
            {
                return &plf_om_ip_str;
            }

            inline std::string const *const plf_get_auto_node_name() const
            {
                return &plf_my_auto_name;
            }

            inline OmAgent::pointer      plf_om_master() const
            {
                return plf_master;
            }

            inline PmAgent::pointer      plf_domain_ctrl() const
            {
                return plf_domain;
            }

            inline std::string const *const plf_node_fdsroot() const
            {
                return &pIf_node_fdsroot;
            }

            inline FlagsMap    & plf_get_flags_map()
            {
                return plf_flags_map;
            }

            /**
             * Common public factory methods.
             */
            virtual boost::intrusive_ptr<PmSvcEp> plat_new_pm_svc(NodeAgent::pointer,
                                                                  fds_uint32_t maj,
                                                                  fds_uint32_t min);

            virtual boost::intrusive_ptr<OmSvcEp> plat_new_om_svc(NodeAgent::pointer,
                                                                  fds_uint32_t maj,
                                                                  fds_uint32_t min);

            virtual boost::intrusive_ptr<SmSvcEp> plat_new_sm_svc(NodeAgent::pointer,
                                                                  fds_uint32_t maj,
                                                                  fds_uint32_t min);

            virtual boost::intrusive_ptr<DmSvcEp> plat_new_dm_svc(NodeAgent::pointer,
                                                                  fds_uint32_t maj,
                                                                  fds_uint32_t min);

            virtual boost::intrusive_ptr<AmSvcEp> plat_new_am_svc(NodeAgent::pointer,
                                                                  fds_uint32_t maj,
                                                                  fds_uint32_t min);

            virtual boost::shared_ptr<BaseAsyncSvcHandler> getBaseAsyncSvcHandler()
            {
                return nullptr;
            }

        protected:
            friend class PlatRpcReqt;
            friend class PlatRpcResp;

            fpi::FDSP_MgrIdType         plf_node_type;
            NodeUuid                    plf_my_uuid;           /**< this node HW uuid.         */
            NodeUuid                    plf_my_svc_uuid;       /**< SM/DM/AM... svc uuid.      */
            NodeUuid                    plf_my_plf_svc_uuid;   /**< platform service.          */
            std::string                 plf_my_node_name;      /**< user assigned node name.   */
            std::string                 plf_my_auto_name;      /**< domain assigned auto node. */

            std::string                         plf_my_ip;
            std::string                         plf_om_ip_str;
            std::string                         pIf_node_fdsroot;
            fds_uint32_t                        plf_om_ctrl_port;
            fds_uint32_t                        plf_om_svc_port;
            fds_uint32_t                        plf_my_base_port;
            fds_uint32_t                        plf_my_node_port;

            PmAgent::pointer                    plf_domain;
            OmAgent::pointer                    plf_master;
            DomainContainer::pointer            plf_node_inv;
            DomainClusterMap::pointer           plf_clus_map;
            DomainResources::pointer            plf_resources;

            /* Server attributes. */
            boost::shared_ptr<netSessionTbl>    plf_net_sess;

            /* Process wide flags */
            FlagsMap                            plf_flags_map;
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_PLATFORM_H_
