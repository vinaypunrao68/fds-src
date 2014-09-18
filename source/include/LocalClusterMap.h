/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_LOCALCLUSTERMAP_H_
#define SOURCE_INCLUDE_LOCALCLUSTERMAP_H_

#include <string>
#include <unordered_map>

#include <fdsp/FDSP_types.h>
#include <fds_typedefs.h>
#include <fds_error.h>
#include <fds_module.h>
#include <concurrency/RwLock.h>

/*  Need to remove netsession stuffs! */
// Forward declarations
namespace FDS_ProtocolInterface {
    class FDSP_MigrationPathReqClient;
    class FDSP_MigrationPathRespProcessor;
    class FDSP_MigrationPathRespIf;
}  // namespace FDS_ProtocolInterface

namespace bo  = boost;
namespace fpi = FDS_ProtocolInterface;

class netSessionTbl;
template <class A, class B, class C> class netClientSessionEx;
typedef netClientSessionEx<fpi::FDSP_MigrationPathReqClient,
                fpi::FDSP_MigrationPathRespProcessor,
                fpi::FDSP_MigrationPathRespIf> netMigrationPathClientSession;

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
        fds_uint64_t node_id;
        unsigned int node_ip_address;
        fds_uint32_t port;
        fds_uint32_t mig_port;  /**< Port for migration services */
        fds_uint32_t meta_sync_port;  /**< Port for meta sync  services */
        FDS_ProtocolInterface::FDSP_NodeState node_state;
        NodeMigSessionPtr   ndMigSession;
        std::string         ndMigSessionId;
        NodeMigReqClientPtr ndMigClient;
    } node_info_t;

    typedef std::unordered_map<fds_uint64_t, node_info_t> node_map_t;

    /**
     * Defines the current state of members in the cluster
     * The map is 'local' because it is a read only copy of
     * the authority map that is maintained by the OM.
     * The map provides access to nodes in the cluster and
     * their communication end points.
     */
    class LocalClusterMap : public Module {
  public:
        LocalClusterMap();
        ~LocalClusterMap();

        /**
         * Returns node info
         * TODO(Andrew): Use NodeUuid not uin64 directly
         */
        int getNodeInfo(fds_uint64_t nodeUuid,
                        fds_uint32_t *nodeIpAddr,
                        fds_uint32_t *nodePort,
                        int *nodeState);

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
        Error addNode(node_info_t *node,
                      fpi::FDSP_MgrIdType myRole,
                      fpi::FDSP_MgrIdType nodeRole);

        fds_uint32_t getNodeMigPort(NodeUuid uuid);
        fds_uint32_t getNodeMetaSyncPort(NodeUuid uuid);

        /**
         * Module methods.
         */
        virtual int  mod_init(SysParams const *const param);
        virtual void mod_startup();
        virtual void mod_shutdown();

  private:
        fds_rwlock   lcmLock;
        fds_uint64_t version;
        // TODO(Andrew): Add a checksum field

        node_map_t clusterMembers;
    };

}  // namespace fds

#endif  // SOURCE_INCLUDE_LOCALCLUSTERMAP_H_
