/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 * Rank engine implementation
 */

#include <include/ObjRank.h>

namespace fds {

ObjectRankEngine::ObjectRankEngine(const std::string& _sm_prefix, fds_uint32_t _tbl_size, ObjStatsTracker *_obj_stats, fds_log* log)
  : rank_tbl_size(_tbl_size), 
    cur_rank_tbl_size(0),
    max_cached_objects(MAX_RANK_CACHE_SIZE),
    tail_rank(OBJECT_RANK_INVALID),
    obj_stats(_obj_stats),
    ranklog(log)
{
  std::string filename(_sm_prefix + "ObjRankDB");
  rankDB = new Catalog(filename);

  map_mutex = new fds_mutex("RankEngineMutex");
  tbl_mutex = new fds_mutex("RankEngineChgTblMutex");

  rankingEnabled = ATOMIC_VAR_INIT(false);
  exiting = false;

  rank_thread = new boost::thread(boost::bind(&runRankingThread, this));
  rank_notify = new fds_notification();

  FDS_PLOG(ranklog) << "ObjectRankEngine: construction done, rank table size = " << rank_tbl_size;
}

ObjectRankEngine::~ObjectRankEngine()
{
  exiting = true;

  /* make sure ranking thread is not waiting  */
  rank_notify->notify();

  /* wait for ranking thread to finish */
  rank_thread->join();

  delete rank_thread;
  delete rank_notify;
  delete map_mutex;
  delete tbl_mutex;

  if (rankDB) {
    delete rankDB;
  }
}

fds_uint32_t ObjectRankEngine::rankAndInsertObject(const ObjectID& objId, const VolumeDesc& voldesc)
{
  fds_uint32_t rank = getRank(objId, voldesc);
  fds_bool_t start_ranking_process = false;

  /* is it possible that object may already exist? if so, what is the expected behavior? */
  /* current implementation works as if insert of existing object is an update, eg. of 
   * existing object has 'all disk' policy but we insert same object with 'all ssd' policy,
   * the object rank will reflect 'all ssd' policy */

  map_mutex->lock();

  cached_obj_map[objId] = rank;
  if (cached_obj_map.size() >= max_cached_objects) {
    start_ranking_process = true;
  }

  /* note that rank already encapsulates volume policy, so no need to keep volume desc anymore */ 

  map_mutex->unlock();

  if (start_ranking_process) {
    startObjRanking();
  }

  return rank;
}

void ObjectRankEngine::deleteObject(const ObjectID& objId)
{
  fds_bool_t start_ranking_process = false;
  fds_bool_t do_cache_obj = true;

  tbl_mutex->lock();
  {
    /* First see if the obj is in delta change table and delete from there -- 
     * so we don't require migrator again check obj for validity */
    obj_rank_cache_it_t it = rankDeltaChgTbl.find(objId);
    if (it != rankDeltaChgTbl.end()) {
      fds_uint32_t rank = it->second;
      rankDeltaChgTbl.erase(it);
      if (isRankDemotion(rank)) {
	/* this obj not in rank table, so nothing else to do */
	do_cache_obj = false;
      }
    }
  }
  tbl_mutex->unlock();

  if (!do_cache_obj) return;

  /* we need to cache this deletion to erase it (if it's there) from 
   * the ranking table during the next ranking process */
  map_mutex->lock();
  {
    cached_obj_map[objId] = OBJECT_RANK_INVALID;
    if (cached_obj_map.size() >= max_cached_objects) {
      start_ranking_process = true;
    }
  }
  map_mutex->unlock();

  if (start_ranking_process) {
    startObjRanking();
  }

}

fds_uint32_t ObjectRankEngine::getRank(const ObjectID& objId, const VolumeDesc& voldesc)
{
  fds_uint32_t rank = 0;
  if (voldesc.volType == FDSP_VOL_BLKDEV_DISK_TYPE)
    return OBJECT_RANK_ALL_DISK;
  if (voldesc.volType == FDSP_VOL_BLKDEV_SSD_TYPE)
    return OBJECT_RANK_ALL_SSD;

  rank = ( ( getObjectSubrank(objId) << 16 ) & 0xFFFF0000 ) + getVolumeRank(voldesc);
  return rank;
}


Error ObjectRankEngine::doRanking()
{
  Error err(ERR_OK);
  /* case #1: migrator took and applied delta change table since last ranking process
   *    rank delta chg table will be empty and we will re-rank objects
   *    in the rank table and see which objects from the cached map will be inserted 
   *    to the ranking table (thus demoting lower-rank objects from the ranking table)
   *    and which cached objects will be 'demoted' right away;
   * case #2: migrator did not take delta change table since last ranking process
   *    so rankDeltaChgTbl contains delta change table from last ranking process; those
   *    objects could have become 'hotter' or 'colder', so we need to include objects 
   *    from that table as well when deciding who stays in the rank table
   * 
   * case #3: migrator took delta change table, but ranking process started before 
   *    migrator told us that it's done with it. TODO: handle this case. 
   * case #4: migrator tries to take delta change table while ranking process is in
   *    progress. The delta change table will be temporarily empty during the ranking
   *    process, so will return error.  TODO: better way to handle this (?)
   */

  const fds_uint32_t max_lowrank_objs = 25;

  rank_order_objects_t *ordered_objects = NULL; /* temp helper ordered list of objects */
  rank_order_objects_it_t mm_it, tmp_it;
  obj_rank_cache_it_t it;
  fds_uint32_t iters = 0;
  obj_rank_cache_t *cached_objects = new obj_rank_cache_t;
  if (!cached_objects) {
    FDS_PLOG(ranklog) << "ObjectRankEngine: failed to allocate memory for cached obj map";
    err = ERR_MAX;
    return err;
  }
  ordered_objects = new rank_order_objects_t;
  if (!ordered_objects) {
    FDS_PLOG(ranklog) << "ObjectRankEngine: failed to allocate tmp memory for ranking process";
    delete cached_objects;
    err = ERR_MAX;
    return err;
  }

  FDS_PLOG(ranklog) << "ObjectRankEngine: start ranking process";

  /* first swap cached_obj_map with local map, so next insert/delete object 
   * method calls will work with a clear map.
   * cached_objects map will hold all cached objects since 
   * the last ranking process */
  map_mutex->lock();
  cached_objects->swap(cached_obj_map);
  map_mutex->unlock();

  /* insert the whole delta change table (if left from previous ranking process) to cached_objects */
  tbl_mutex->lock();
  /* insert is ok because if an object is in both maps (e.g. it was deleted)
   * then objects from cached_obj_map which are more recent take precedence  */
  cached_objects->insert(rankDeltaChgTbl.begin(), rankDeltaChgTbl.end());
  rankDeltaChgTbl.clear(); /* we still have this version of delta chg tbl persistent */
  lowrank_objs.clear(); /* this one in rank table, so just throw away */
  tbl_mutex->unlock();

  /* cached_objects:
   * for sure promote  +
   * for sure demote   -
   * candidates -- will compare with objects in rank table

   /* rank all objects in cached_objects and insert in ordered_objects map in rank,object order  */
  it = cached_objects->begin();
  while (it != cached_objects->end()) {
    fds_uint32_t rank = updateRank(it->first, it->second);
    std::pair<fds_uint32_t, ObjectID> entry(rank, it->first); 
    ordered_objects->insert(entry);
    //FDS_PLOG(ranklog) << "ObjectRankEngine: Ranking: cached obj " << (it->first).ToString() << " rank=" << rank;
    cached_objects->erase(it);
    it = cached_objects->begin();
  }
  /* cleanup cached_objects, they are all moved to ordered_objects map */
  delete cached_objects;
  cached_objects = NULL;

  /* Step 1 -- go through ordered_objects and update/delete entries in the database, 
   * add to the database only if the database would not grow over its max size */
  fds_verify(tmp_chgTbl.size() == 0);
  FDS_PLOG(ranklog) << "ObjectRankEngine: Ranking: initial db size " << cur_rank_tbl_size;

  mm_it = ordered_objects->begin();
  iters = 0; /* guarding that this loop actually finishes, we should see at most 2 full iterations of ordered_objects */
  int dbg_count = 0;
  while ((cur_rank_tbl_size < rank_tbl_size) &&
	 (ordered_objects->size() > 0))
    {
      fds_uint32_t diff_count = rank_tbl_size - cur_rank_tbl_size;
      ObjectID oid = mm_it->second;
      fds_uint32_t o_rank = mm_it->first;

      Record key((const char*)&oid, sizeof(oid));
      std::string val = "";

      /* we want to query first on any case, so we can do correct accounting of # objs in db */
      err = rankDB->Query(key, &val);
      if (o_rank == OBJECT_RANK_INVALID) {
	/* delete from database if it's there */
	if ( err.ok() ) {
	  Record del_key((const char*)&oid, sizeof(oid));
	  err = rankDB->Delete(del_key);
	  fds_verify(cur_rank_tbl_size > 0);
	  --cur_rank_tbl_size;
	  FDS_PLOG(ranklog) << "ObjectRankEngine: Ranking: removing obj " << oid.ToHex() << " from db";
	}
	/* remove from ordered_objects */
	tmp_it = mm_it;
	++mm_it;
	ordered_objects->erase(tmp_it);
      }
      else {
	/* object not marked for deletion  -- update/possibly add to database */
	if ( err.ok() || ((err == ERR_CAT_ENTRY_NOT_FOUND) && (diff_count > 0)) ) {
	  /* we are ok either update rank or add new entry */
	  Record put_key((const char*)&oid, sizeof(oid));
	  std::string valstr = std::to_string(o_rank);
	  Record put_val(valstr);
	  if (err == ERR_CAT_ENTRY_NOT_FOUND) {
	    /* we are adding new entry */
	    ++cur_rank_tbl_size;
	    //FDS_PLOG(ranklog) << "ObjectRankEngine: Ranking: adding obj " << oid.ToHex() << " rank " << o_rank << " to db";
	  }
	  err = rankDB->Update(put_key, put_val);

	  /* remove from ordered_objects */
	  tmp_it = mm_it;
	  ++mm_it;
	  ordered_objects->erase(tmp_it);
	}
	else {
	  ++mm_it;
	}
      }

      if (mm_it == ordered_objects->end()) {
	mm_it = ordered_objects->begin();
	++iters;
	fds_verify(iters < 3);
      }

      ++dbg_count;
      if ( (dbg_count % 1000) == 0) FDS_PLOG(ranklog) << "ObjectRankEngine: Ranking: processed batch of 1000 objs in part 1";
    }

  FDS_PLOG(ranklog) << "ObjectRankEngine: finished part 1 of ranking process, starting part 2";

  /* if ordered_objects map is empty, it means that we could fit everything in rank table
   * except of course objects mapped for demotion. If so, we have change table in cached_chg_table.
   * so we are done
   * -- the ranking table in db may still have some outdated ranks, but ok for now (not 
   *    sure if it's even useful to update them). 
   * 
   * Also, if we got signal that we are exiting or ranking was disabled while we were ranking,
   * we can declare that current change table we build + what we currently have in ordered_objects
   * is what we are going to demote (which is not completely accurate, because objects in rank
   * table may have lower rank) -- this is our approach for now, TODO: something better
   */
  if (exiting || (atomic_load(&rankingEnabled)==false)) {
    // our current ordered_object are those we are going to demote (not complete accurate, see above comment)
    for (mm_it = ordered_objects->begin();
	 mm_it != ordered_objects->end();
	 ++mm_it) {
      tmp_chgTbl[mm_it->second] = mm_it->first | RANK_DEMOTION_MASK;
      FDS_PLOG(ranklog) << "ObjectRankEngine: Ranking (early stop): (to demote) obj " << (mm_it->second).ToHex();
    }
    ordered_objects->clear();
  }

  if (ordered_objects->size() == 0) {
    /* now we need to persist change table and swap to rankDeltaChgTbl */
    err = persistChgTableAndSwap();

    FDS_PLOG(ranklog) << "ObjectRankEngine: Ranking finished; result  " << err.GetErrstr();

    /* cleanup and exit */
    delete ordered_objects;
    return err;
  }

  /* Step 2 -- if we are here, ordered_objects contain candidate objects that may or may not replace 
   * objects currently in the ranking table. So we walk rank table and compare which objects
   * will actually 'fall out' from it. We will do it in chunks:
   *
   * Read N objects from rank table, update rank, insert to ordered_objects map. Take N
   * highest ranked objects from ordered_objects map and put to db. Continue until we walked
   * all objects in db. If we accidentally insert object and read it again (if iterator reaches
   * it again) its rank should be the same, so we just read/update it again -- not super efficient
   * but still correct. We will need to revisit efficiency later.
   *
   * Because we also want to keep lowrank_objs, we will first read 'max_lowrank_objs' to 
   * ordered_objects, and then start the process in above para. So after we load our last chunk
   * from db, 'max_lowrank_objs' + initial 'ordered_objects.size()' objects in the ordered_objects
   * map will be the lowest ranked objects. We will put the higher ranked 'max_lowrank_objs' to
   * db and also keep them in lowrank_objs map and move the others to delta change table.  
   **/
  fds_uint32_t max_count = max_lowrank_objs;
  fds_uint32_t it_count = 0;
  fds_uint32_t addt_count = 0; /* this will be additional objs we put initially to ordered_objects */
  fds_uint32_t objs_count = 0;

  Catalog::catalog_iterator_t *db_it = rankDB->NewIterator();
  fds_bool_t stopped = false;
  for (db_it->SeekToFirst(); db_it->Valid(); db_it->Next()) 
    {
      Record key = db_it->key();
      Record value = db_it->value();
      ObjectID oid(key.ToString());
      fds_uint32_t rank = (fds_uint32_t)std::stoul(value.ToString());
      /* we will temporary set demotion flag, but reset if we move those objs back to rank table */
      rank = updateRank(oid, rank);
      //FDS_PLOG(ranklog) << "ObjectRankEngine: Ranking: read from db obj " << key.ToString() << " rank " << rank;
      fds_verify(isRankDemotion(rank) == false);
      rank |= RANK_DEMOTION_MASK;
      std::pair<fds_uint32_t,ObjectID> entry(rank, oid);
      ordered_objects->insert(entry);

      ++it_count;
      ++objs_count;
      if ((it_count < max_count) && (objs_count < cur_rank_tbl_size)) continue;
      FDS_PLOG(ranklog) << "ObjectRankEngine: Ranking part 2: pulled " << it_count << " objects to process";

      /* the first time we put number of objects = number of lowrank objects we want to keep cached */
      if (addt_count == 0) {
	if (objs_count < cur_rank_tbl_size)  {
	  addt_count = it_count;
	  it_count = 0; 
	  continue;
	}
      }

      /* If we finished with 'count' objects, write 'count' highest rank objects back to rank table */
      mm_it = ordered_objects->begin();
      if (objs_count == cur_rank_tbl_size) {
	/* we will need to write additional 'lowrank_count' objects to db since we initially 
	 * added that many objects to ordered_objects */
	it_count += addt_count;
	max_count += addt_count; 
      }  
      for (fds_uint32_t i = 0; i < max_count; ++i)
	{
	  fds_verify(mm_it != ordered_objects->end());
	  ObjectID oid = mm_it->second;

	  rank = mm_it->first & (~RANK_DEMOTION_MASK); /* make sure demotion flag not set when moving back to rank db */
	  Record put_key((const char*)&oid, sizeof(oid));
	  std::string valstr = std::to_string(rank);
	  Record put_val(valstr);
	  err = rankDB->Update(put_key, put_val);
	  //FDS_PLOG(ranklog) << "ObjectRankEngine: Ranking: putting back to db obj " 
	  //		    << (mm_it->second).ToHex() << " rank " << rank;

	  if ((objs_count == cur_rank_tbl_size) && (it_count < max_lowrank_objs)) {
	    std::pair<fds_uint32_t,ObjectID> entry(mm_it->first, mm_it->second);
	    tbl_mutex->lock();
	    lowrank_objs.insert(entry);
	    tbl_mutex->unlock();
	  }

	  ordered_objects->erase(mm_it);
	  mm_it = ordered_objects->begin();

	  --it_count;
	  if (it_count == 0) break;
	}

      /* if we need to exit or we are asked to disable ranking process, exit this loop */
      if (exiting || (atomic_load(&rankingEnabled) == false)) {
	stopped = true;
	break;
      }
    }

  /* ordered_objects now contains objects that we demote */
  for (mm_it = ordered_objects->begin();
       mm_it != ordered_objects->end();
       ++mm_it)
    {
      FDS_PLOG(ranklog) << "ObjectRankEngine: Ranking: (to demote) obj " << (mm_it->second).ToHex() 
			<< " rank " << (mm_it->first | RANK_DEMOTION_MASK);
      tmp_chgTbl[mm_it->second] = mm_it->first | RANK_DEMOTION_MASK;
    }
  ordered_objects->clear();
  delete ordered_objects;
  ordered_objects = NULL;

  /* update the tail rank with the last object in the rank table (if its full) */
  if (cur_rank_tbl_size < rank_tbl_size)
    tail_rank = OBJECT_RANK_INVALID;
  else {
    /* safe to update without lock because lowrank_objs never accessed if ranking is in progress */
    rank_order_objects_rit_t rit = lowrank_objs.rbegin();
    tail_rank = rit->first;
  }

  /* persist the change table */
  err = persistChgTableAndSwap();

  FDS_PLOG(ranklog) << "ObjectRankEngine: Ranking finished, rank table size " << cur_rank_tbl_size;

  return err;
}


/* performing ranking process when notified */
void ObjectRankEngine::runRankingThreadInternal()
{
  Error err(ERR_OK);

  while (1)
    {
      if (exiting == true) {
	break;
      }

      /* wait until we are notified to start ranking process */
      while ( !rank_notify->timed_wait_for_notification(300000) ) {
	if (atomic_load(&rankingEnabled)) break;
      }

      /* perform ranking */
      err = doRanking();

      /* set flag that we finished ranking process (even if we stopped due to stopObjRanking */
      atomic_store(&rankingEnabled, false);
    }
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
      /* notify the ranking thread to start the ranking process  */
      rank_notify->notify();
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

fds_uint32_t ObjectRankEngine::getDeltaChangeTblSegment(fds_uint32_t num_objs, std::pair<ObjectID,rankOperType>* objRankArray)
{
  fds_uint32_t ret_count = 0;

  /* TODO: we don't handle power failures yet. Assumes that once we return objects from delta 
   * change table, we remove them right awau from delta change table. So if those objects do 
   * not get actually promoted/demoted, ranking engine loses info about them  */
  if (rankingInProgress())
    return 0;

  tbl_mutex->lock();
  obj_rank_cache_it_t it = rankDeltaChgTbl.begin();
  for (fds_uint32_t i = 0; i < num_objs; ++i)
    {
      if (it == rankDeltaChgTbl.end()) break;
      std::pair<ObjectID, rankOperType> entry(it->first, isRankDemotion(it->second) ? OBJ_RANK_DEMOTION : OBJ_RANK_PROMOTION); 
      objRankArray[i] = entry;
      rankDeltaChgTbl.erase(it);
      it = rankDeltaChgTbl.begin();
      ++ret_count;
    }

  tbl_mutex->unlock();

  return ret_count;
}

Error ObjectRankEngine::persistChgTableAndSwap()
{
  Error err(ERR_OK);
  // TODO: need additional db to persist

  /* swap to rankDeltaChgTbl */
  tbl_mutex->lock();
  rankDeltaChgTbl.swap(tmp_chgTbl);
  tbl_mutex->unlock();
  fds_verify(tmp_chgTbl.size() == 0);

  return err;
}

fds_uint32_t ObjectRankEngine::getNextTblSegment(fds_uint32_t num_objs, ObjectID *objIdArray)
{
  fds_uint32_t ret_count = 0;
  fds_verify(objIdArray != NULL);

  if (rankingInProgress())
    return 0;

  /* get lowest rank objects from our lowrank_objs cache, once we get them, we remove them
   * from the cache */

  tbl_mutex->lock();
  rank_order_objects_rit_t rit = lowrank_objs.rbegin();
  for (fds_uint32_t i = 0; i < num_objs; ++i)
    {
      if (rit == lowrank_objs.rend()) break;
      objIdArray[i] = rit->second;
      lowrank_objs.erase(--rit.base());
      rit = lowrank_objs.rbegin(); 
      ++ret_count;
    }  
  tbl_mutex->unlock();

  return ret_count;
}

void ObjectRankEngine::runRankingThread(ObjectRankEngine* self)
{
  fds_verify(self != NULL);
  self->runRankingThreadInternal();
}

} // namespace fds



