/* Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_CLUSTERCOMMMGR_H_
#define SOURCE_INCLUDE_CLUSTERCOMMMGR_H_

#include <unordered_map>
#include <set>
#include <boost/shared_ptr.hpp>

#include <OMgrClient.h>

namespace fds {

typedef std::unordered_map<NodeUuid, std::set<fds_token_id>, UuidHash> NodeTokenTbl;

/**
 * @brief Cluster communication interface. Use this class for accessing
 * Node service endpoints such as datapath, control path, etc.
 * NOTE: Some of the functions below are marked virtual though they don't
 * need to be to allow for creating mock objects
 */
class ClusterCommMgr {
 public:
    explicit ClusterCommMgr(OMgrClient *omClient);

    virtual NodeTokenTbl partition_tokens_by_node(const std::set<fds_token_id> &tokens);

    virtual bool get_node_mig_ip_port(const NodeUuid &node_id,
                                      uint32_t &ip,
                                      uint32_t &port);

    // netMigrationPathClientSession* get_migration_session(const NodeUuid &node_id);
 protected:
    // TODO(Rao): We shouldn't rely on omClient.  We should just have a
    // reference to DLT and Cluster map
    OMgrClient         *omClient_;
};

typedef boost::shared_ptr<ClusterCommMgr> ClusterCommMgrPtr;

}  // namespace fds

#endif  // SOURCE_INCLUDE_CLUSTERCOMMMGR_H_
