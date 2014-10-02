#ifndef ORCH_MGR_POLICY_H_
#define ORCH_MGR_POLICY_H_

#include <unordered_map>
#include <string>

#include <fds_volume.h>
#include <fds_typedefs.h>
#include <util/Log.h>
#include <kvstore/configdb.h>
/* TODO: move to include dir if this is the top level API header file. */
// #include <lib/Catalog.h>

namespace fds {

/* Volume policy catalog, etc  */
class VolPolicyMgr : public HasLogger {
  public:
    VolPolicyMgr(kvstore::ConfigDB* configDB,fds_log* om_log);
    ~VolPolicyMgr();

    /* create new policy
     * returns 'invalid argument' error if policy already exists */
    Error createPolicy(const fpi::FDSP_PolicyInfoType& pol_info);

    /* modify existing policy 
     * returns 'invalid argument' error if policy does not exist */
    Error modifyPolicy(const fpi::FDSP_PolicyInfoType& pol_info);

    /* delete policy if policy exists 
     * no error if policy does not exist  */
    Error deletePolicy(int policy_id, const std::string& policy_name);

    /* queries volume policy from the catalog and returns in 'ret_policy' */
    Error queryPolicy(int policy_id, FDS_VolumePolicy *ret_policy);

    /* queries volume policy specified in 'voldesc' from the catalog and 
     * fills in policy fields in 'voldesc' */
    Error fillVolumeDescPolicy(VolumeDesc* voldesc);

  private: /* methods */
    void copyPolInfoToFdsPolicy(FDS_VolumePolicy& fdsPolicy, const fpi::FDSP_PolicyInfoType& pol_info); //NOLINT
    Error updateCatalog(int policy_id, const fpi::FDSP_PolicyInfoType& pol_info); //NOLINT

  private:
    kvstore::ConfigDB* configDB;
};


} /* namespace fds */

#endif /* ORCH_MGR_POLICY_H */
