/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_LOCALCLUSTERMAP_H_
#define SOURCE_INCLUDE_LOCALCLUSTERMAP_H_

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
    // typedef boost::shared_ptr<netMigrationPathClientSession>
    //     NodeAgentCpSessionPtr;

    /**
     * Descriptor for a node in the cluster.
     * TODO(Andrew): Should use NodeAgent instead...
     */
    typedef struct _node_info_t {
        int node_id;
        unsigned int node_ip_address;
        fds_uint32_t port;
        FDS_ProtocolInterface::FDSP_NodeState node_state;
        NodeMigReqClientPtr nodeMigClient;
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
        LocalClusterMap();
        ~LocalClusterMap();

        /**
         * Returns node info
         */
        NodeUuid getNodeInfo() const;

        /**
         * Updates (add, rm, mod) a node in the
         * cluster map
         */
        Error addNode(const node_info_t &node);

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
    };

}  // namespace fds

#endif  // SOURCE_INCLUDE_LOCALCLUSTERMAP_H_
