#ifndef ORCH_MGR_POLICY_H_
#define ORCH_MGR_POLICY_H_

#include <unordered_map>
#include <string>

#include <fds_volume.h>
#include <fdsp/FDSP_types.h>
#include <util/Log.h>

/* TODO: move to include dir if this is the top level API header file. */
#include <lib/Catalog.h>

namespace fds {

/* Volume policy catalog, etc  */
class VolPolicyMgr {
  public:
    VolPolicyMgr(const std::string& om_prefix, fds_log* om_log);
    ~VolPolicyMgr();

    /* create new policy
     * returns 'invalid argument' error if policy already exists */
    Error createPolicy(
        const FDS_ProtocolInterface::FDSP_PolicyInfoType& pol_info);

    /* modify existing policy 
     * returns 'invalid argument' error if policy does not exist */
    Error modifyPolicy(
        const FDS_ProtocolInterface::FDSP_PolicyInfoType& pol_info);

    /* delete policy if policy exists 
     * no error if policy does not exist  */
    Error deletePolicy(int policy_id, const std::string& policy_name);

    /* queries volume policy from the catalog and returns in 'ret_policy' */
    Error queryPolicy(int policy_id, FDS_VolumePolicy *ret_policy);

    /* queries volume policy specified in 'voldesc' from the catalog and 
     * fills in policy fields in 'voldesc' */
    Error fillVolumeDescPolicy(VolumeDesc* voldesc);

  private: /* methods */
    void copyPolInfoToFdsPolicy(
        FDS_VolumePolicy& fdsPolicy,
        const FDS_ProtocolInterface::FDSP_PolicyInfoType& pol_info);
    Error updateCatalog(
        int policy_id,
        const FDS_ProtocolInterface::FDSP_PolicyInfoType& pol_info);

  private:
    Catalog* policy_catalog;
 
    /* no locks here for now, assumes that OrchMgr holds om_mutex lock
     *  when any of the methods accessing/modifying policy_map are called */

    /* parent log */
    fds_log* parent_log;
};


} /* namespace fds */

#endif /* ORCH_MGR_POLICY_H */
