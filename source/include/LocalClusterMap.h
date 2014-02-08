/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_LOCALCLUSTERMAP_H_
#define SOURCE_INCLUDE_LOCALCLUSTERMAP_H_

#include <string>
#include <unordered_map>

#include <fdsp/FDSP_types.h>
#include <fds_typedefs.h>
#include <fds_err.h>
#include <fds_module.h>
#include <NetSession.h>
#include <concurrency/RwLock.h>

namespace fds {

    typedef boost::shared_ptr<FDS_ProtocolInterface::FDSP_MigrationPathReqClient>
            NodeMigReqClientPtr;
    typedef boost::shared_ptr<netMigrationPathClientSession>
            NodeMigSessionPtr;

    /**
     * Descriptor for a node in the cluster.
     * TODO(Andrew): Should use NodeAgent instead...
     */
    typedef struct _node_info_t {
        int node_id;
        unsigned int node_ip_address;
        fds_uint32_t port;
        fds_uint32_t mig_port;  /**< Port for migration services */
        FDS_ProtocolInterface::FDSP_NodeState node_state;
        NodeMigSessionPtr   ndMigSession;
        std::string         ndMigSessionId;
        NodeMigReqClientPtr ndMigClient;
    } node_info_t;

    typedef std::unordered_map<int, node_info_t> node_map_t;

    /**
     * Defines the current state of members in the cluster
     * The map is 'local' because it is a read only copy of
     * the authority map that is maintained by the OM.
     * The map provides access to nodes in the cluster and
     * their communication end points.
     */
    class LocalClusterMap : public Module {
  public:
        LocalClusterMap(boost::shared_ptr<FDS_ProtocolInterface::
                        FDSP_MigrationPathRespIf> migRespHndlr);
        ~LocalClusterMap();

        /**
         * Returns node info
         */
        NodeUuid getNodeInfo() const;

        /**
         * Return a migration interface client to the
         * specific node.
         */
        NodeMigReqClientPtr getMigClient(fds_uint64_t node_id);

        /**
         * Updates (add, rm, mod) a node in the
         * cluster map.
         * Creates an endpoint and stashes it in
         * the node.
         */
        Error addNode(node_info_t *node);

        /**
         * Module methods.
         */
        virtual int  mod_init(SysParams const *const param);
        virtual void mod_startup();
        virtual void mod_shutdown();

  private:
        fds_rwlock   lcmLock;
        fds_uint64_t version;
        Sha1Digest   checksum;

        node_map_t clusterMembers;

        // TODO(Andrew): The netsession table probably doesn't
        // belong in this class, but we can refactor later when
        // we start locally versioning this info...
        boost::shared_ptr<netSessionTbl> lcmSessTbl;

        /**
         * Local response handlers that are provided by the
         * user of this class... They may be NULL if the
         * owner doesn't want to handle an interface.
         */
        /**
         * Provides SM to SM interface for migration
         */
        boost::shared_ptr<FDS_ProtocolInterface::
                FDSP_MigrationPathRespIf> migRspHndlr;
    };

}  // namespace fds

#endif  // SOURCE_INCLUDE_LOCALCLUSTERMAP_H_
