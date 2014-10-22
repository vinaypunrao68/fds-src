/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_TIERPUTALGORITHMS_H_
#define SOURCE_STOR_MGR_INCLUDE_TIERPUTALGORITHMS_H_

#include <TierEngine.h>

namespace fds {

/*
 * A test algorithm that just selects a random tier.
 * Obviously not practical, but useful for testing.
 */
class RandomTestAlgo : public TierPutAlgo {
    public:
     diskio::DataTier selectTier(const ObjectID &oid,
                                 fds_volid_t     vol) {
         if ((::random() % 2) == 0) {
             return diskio::flashTier;
         }
         return diskio::diskTier;
     }
};

/*
 * Very simple algorithm based on tier policy.
 *
 * Policy:
 * 'all ssd' -- puts to SSD
 * 'all disk' -- puts to disk
 * 'hybrid' -- will put to SSD if either:
 *       1) Rank table is not full      
 *       2) The rank of an object with the lowest rank
 *       in the rank table is lower than the rank of 
 *       of this object (we know that this object can 
 *       kick the lowest-rank object out of the table)
 */
class RankTierPutAlgo: public TierPutAlgo {
    public:
     RankTierPutAlgo(StorMgrVolumeTable* _sm_volTbl,
                     ObjectRankEngine* _rank_eng)
             : rank_eng(_rank_eng),
               sm_volTbl(_sm_volTbl) {
         }
     ~RankTierPutAlgo() {
     }

     diskio::DataTier selectTier(const ObjectID &oid,
                                 fds_volid_t volid);

    private:
     /* does not own, gets passed from SM */
     ObjectRankEngine* rank_eng;

     /* does not own, gets passed from SM */
     StorMgrVolumeTable* sm_volTbl;
};

}  // namespace fds
#endif  // SOURCE_STOR_MGR_INCLUDE_TIERPUTALGORITHMS_H_
