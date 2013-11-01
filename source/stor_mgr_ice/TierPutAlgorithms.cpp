/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 * Implementation of various tier put algorithms.
 */
#include "TierPutAlgorithms.h"


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
  FDSP_VolType vol_type;
  fds_uint32_t rank;
  StorMgrVolume* vol = sm_volTbl->getVolume(volid);
  fds_verify(vol != NULL);
  fds_verify(vol->voldesc != NULL);

  vol_type = vol->voldesc->volType;

  if (vol_type == FDSP_VOL_BLKDEV_SSD_TYPE) {
    /* if 'all ssd', put to ssd */
    ret_tier = diskio::flashTier;
  }
  else if (vol_type == FDSP_VOL_BLKDEV_DISK_TYPE) {
    /* if 'all disk', put to disk */
    ret_tier = diskio::diskTier;
  }
  else if (vol_type == FDSP_VOL_BLKDEV_HYBRID_TYPE) {
    /* hybrid tier policy */
    fds_uint32_t rank = rank_eng->getRank(oid, *vol->voldesc);
    if (rank < rank_eng->getTblTailRank()) {
      /* lower value means higher rank */
      ret_tier = diskio::flashTier;
    }
    // else ret_tier already set to disk 
  }
  else {
    FDS_PLOG(tpa_log) << "RankTierPutAlgo: selectTier received unexpected volume type: " << vol_type;
  }

  return ret_tier;
}
