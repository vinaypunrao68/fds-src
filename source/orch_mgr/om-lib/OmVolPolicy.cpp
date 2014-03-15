/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <string>
#include <OmVolPolicy.hpp>
#include <fds_process.h>

namespace fds {

VolPolicyMgr::VolPolicyMgr(kvstore::ConfigDB* configDB, fds_log* om_log) {
    SetLog(om_log);
    this->configDB = configDB;
    if (!configDB->isConnected()) {
        LOGWARN << "unable to talk to configdb";
    }
}

VolPolicyMgr::~VolPolicyMgr() {
}

void VolPolicyMgr::copyPolInfoToFdsPolicy(
    FDS_VolumePolicy& fdsPolicy,
    const fpi::FDSP_PolicyInfoType& pol_info)
{
    fdsPolicy.volPolicyId = pol_info.policy_id;
    fdsPolicy.volPolicyName = pol_info.policy_name;
    fdsPolicy.iops_max = pol_info.iops_max;
    fdsPolicy.iops_min = pol_info.iops_min;
    fdsPolicy.thruput = 0; /* are we still going to use this? */
    fdsPolicy.relativePrio = pol_info.rel_prio;
}

Error VolPolicyMgr::updateCatalog(
    int policy_id,
    const fpi::FDSP_PolicyInfoType& pol_info)
{
    Error err(ERR_OK);
    FDS_VolumePolicy policy;
    copyPolInfoToFdsPolicy(policy, pol_info);

    if (!configDB->addPolicy(policy)) {
        err = ERR_CAT_QUERY_FAILED;
    }

    return err;
}


Error VolPolicyMgr::createPolicy(const fpi::FDSP_PolicyInfoType& pol_info) {
    Error err(ERR_OK);
    int polid = pol_info.policy_id;

    LOGNORMAL << "VolPolicyMgr::createPolicy -- will add policy to the catalog ";

    /* check if we already have this policy in the catalog */
    FDS_VolumePolicy volPolicy;
    err = queryPolicy(pol_info.policy_id, &volPolicy);
    if (err == ERR_NOT_FOUND) {
        /* write new policy to catalog */
        err = updateCatalog(polid, pol_info);
        LOGNORMAL << "added policy [" << polid << "] to policy catalog";
    } else if (err.ok()) {
        LOGWARN << "policy [" << polid << "] already exists";
    } else {
        LOGERROR << "failed to access policy catalog to add policy " << polid;
    }

    return err;
}

Error VolPolicyMgr::queryPolicy(int policy_id, FDS_VolumePolicy *volPolicy) {
    Error err(ERR_OK);
    assert(volPolicy);

    /* query policy from the catalog */
    if (configDB->getPolicy(policy_id, *volPolicy)) {
        LOGNORMAL << *volPolicy;
    } else {
        LOGWARN << "policy not found : " << policy_id;
        err = ERR_NOT_FOUND;
    }

    return err;
}

Error VolPolicyMgr::fillVolumeDescPolicy(VolumeDesc* voldesc)
{
    Error err(ERR_OK);
    FDS_VolumePolicy policy;

    LOGNORMAL << "VolPolicyMgr::fillVolimeDescPolicy: start";

    assert(voldesc);
    err = queryPolicy(voldesc->volPolicyId, &policy);
    if (err.ok()) {
        voldesc->iops_min = policy.iops_min;
        voldesc->iops_max = policy.iops_max;
        voldesc->relativePrio = policy.relativePrio;
    }

    return err;
}

Error VolPolicyMgr::modifyPolicy(const fpi::FDSP_PolicyInfoType& pol_info) {
    Error err(ERR_OK);
    FDS_VolumePolicy volPolicy;

    /* first get policy record to see if it exists */
    if (configDB->getPolicy(pol_info.policy_id, volPolicy)) {
        /* we will just over-write the policy record with pol_info */
        err = updateCatalog(pol_info.policy_id, pol_info);
    }

    if (err.ok()) {
        LOGNORMAL << "VolPolicyMgr::modifyPolicy -- Modified policy "
                  << pol_info.policy_id;
    } else {
        LOGNORMAL << "VolPolicyMgr::modifyPolicy -- did not modify policy "
                  << pol_info.policy_id << "; error: " << err.GetErrstr();
    }

    return err;
}

/* Removes policy from policy catalog */
Error VolPolicyMgr::deletePolicy(int policy_id,
                                 const std::string& policy_name)
{
    Error err(ERR_OK);

    if (!configDB->deletePolicy(policy_id)) {
        LOGERROR << "unable to delete policy [" << policy_id <<"]";
        err = ERR_INVALID_ARG;
    }

    return err;
}



} /* namespace fds */
