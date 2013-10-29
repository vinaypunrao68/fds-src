/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 * Rank engine implementation
 */

#include <include/ObjRank.h>

namespace fds {

ObjectRankEngine::ObjectRankEngine(const std::string& _sm_prefix, fds_uint32_t _tbl_size, fds_log* log)
  : rank_tbl_size(_tbl_size), 
    cur_rank_tbl_size(0),
    max_candidate_size(MAX_RANK_CACHE_SIZE),
    ranklog(log)
{
  std::string filename(_sm_prefix + "ObjRankDB");

  rankingEnabled = ATOMIC_VAR_INIT(false);

  FDS_PLOG(ranklog) << "ObjectRankEngine: construction done, rank table size = " << rank_tbl_size;
}

ObjectRankEngine::~ObjectRankEngine()
{

}

fds_uint32_t ObjectRankEngine::rankAndInsertObject(const ObjectID& objId, const VolumeDesc& voldesc)
{
  fds_uint32_t rank = getRank(objId, voldesc);

  /* is it possible that object may already exist? if so, what is the expected behavior? */
  /* current implementation will ignore insertion of duplicate, that object id will be ranked
  * as if it was inserted now/previosly once */

  maps_rwlock.write_lock();

  /* 'all disk' volumes go directly to 'demotion' */
  if (rank == OBJECT_RANK_ALL_DISK) {
    rankDeltaChgTbl[objId] = OBJ_RANK_DEMOTION;
  }
  else if (rank == OBJECT_RANK_ALL_SSD) {
    /* 'all ssd' volumes will go directly to 'promotion', assuming
     * we don't have the case that the sum of all objects from 
     * 'all ssd' volumes are greater than rank table
     * TODO: handle this case later ... */
    rankDeltaChgTbl[objId] = OBJ_RANK_PROMOTION;
  }
  else {
    /* otherwise, this is a candidate for promotion */
    candidate_map[objId] = rank;

    if (candidate_map.size() >= max_candidate_size) {
      maps_rwlock.write_unlock();
      startObjRanking();
      maps_rwlock.write_lock();
    }
  }

  maps_rwlock.write_unlock();

  return rank;
}

void ObjectRankEngine::deleteObject(const ObjectID& objId)
{
  maps_rwlock.write_lock();

  /* first delete from candidate list if object is there */
  if (candidate_map.count(objId) > 0) {
    candidate_map.erase(objId);
  }

  /* this object goes for demotion */
  rankDeltaChgTbl[objId] = OBJ_RANK_DEMOTION;

  /* if objects is in rank table in persistent storage, we will
   * delete the object from there during ranking process */

  maps_rwlock.write_unlock();
}

fds_uint32_t ObjectRankEngine::getRank(const ObjectID& objId, const VolumeDesc& voldesc)
{
  fds_uint32_t rank = 0;
  /* TODO: first check if volume is 'all disk' or 'all ssd' -- we don't have this policy yet */
  rank = ( ( getObjectSubrank(objId) << 16 ) & 0xFFFF0000 ) + getVolumeRank(voldesc);
  return rank;
}

fds_bool_t ObjectRankEngine::rankingInProgress()
{
  return atomic_load(&rankingEnabled);
}

/* start ranking process */
void ObjectRankEngine::startObjRanking()
{
  fds_bool_t was_enabled = atomic_exchange(&rankingEnabled, true);
  if (!was_enabled)
    {
      /* start the ranking thread */
      FDS_PLOG(ranklog) << "ObjectRankEngine: Starting ranking process in a background thread";
    }

}

void ObjectRankEngine::stopObjRanking()
{
  fds_bool_t was_enabled = atomic_exchange(&rankingEnabled, false);
  if (was_enabled)
    {
      FDS_PLOG(ranklog) << "ObjectRankEngine: Stopping ranking process before it finished";

      /* should we keep the state if we stop in the middle so we start from 
       * where we left off or we will start from the beginning? */ 
    }
 
}

void ObjectRankEngine::getRankDeltaChangeTable(rank_delta_table_t *tbl)
{
}

void ObjectRankEngine::persistRankTbl()
{
}

fds_bool_t getNextTblSegment(fds_uint32_t num_objs, ObjectID *objIdArray)
{
  return false;
}

} // namespace fds



