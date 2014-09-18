/* Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_CLUSTERCOMMMGR_H_
#define SOURCE_INCLUDE_CLUSTERCOMMMGR_H_

#include <unordered_map>
#include <set>
#include <boost/shared_ptr.hpp>

#include <OMgrClient.h>

class netSessionTbl;
typedef boost::shared_ptr<netSessionTbl> netSessionTblPtr;

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
    ClusterCommMgr(OMgrClient *omClient, netSessionTblPtr nst);

    TVIRTUAL NodeTokenTbl partition_tokens_by_node(const std::set<fds_token_id> &tokens);

    TVIRTUAL bool get_node_mig_ip_port(const fds_token_id &token_id,
                                      uint32_t &ip,
                                      uint32_t &port);
    TVIRTUAL const DLT* get_dlt();

    OMgrClient* get_om_client();

    netSessionTblPtr get_nst();

    template <class Type>
    boost::shared_ptr<Type> get_client_session(const NodeUuid &node_id);

    // netMigrationPathClientSession* get_migration_session(const NodeUuid &node_id);
 protected:
    // TODO(Rao): We shouldn't rely on omClient.  We should just have a
    // reference to DLT and Cluster map
    OMgrClient         *om_client_;
    netSessionTblPtr    nst_;
};

typedef boost::shared_ptr<ClusterCommMgr> ClusterCommMgrPtr;

}  // namespace fds

#endif  // SOURCE_INCLUDE_CLUSTERCOMMMGR_H_
