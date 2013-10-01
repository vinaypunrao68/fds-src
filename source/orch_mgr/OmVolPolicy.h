#ifndef ORCH_MGR_POLICY_H_
#define ORCH_MGR_POLICY_H_

#include <Ice/Ice.h>
#include <unordered_map>
#include <string>

#include "include/fds_volume.h"
#include "fdsp/FDSP.h"
#include "util/Log.h"
#include "lib/Catalog.h"

namespace fds {

typedef FDS_ProtocolInterface::FDSP_PolicyInfoTypePtr FdspPolInfoPtr;

/* Volume policy catalog, etc  */
class VolPolicyMgr
{
 public:
  VolPolicyMgr(const std::string& om_prefix, fds_log* om_log);
  ~VolPolicyMgr();

  /* create new policy
   * returns 'invalid argument' error if policy already exists */
  Error createPolicy(const FdspPolInfoPtr& pol_info);

  /* modify existing policy 
   * returns 'invalid argument' error if policy does not exist */
  Error modifyPolicy(const FdspPolInfoPtr& pol_info);

  /* delete policy if policy exists 
   * no error if policy does not exist  */
  Error deletePolicy(int policy_id, const std::string& policy_name);

  /* for now just prints policy to log 
   * TODO: probably should query by name? return FDS_VolumePolicy object */
  Error queryPolicy(int policy_id);

 private: /* methods */
  void copyPolInfoToFdsPolicy(FDS_VolumePolicy& fdsPolicy, const FdspPolInfoPtr& pol_info);
  Error updateCatalog(int policy_id, const FdspPolInfoPtr& pol_info);

 private:
  Catalog* policy_catalog;
 
  /* no locks here for now, assumes that OrchMgr holds om_mutex lock
   *  when any of the methods accessing/modifying policy_map are called */

  /* parent log */
  fds_log* parent_log;
};


} /* namespace fds */

#endif /* ORCH_MGR_POLICY_H */
