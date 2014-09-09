/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 * Implementation of various tier put algorithms.
 */
#include <TierPutAlgorithms.h>

/******* RankTierPutAlgo  implementation ******/

/* 
 * Selects tier based on rank (very simple)
 *  'all ssd' -- selects ssd
 *  'all disk' -- selects disk
 *  'hybrid' -- 
 *       1) gets rank from ranking engine
 *       2) gets the rank of the lowest ranked object in the rank table,
 *          the ranking engine will return the lowest possible rank if 
 *          rank table has free space (inserting this obj will not kick
 *          anyone out
 *       3) if rank of 'oid' is greater the lowest rank in the rank table
 *          then put to SSD, otherwise put to disk      
 *
 */
diskio::DataTier RankTierPutAlgo::selectTier(const ObjectID& oid,
                                             fds_volid_t volid)
{
    diskio::DataTier ret_tier = diskio::diskTier;
    FDSP_MediaPolicy media_policy;
    fds_uint32_t rank;
    StorMgrVolume* vol = sm_volTbl->getVolume(volid);
    fds_verify(vol != NULL);
    fds_verify(vol->voldesc != NULL);

    media_policy = vol->voldesc->mediaPolicy;

    if (media_policy == FDSP_MEDIA_POLICY_SSD) {
        /* if 'all ssd', put to ssd */
        ret_tier = diskio::flashTier;
    } else if ((media_policy == FDSP_MEDIA_POLICY_HDD) ||
             (media_policy == FDSP_MEDIA_POLICY_HYBRID_PREFCAP)) {
        /* if 'all disk', put to disk 
         * or if hybrid but first preference to capacity tier, put to disk  */
        ret_tier = diskio::diskTier;
    } else if (media_policy == FDSP_MEDIA_POLICY_HYBRID) {
        /* hybrid tier policy */
        fds_uint32_t rank = rank_eng->getRank(oid, *vol->voldesc);
        if (rank < rank_eng->getTblTailRank()) {
            /* lower value means higher rank */
            ret_tier = diskio::flashTier;
        }
    } else {  // else ret_tier already set to disk
        FDS_PLOG(tpa_log)
                << "RankTierPutAlgo: selectTier received unexpected media policy: " << media_policy;
    }

    return ret_tier;
}
