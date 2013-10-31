/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 * Implementation of various tier put algorithms.
 */
#include "TierPutAlgorithms.h"


/******* PolicyTierPutAlgo  implementation ******/

/* 
 * Selects tier based on policy (very simple)
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
diskio::DataTier PolicyTierPutAlgo::selectTier(const ObjectID& oid,
					       fds_volid_t volid)
{
  return diskio::diskTier;
}
