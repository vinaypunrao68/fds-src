/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include "OmVolPolicy.h"

namespace fds {


VolPolicyMgr::VolPolicyMgr(fds_log* om_log)
  :parent_log(om_log)
{
  const std::string catname("volpolicy_cat.ldb");
  policy_catalog = new Catalog(catname);
}

VolPolicyMgr::~VolPolicyMgr()
{
  delete policy_catalog;
}

void VolPolicyMgr::copyPolInfoToFdsPolicy(FDS_VolumePolicy& fdsPolicy, const FdspPolInfoPtr& pol_info)
{
  fdsPolicy.volPolicyId = pol_info->policy_id;
  fdsPolicy.volPolicyName = pol_info->policy_name;
  fdsPolicy.iops_max = pol_info->iops_max;
  fdsPolicy.iops_min = pol_info->iops_min;
  fdsPolicy.thruput = 0; /* are we still going to use this? */
  fdsPolicy.relativePrio = pol_info->rel_prio;
}

Error VolPolicyMgr::updateCatalog(int policy_id, const FdspPolInfoPtr& pol_info)
{
  Error err(ERR_OK);
  Record key((const char *)&policy_id, sizeof(policy_id));

  FDS_VolumePolicy policy;
  copyPolInfoToFdsPolicy(policy, pol_info);
  std::string policy_string = policy.toString();
  Record val(policy_string);

  err = policy_catalog->Update(key, val);

  return err;
}


Error VolPolicyMgr::createPolicy(const FdspPolInfoPtr& pol_info)
{
  Error err(ERR_OK);
  int polid = pol_info->policy_id;
  std::string val;

  FDS_PLOG(parent_log) << "VolPolicyMgr::createPolicy -- will add policy to the catalog ";

  /* check if we already have this policy in the catalog */
  Record key((const char *)&polid, sizeof(polid));
  err = policy_catalog->Query(key, &val);
  if ( err == ERR_CAT_ENTRY_NOT_FOUND)
    {
      /* write new policy to catalog */
      err = updateCatalog(polid, pol_info);
      FDS_PLOG(parent_log) << "VolPolicyMgr::createPolicy -- added policy " << polid << " to policy catalog";
    }
  else if (err.ok())
    {
      FDS_PLOG(parent_log) << "VolPolicyMgr::createPolicy -- policy " << polid << " already exists";
    }
  else 
    {
      FDS_PLOG(parent_log) << "VolPolicyMgt::createPolicy -- failed to access policy catalog to add policy " << polid;
    }

  /* test */
  queryPolicy(polid);

  return err;
}

Error VolPolicyMgr::queryPolicy(int policy_id)
{
  Error err(ERR_OK);
  std::string val;
  Record key((const char *)&policy_id, sizeof(policy_id));

  err = policy_catalog->Query(key, &val);
  if ( err.ok())
    {
      FDS_VolumePolicy policy;
      err = policy.setPolicy(val.c_str(), val.length());
      if (err.ok()) {
	FDS_PLOG(parent_log) << "VolPolicyMgr::queryPolicy -- policy id " << policy_id
			     << "; name " << policy.volPolicyName
			     << "; iops_min " << policy.iops_min
			     << "; iops_max " << policy.iops_max
			     << "; rel_prio " << policy.relativePrio;
      }
    }

  FDS_PLOG(parent_log) << "VolPolicyMgr::queryPolicy -- " << err.GetErrstr();

  return err;
}


Error VolPolicyMgr::modifyPolicy(const FdspPolInfoPtr& pol_info)
{
  Error err(ERR_OK);
  int polid = pol_info->policy_id;
  Record key((const char *)&polid, sizeof(polid));
  std::string val;

  /* test*/
  queryPolicy(polid);

  /* first get policy record to see if it exists */
  err = policy_catalog->Query(key, &val);
  if ( err.ok())
    {
      /* we will just over-write the policy record with pol_info */
      err = updateCatalog(polid, pol_info);
    }
  
  if ( err.ok())
    {
      FDS_PLOG(parent_log) << "VolPolicyMgr::modifyPolicy -- Modified policy " << polid;
    }
  else 
    {
      FDS_PLOG(parent_log) << "VolPolicyMgr::modifyPolicy -- did not modify policy " 
			   << polid << "; error: " << err.GetErrstr();
    }
    
  /* test */
  queryPolicy(polid);

  return err;
}

/* Removes policy from policy catalog 
 * If policy does not exist in the policy catalog, no error */
Error VolPolicyMgr::deletePolicy(int policy_id, const std::string& policy_name)
{
  Error err(ERR_OK);
  Record key((const char *)&policy_id, sizeof(policy_id));

  /* test */
  queryPolicy(policy_id);

  err = policy_catalog->Delete(key); 

  FDS_PLOG(parent_log) << "VolPolicyMgr::deletePolicy -- policy " << policy_id
		       << ", result " << err.GetErrstr();

  return err;
}



} /* namespace fds */
