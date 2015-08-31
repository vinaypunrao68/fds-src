/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 * Rank engine implementation
 */

// Standard includes.
#include <set>
#include <utility>
#include <string>

// Internal includes.
#include "catalogKeys/ObjectRankKey.h"
#include "leveldb/db.h"

// Class include.
#include "ObjRank.h"

namespace fds {

ObjectRankEngine::ObjectRankEngine(const std::string& _sm_prefix,
                                   fds_uint32_t _tbl_size,
                                   fds_uint32_t rank_freq_sec,
                                   fds_uint32_t _hot_threshold,
                                   fds_uint32_t _cold_threshold,
                                   StorMgrVolumeTable* _sm_volTbl,
                                   ObjStatsTracker *_obj_stats)
        : rank_tbl_size(_tbl_size),
          rank_period_sec(rank_freq_sec),
          cur_rank_tbl_size(0),
    hot_threshold(_hot_threshold),
    cold_threshold(_cold_threshold),
    max_cached_objects(MAX_RANK_CACHE_SIZE),
    tail_rank(OBJECT_RANK_LOWEST),
    obj_stats(_obj_stats),
    sm_volTbl(_sm_volTbl),
    rankTimer(new FdsTimer()),
    rankTimerTask(new RankTimerTask(*rankTimer, this)) {
    std::string filename(_sm_prefix + "ObjRankDB");
    try
    {
        rankDB = new Catalog(filename);
    }
    catch(const CatalogException& e)
    {
        LOGERROR << "Failed to create Catalog " << filename;
        LOGERROR << e.what();

        /*
         * TODO(Greg): We need to end this process at this point, but we need
         * a more controlled and graceful way of doing it. I suggest that in
         * such cases we throw another exception to be caught by the mainline
         * method which can then perform any cleanup, log a useful message, and
         * shutdown.
         */
        LOGNORMAL << "SM shutting down with a failure condition.";
        exit(EXIT_FAILURE);
    }

    map_mutex = new fds_mutex("RankEngineMutex");
    tbl_mutex = new fds_mutex("RankEngineChgTblMutex");

    rankingEnabled = ATOMIC_VAR_INIT(false);
    rankeng_state = RANK_ENG_INITIALIZING;

    rank_notify = new fds_notification();

    /* for now set low threshold for hot objects -- does not impact correctness, just
     * the amount of memory stat tracker will need to keep the list of hot objects */
    obj_stats->setHotObjectThreshold(hot_threshold);
    obj_stats->setColdObjectThreshold(cold_threshold);

    /* TMP -- for testing/demo, if we have small rank table size, lets use
     * small cache of insertions/deletions so it will trigger ranking process more often */
    max_cached_objects = 250;

    LOGNORMAL << "Construction done. Rank period " << rank_period_sec << " sec, "
              << " Size of rank table " << rank_tbl_size
              << ", Hot obj threshold " << hot_threshold
              << ", Cold object threshold " << cold_threshold;
}

ObjectRankEngine::~ObjectRankEngine() {
    rankeng_state = RANK_ENG_EXITING;
    rankTimer->destroy();

    /* make sure ranking thread is not waiting  */
    rank_notify->notify();

    delete rank_notify;
    delete map_mutex;
    delete tbl_mutex;

    if (rankDB) {
        delete rankDB;
    }
}

/* called by background thread to finish initialization process -- find number of entries in rankdb
 * and build a cache of the rank table tail (lowest ranked objects) */
Error ObjectRankEngine::initialize()
{
    Error err(ERR_OK);
    rank_order_objects_it_t mm_it;
    /* we don't lock lowrank table here because none of the methods accessing lowrank
     * table or chg table will access these (they check if we are still in init state
     * and return right away), except for inser/delete cache which we don't access here */

    auto db_it = rankDB->NewIterator();
    for (db_it->SeekToFirst(); db_it->Valid(); db_it->Next())
    {
        ObjectRankKey key { db_it->key() };
        leveldb::Slice value = db_it->value();
        ObjectID oid = key.getObjectId();
        fds_uint32_t rank = (fds_uint32_t)std::stoul(value.ToString());
        rank = updateRank(oid, rank);
        std::pair<fds_uint32_t, ObjectID> entry(rank, oid);
        lowrank_objs.insert(entry);

        if (lowrank_objs.size() > LOWRANK_OBJ_CACHE_SIZE) {
            /* kick out highest ranked obj from lowrank cache */
            mm_it = lowrank_objs.begin(); /* highest ranked */
            fds_verify(mm_it != lowrank_objs.end());
            lowrank_objs.erase(mm_it);
        }
        err = putToRankDB(oid, rank, false, false);
        if (!err.ok()) {
            LOGERROR << "couldn't update rank db";
        }
        ++cur_rank_tbl_size; /* we are counting number of entries in db here as well */
    }

    /* start timer for getting hot objects from the stats tracker.
     * for now setting small intervals so we can have short demo */
    if (!rankTimer->scheduleRepeated(rankTimerTask, std::chrono::seconds(rank_period_sec))) {
        LOGNORMAL << "ObjectRankEngine: ERROR: failed to schedule timer to "
                  << "analyze stats from stat tracker";
        err = ERR_MAX;
    }

    LOGNORMAL << "ObjectRankEngine: finished initialization process, "
              << "rank table size " << cur_rank_tbl_size;
    rankeng_state = RANK_ENG_ACTIVE;
    return err;
}

fds_uint32_t ObjectRankEngine::rankAndInsertObject(const ObjectID& objId, const VolumeDesc& voldesc)
{
    fds_uint32_t rank = getRank(objId, voldesc);
    fds_bool_t start_ranking_process = false;

    /* is it possible that object may already exist? if so, what is the expected behavior? */
    /* current implementation works as if insert of existing object is an update, eg. of
     * existing object has 'all disk' policy but we insert same object with 'all ssd' policy,
     * the object rank will reflect 'all ssd' policy */
    LOGTRACE << "ObjectRankEngine: rankAndInsertObject: vol " << voldesc.volUUID
             << " cached_obj_map size " << cached_obj_map.size();

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
        cached_obj_map[objId] = OBJECT_RANK_LOWEST | RANK_INVALID_MASK;
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
    if (voldesc.mediaPolicy == fpi::FDSP_MEDIA_POLICY_HDD)
        return OBJECT_RANK_ALL_DISK;
    if (voldesc.mediaPolicy == fpi::FDSP_MEDIA_POLICY_SSD)
        return OBJECT_RANK_ALL_SSD;

    rank = ((getObjectSubrank(objId) << 16) & 0xFFFF0000) + getVolumeRank(voldesc);
    return rank;
}

/*
 * Called periodically (on timer) to get list of 'hot' objects from stat tracker
 * If an object in that list is not in the rank table and its rank is higher than
 * the lowest-ranked object in the rank table, we will insert this hot object to
 * the rank table and kick out the lowest ranked object from the rank table (if
 * the rank table is full).
 *
 * We do the analysis based on the cached list of lowest ranked objects. It will
 * be slightly outdated -- e.g. higher ranked objects in the rank table may
 * become cold and should be moved down, but we don't want to run the whole
 * rankin process here. We will do the best guess based on the cached list, and
 * ranking process running less often than stat analyzis will make sure that
 * out cached list is not too much out-of-date.
 *
 */
void ObjectRankEngine::analyzeStats()
{
    fds_uint32_t rank, lowest_rank;
    fds_bool_t stop = false;
    std::set<ObjectID, ObjectLess> hot_list;
    std::set<ObjectID, ObjectLess>::iterator list_it;
    StorMgrVolume* vol = NULL;
    fds_volid_t volid;
    /* default volume desc we will use if we don't get voldesc, e.g. if using unit test */
    VolumeDesc voldesc(std::string("novol"), fds_volid_t(1), 0, 0, 10);

    /* ignore the hot objects if we are already in the ranking process, for now...
     * need some extra care since lowrank objs and change table are temporarily unavail that time */
    if (rankingInProgress())
        return;

    obj_stats->getHotObjectList(hot_list);
    LOGNORMAL << "Hot obj list size " << hot_list.size()
              << " lowrank objs list size " << lowrank_objs.size();

    /* update ranks of objects in the cached list of lowest ranked objects in the table */
    tbl_mutex->lock();
    {
        rank_order_objects_t new_lowrank_objs;
        rank_order_objects_it_t it = lowrank_objs.begin();
        while ( it != lowrank_objs.end())
        {
            // if this object in hot list, just remove from hot list,
            // since it's already in the table
            list_it = hot_list.find(it->second);
            if (list_it != hot_list.end()) {
                hot_list.erase(list_it);
            }

            rank = updateRank(it->second, it->first);
            std::pair<fds_uint32_t, ObjectID> entry(rank, it->second);
            new_lowrank_objs.insert(entry);
            lowrank_objs.erase(it);
            it = lowrank_objs.begin();
        }
        lowrank_objs.swap(new_lowrank_objs);
        /* if there are no obj cached in lowrank cache, we don't dig lowest ranks in db, but
         * don't promote anymore hot objects until next ranking process  */
        if (lowrank_objs.size() == 0) {
            stop = true;
            LOGNORMAL << "ObjectRankEngine: will stop, lowrank empty";
        }
    }
    tbl_mutex->unlock();

    /* iterate through list of hot objects, and promote if possible */
    lowest_rank = getLowestRank();
    for (list_it = hot_list.begin();
         list_it != hot_list.end();
         ++list_it)
    {
        if (stop) break;

        /* not checking delta change table if promoted, must be in rankdb (will check below)
         * and if demoted, we will handle this when putting obj to rankdb (will not be demoted anymore */

        /* check cached insertions/deletions -- if it was recently inserted
         * then no need to promote; if deleted not sure yet what we should do,
         * so for now not considering those objects */
        if (inInsertDeleteCache(*list_it)) {
            LOGNORMAL << " in insert/delete cache";
            continue;
        }

        /* see if we can promote this object */
        volid = obj_stats->getVolId(*list_it);
        vol = sm_volTbl->getVolume(volid);
        rank = getRank(*list_it, (vol != NULL) ? *(vol->voldesc) : voldesc);
        if ((rank < lowest_rank) &&
            !inRankDB(*list_it))
        {
            /* since this obj is ranked higher than at least on object
             * in our cached rank table tail (plus we just re-computed
             * those ranks, so they are up-to-date), this obj definitely
             * can be promoted
             * Note that we may have false negatives -- once we kick out all
             * objects from cached tail of rank table, we cannot kick out anymore
             * since we need to recompute rank table in db. Maybe that would
             * be a good time to start the ranking process... TODO */

            /* insert to lowrank_objs. If database not full, remove highest ranked from
             * from lowrank_objs; otherwise, demote lowest ranked in lowrank_objs */
            std::pair<fds_uint32_t, ObjectID> entry(rank, *list_it);
            LOGNORMAL << " vol " << volid << " rank " << rank;

            tbl_mutex->lock();
            lowrank_objs.insert(entry);
            rank_order_objects_it_t it = lowrank_objs.begin(); /* highest ranked */
            fds_verify(it != lowrank_objs.end());
            /* if space in rank table -- remove highest ranked from cached tail */
            if ((cur_rank_tbl_size < rank_tbl_size) || (it->second == *list_it)) {
                /* also if this obj is highest ranked of all objs in cached tail, remove it
                 * from cached tail because we don't want migrator to migrate it  */
                lowrank_objs.erase(it);
            }

            /* if table is full, we need to demote a lowest ranked object */
            if (cur_rank_tbl_size >= rank_tbl_size) {
                rank_order_objects_rit_t rit = lowrank_objs.rbegin(); /* lowest ranked */
                /* otherwise lowest rank will be highest possible rank */
                fds_verify(rit != lowrank_objs.rend());
                /* otherwise something wrong with lowest rank */
                fds_verify(rit->second != *list_it);
                ObjectID del_oid(rit->second);
                rankDeltaChgTbl[del_oid] = rit->first | RANK_DEMOTION_MASK;
                lowrank_objs.erase(--rit.base());
                rit = lowrank_objs.rbegin();
                if (rit != lowrank_objs.rend()) {
                    lowest_rank = rit->first;
                    tail_rank = lowest_rank;
                } else {
                    stop = true;
                }
                tbl_mutex->unlock();
                deleteFromRankDB(del_oid);
                LOGNORMAL;
            } else {
                tbl_mutex->unlock();
            }

            /* we will add directly to rankDB without putting it to the cached rankDB tail.
             * if we put to the cached rankDB tail, it is likely that migrator will get those
             * objects and remove from flash -- while those objects are most likely relatively
             * high ranked */
            putToRankDB(*list_it, rank, true, false);
            tbl_mutex->lock();
            rankDeltaChgTbl[*list_it] = rank | RANK_PROMOTION_MASK;
            tbl_mutex->unlock();
            LOGNORMAL << " rank " << rank << " will be promoted";
        } else {
            LOGNORMAL << " rank " << rank << " lower than ranks of all objs in rank table";
        }
    }
    hot_list.clear();
}

/*
 * This method is called either during ranking process or on timer when
 * we get list of hot objects from the stat tracker. Since we only
 * process hot list when ranking process not in progress, updating
 * cur_rank_tbl_size or updating db is safe
 */
Error ObjectRankEngine::deleteFromRankDB(const ObjectID& oid)
{
    Error err(ERR_OK);
    ObjectRankKey key { oid };
    err = rankDB->Delete(key);
    fds_verify(cur_rank_tbl_size > 0);
    --cur_rank_tbl_size;
    LOGNORMAL << " from rankDB";
    return err;
}

/*
 * Assumes that the caller already checked whether the object in rankDB
 * and pass b_addition true if does not exist and need to add object, or
 * false if this is an update
 * This method is called either during ranking process or on timer when
 * we get list of hot objects from the stat tracker. Since we only
 * process hot list when ranking process not in progress, updating
 * cur_rank_tbl_size or updating db is safe
 */
Error ObjectRankEngine::putToRankDB(const ObjectID& oid, fds_uint32_t rank,
                                    fds_bool_t b_addition, fds_bool_t b_tmp_chgtbl)
{
    Error err(ERR_OK);
    /* ignore helper bits specifying promotion/demotion/etc. */
    fds_uint32_t o_rank = rank & 0xFFFFFF00;
    ObjectRankKey key { oid };
    std::string valstr = std::to_string(o_rank);
    leveldb::Slice put_val(valstr);
    if (b_addition) {
        /* we are adding new entry */
        ++cur_rank_tbl_size;
        LOGNORMAL << " rank " << o_rank << " to db";
    }
    err = rankDB->Update(key, put_val);

    /* if we add to rankdb something we demoted (e.g. deletions happend after hot objs
     * demoted objs in rank db), this obj is not demoted anymore */
    if (isRankDemotion(rank)) {
        fds_verify(b_addition);  // we delete from db when demoting
        if (b_tmp_chgtbl) {
            tmp_chgTbl.erase(oid);
        } else {
            tbl_mutex->lock();
            rankDeltaChgTbl.erase(oid);
            tbl_mutex->unlock();
        }
    }

    return err;
}

fds_bool_t ObjectRankEngine::inRankDB(const ObjectID& oid)
{
    Error err(ERR_OK);
    ObjectRankKey key { oid };
    std::string val("");

    err = rankDB->Query(key, &val);

    return (err.ok());
}

/* remove all objects in 'cached_objects' that are marked for deletion from rankDB
 * and if we see insertions for objs we wanted to promote, remove those promote ops
 * rank delta change table (those objs already in flash, no need to promote)
 * Caller must swap rankDeltaChgTbl to tmp_chgTbl before calling this method */
Error ObjectRankEngine::applyCachedOps(obj_rank_cache_t* cached_objects)
{
    Error err(ERR_OK);
    obj_rank_cache_it_t it, chgtbl_it;
    fds_uint32_t o_rank;
    fds_verify(cached_objects != NULL);
    for (it = cached_objects->begin();
         it != cached_objects->end(); )
    {
        o_rank = it->second;
        if (isRankInvalid(o_rank))
        {
            /* deletion */
            if (!inRankDB(it->first)) {
                err = deleteFromRankDB(it->first);
            }
            /* remove from delta change table if there */
            tmp_chgTbl.erase(it->first);
            /* remove from cached_objects */
            it = cached_objects->erase(it);
        } else {
            /* insertion - remove dup 'promotion' from delta chg tbl */
            chgtbl_it = tmp_chgTbl.find(it->first);
            if (chgtbl_it != tmp_chgTbl.end()) {
                if (isRankPromotion(chgtbl_it->second)) {
                    tmp_chgTbl.erase(chgtbl_it);
                    ++it;
                } else {
                    it = cached_objects->erase(it);
                }
            } else {
                ++it;
            }
        }
    }
    return err;
}

/*
 * Does complete re-ranking of all the objects in caches + rank database and
 * re-builds the rank table + delta change table + cached tail of rank table
 *
 * State when the ranking process starts:
 *  - delta change table may contain:
 *       1) promotions of hot objects we got from stats tracker since last
 *          ranking process;
 *       2) demotions of objects that were kicked out by the hot objects we got
 *          from the stats tracker since last ranking process
 *       3) (possibly) delta change table from a previous ranking process(es)
 *  - cached insertions/deletions (not applied to rankDB or any other caches)
 *  - cached tail of the rank table in database that we cached during the last
 *    ranking process; those objects are in db so we just clear the list and build new
 *
 * While ranking is in progress, delta change table and cached tail of the rank table
 * is unavailable (temporarily empty), so all functions that need access to them
 * check if ranking is in progress and don't return anything if ranking is in progress.
 * The insertion/deletion cache 'cached_obj_map' is always available and this is the
 * only cache that is accessed on datapath (via rankAndIsertObject and deleteObject)
 *
 * State after ranking process completes:
 *  - delta change table contains updated list of promotions/demotions
 *  - cached tail of the rank table in the database will contain most recent info
 */
Error ObjectRankEngine::doRanking()
{
    Error err(ERR_OK);
    const fds_uint32_t max_lowrank_objs = LOWRANK_OBJ_CACHE_SIZE;
    rank_order_objects_t *ordered_objects = NULL; /* temp helper ordered list of objects */
    rank_order_objects_it_t mm_it, tmp_it;
    obj_rank_cache_it_t it;
    obj_rank_cache_t *cached_objects = new obj_rank_cache_t;
    if (!cached_objects) {
        LOGNORMAL << "ObjectRankEngine: failed to allocate memory for cached obj map";
        err = ERR_MAX;
        return err;
    }
    ordered_objects = new rank_order_objects_t;
    if (!ordered_objects) {
        LOGNORMAL << "ObjectRankEngine: failed to allocate tmp memory for ranking process";
        delete cached_objects;
        err = ERR_MAX;
        return err;
    }

    LOGNORMAL << "ObjectRankEngine: start ranking process";

    /* first swap cached_obj_map with local map, so next insert/delete object
     * method calls will work with a clear map.
     * cached_objects map will hold all cached objects since
     * the last ranking process */
    map_mutex->lock();
    cached_objects->swap(cached_obj_map);
    map_mutex->unlock();

    tbl_mutex->lock();
    fds_verify(tmp_chgTbl.size() == 0);
    tmp_chgTbl.swap(rankDeltaChgTbl);
    lowrank_objs.clear(); /* this one is in rank table, so throw away */
    tbl_mutex->unlock();

    /* Step 1 -- apply all deletions to the rankDB, and make
     * sure no collisions with delta chg tbl
     */
    LOGNORMAL << "ObjectRankEngine: Ranking: initial db size " << cur_rank_tbl_size;
    err = applyCachedOps(cached_objects);
    if (!err.ok()) {
        LOGNORMAL;
        /*TODO: how do we handle this? for now ignore */
    }

    /* only insert 'demotions' from delta chg table to cached_objs (promotions are already in db) */
    for (it = tmp_chgTbl.begin(); it != tmp_chgTbl.end(); ++it) {
        if (isRankDemotion(it->second))
            cached_objects->insert(it, it);
    }

    /* rank all objects in cached_objects and insert in ordered_objects map in rank,object order  */
    it = cached_objects->begin();
    while (it != cached_objects->end()) {
        fds_uint32_t rank = updateRank(it->first, it->second);
        std::pair<fds_uint32_t, ObjectID> entry(rank, it->first);
        ordered_objects->insert(entry);
        // LOGNORMAL << " rank=" << rank;
        cached_objects->erase(it);
        it = cached_objects->begin();
    }
    /* cleanup cached_objects, they are all moved to ordered_objects map */
    delete cached_objects;
    cached_objects = NULL;

    /* if nothing in rank table and nothing in ordered objects, we are done  */
    if ((cur_rank_tbl_size == 0) && (ordered_objects->size() == 0)) {
        /* persist the change table */
        err = persistChgTableAndSwap();
        delete ordered_objects;
        LOGNORMAL << "ObjectRankEngine: Ranking finished, rank table size " << cur_rank_tbl_size;
        return err;
    }

    /* Step 2 -- go through ordered_objects and either 1) update rank if object already in rank table
     * or 2) add highest ranked objects to database if the rank table is not full
     */
    LOGNORMAL << "ObjectRankEngine: Ranking: part 2: rank db size " << cur_rank_tbl_size;
    for (mm_it = ordered_objects->begin();
         mm_it != ordered_objects->end(); )
    {
        fds_uint32_t diff_count = rank_tbl_size - cur_rank_tbl_size;
        ObjectID oid = mm_it->second;
        fds_uint32_t o_rank = mm_it->first;
        fds_bool_t in_rankdb = inRankDB(oid);
        if ( in_rankdb || (diff_count > 0))
        {
            err = putToRankDB(oid, o_rank, !in_rankdb, true);
            mm_it = ordered_objects->erase(mm_it);
        } else {
            ++mm_it;
        }
    }
    fds_verify((cur_rank_tbl_size > 0) ||  (ordered_objects->size() == 0));
    LOGNORMAL << "ObjectRankEngine: finished part 2 of ranking process, "
              << "starting part 3; cur_rank_tbl_size = "
              << cur_rank_tbl_size;

    /* Step 3 -- if we are here, ordered_objects contain candidate objects that may or may not replace
     * objects currently in the ranking table. So we walk rank table and compare which objects
     * will actually 'fall out' from it. We will do it in chunks:
     * If ordered_objects are empty (we could fit everything to the rank table), we still need to
     * cache the rank table tail.
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
    /* this will be additional objs we put initially to ordered_objects */
    fds_uint32_t addt_count = 0;
    fds_uint32_t objs_count = 0;

    auto db_it = rankDB->NewIterator();
    fds_bool_t stopped = false;
    for (db_it->SeekToFirst(); db_it->Valid(); db_it->Next())
    {
        /* if we need to exit or we are asked to disable ranking process, exit this loop
         * TODO: not sure yet if exit will leave us in good state, need to revisit */
        if ((rankeng_state == RANK_ENG_EXITING) || (atomic_load(&rankingEnabled) == false)) {
            stopped = true;
            break;
        }

        leveldb::Slice key = db_it->key();
        leveldb::Slice value = db_it->value();
        ObjectID oid(key.ToString());
        fds_uint32_t rank = (fds_uint32_t)std::stoul(value.ToString());
        rank = updateRank(oid, rank);
        // LOGNORMAL << " rank " << rank;
        /* temp overload set promotion flags to mark that this obj is in rank db - hack?*/
        fds_verify(!isRankDemotion(rank) && !isRankPromotion(rank));
        rank |= RANK_PROMOTION_MASK;
        std::pair<fds_uint32_t, ObjectID> entry(rank, oid);
        ordered_objects->insert(entry);

        ++it_count;
        ++objs_count;
        if ((it_count < max_count) && (objs_count < cur_rank_tbl_size)) continue;
        LOGNORMAL << "ObjectRankEngine: Ranking part 2: pulled "
                  << it_count << " objects to process";

        /* the first time we put number of objects = number of lowrank
         * objects we want to keep cached
         */
        if (addt_count == 0) {
            if (objs_count < cur_rank_tbl_size)  {
                addt_count = it_count;
                it_count = 0;
                continue;
            }
        }

        /* we finished with 'count' objects, write 'count' highest
         * rank objects back to rank table
         */
        mm_it = ordered_objects->begin();
        if (objs_count == cur_rank_tbl_size) {
            /* we will need to write additional 'lowrank_count' objects to db since we initially
             * added that many objects to ordered_objects */
            it_count += addt_count;
            max_count += addt_count;
            objs_count = 0;  // our exit value
        }
        for (fds_uint32_t i = 0; i < max_count; ++i)
        {
            fds_verify(mm_it != ordered_objects->end());
            fds_bool_t in_rankdb = isRankPromotion(mm_it->first);
            fds_uint32_t o_rank = mm_it->first & (~RANK_PROMOTION_MASK);
            err = putToRankDB(mm_it->second, o_rank, !in_rankdb, true);

            if ((objs_count == 0) && (it_count < max_lowrank_objs)) {
                LOGNORMAL << " rank " << (mm_it->first & 0xFFFFFF00);
                std::pair<fds_uint32_t, ObjectID> entry(mm_it->first & 0xFFFFFF00, mm_it->second);
                tbl_mutex->lock();
                lowrank_objs.insert(entry);
                tbl_mutex->unlock();
            }

            ordered_objects->erase(mm_it);
            mm_it = ordered_objects->begin();
            --it_count;
            if (it_count == 0) break;
        }

        /* since we can add obj and then iterate over them again, we may continue loop even
         * when we iterated over cur_rank_tbl_size number of objects; we should version our objects
         * but for now, finish the loop once we iterated cur_rank_tbk_size # obj -- the consequence
         * is that the rank table may contain few lower ranked objs than we got in ordered_objs, but
         * should be small amount -- so ok for now, adding versioning will make it a bit more accurate  */
        if (objs_count == 0) break;
    }

    /* ordered_objects now contains objects that we demote */
    for (mm_it = ordered_objects->begin();
         mm_it != ordered_objects->end();
         ++mm_it)
    {
        fds_uint32_t o_rank = mm_it->first & (~RANK_PROMOTION_MASK);

        /* if obj in rankdb (remember we overload promotion bit to specify this),
         * then remove from there
         */
        if (isRankPromotion(mm_it->first)) {
            deleteFromRankDB(mm_it->second);
        }
        /* if tmp_chgTbl has this obj for promotion, we just delete from
         * chg tbl because this obj never
         * really made it to flash
         */
        it = tmp_chgTbl.find(mm_it->second);
        if ( (it != tmp_chgTbl.end()) && (isRankPromotion(it->second)) ) {
            tmp_chgTbl.erase(it);
      } else {
            tmp_chgTbl[mm_it->second] = o_rank | RANK_DEMOTION_MASK;
            // LOGNORMAL << " rank " << o_rank;
        }
    }
    ordered_objects->clear();
    delete ordered_objects;
    ordered_objects = NULL;

    /* update the tail rank with the last object in the rank table (if its full) */
    tail_rank = getLowestRank();

    /* persist the change table */
    err = persistChgTableAndSwap();

    LOGNORMAL << "ObjectRankEngine: Ranking finished, rank table size " << cur_rank_tbl_size;

    return err;
}

/* returns lowest rank in lowrank_obj cache of lowest-ranked objects */
fds_uint32_t ObjectRankEngine::getLowestRank()
{
    fds_uint32_t ret_rank = OBJECT_RANK_LOWEST; /* lowest rank in case rank table is not full */

    /* we don't lock cur_rank_tbl_size because currently it can only be updated by one thread
     * either ranking thread or on timer but only when ranking process is not running */
    if (cur_rank_tbl_size >= rank_tbl_size)
    {
        tbl_mutex->lock();
        rank_order_objects_rit_t rit = lowrank_objs.rbegin();
        if (rit != lowrank_objs.rend()) {
            ret_rank = rit->first;
        } else {
            /* if lowrank table is empty, better to have false negative */
            ret_rank = OBJECT_RANK_ALL_SSD;
        }
        tbl_mutex->unlock();
    }

    return ret_rank;
}

fds_bool_t ObjectRankEngine::inInsertDeleteCache(const ObjectID& oid)
{
    fds_bool_t ret = false;
    map_mutex->lock();
    ret = (cached_obj_map.count(oid) > 0);
    map_mutex->unlock();
    return ret;
}

/* performing ranking process when notified */
void ObjectRankEngine::runRankingThreadInternal()
{
    Error err(ERR_OK);

    /* when we start this thread, we must be in initialization state  */
    fds_verify(rankeng_state == RANK_ENG_INITIALIZING);
    err = initialize();
    if ( !err.ok() ) {
        LOGNORMAL << "ObjectRankEngine: ERROR failed to initialize";
        // TODO(???): should we set state that ranking engine is invalid or something like that?
        // for now just not doing ranking..
        return;
    }

    while (1)
    {
        if (rankeng_state == RANK_ENG_EXITING) {
            break;
        }

        /**
         * TODO (Sean)
         * There is not reason why this is polling.  We really need to get rid of all
         * polling to determine action.  This just waste cpu cycles.
         */
        /* wait until we are notified to start ranking process */
        while ( !rank_notify->timed_wait_for_notification(3000) ) {
            if (atomic_load(&rankingEnabled)) {
                break;
            }
            if (rankeng_state == RANK_ENG_EXITING) {
                return;
            }
        }

        /* perform ranking */
        err = doRanking();

        /* set flag that we finished ranking process (even if we stopped due to stopObjRanking */
        atomic_store(&rankingEnabled, false);

        // objStorMgr->tierEngine->migrator->startRankTierMigration();
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
        LOGNORMAL << "ObjectRankEngine: Starting ranking process in a background thread";
    }
}

void ObjectRankEngine::stopObjRanking()
{
    fds_bool_t was_enabled = atomic_exchange(&rankingEnabled, false);
    if (was_enabled)
    {
        LOGNORMAL << "ObjectRankEngine: Stopping ranking process before it finished";

        /* should we keep the state if we stop in the middle so we start from
         * where we left off or we will start from the beginning? */
    }
}

fds_uint32_t
ObjectRankEngine::getDeltaChangeTblSegment(fds_uint32_t num_objs,
                                           std::pair<ObjectID, rankOperType>* objRankArray) {
    fds_uint32_t ret_count = 0;

    /* TODO(???): we don't handle power failures yet. Assumes that once we return objects
     * from delta change table, we remove them right awau from delta change table.
     * So if those objects do not get actually promoted/demoted, ranking engine
     * loses info about them
     */
    if (rankingInProgress())
        return 0;

    tbl_mutex->lock();
    obj_rank_cache_it_t it = rankDeltaChgTbl.begin();
    for (fds_uint32_t i = 0; i < num_objs; ++i)
    {
        if (it == rankDeltaChgTbl.end()) break;
        std::pair<ObjectID, rankOperType> entry(it->first,
                                                isRankDemotion(it->second) ?
                                                OBJ_RANK_DEMOTION : OBJ_RANK_PROMOTION);
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
    // TODO(???): need additional db to persist

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

/* timer task to get stats from stat tracker and see if we can promote 'hot' objects
 * (also demoting if we need to kick objs out of rank table) */
void RankTimerTask::runTimerTask()
{
    rank_eng->analyzeStats();
    // objStorMgr->tierEngine->migrator->startRankTierMigration();
}

}  // namespace fds



