/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <vector>
#include <map>
#include <fiu-local.h>
#include <fiu-control.h>
#include <object-store/ObjectStore.h>
#include <object-store/ObjectPersistData.h>
#include <object-store/TokenCompactor.h>
#include <StorMgr.h>
namespace fds {

TokenCompactor::TokenCompactor(SmIoReqHandler *_data_store,
                               SmPersistStoreHandler* persist_store)
        : token_id(0),
          done_evt_handler(NULL),
          verifyData(false),
          data_store(_data_store),
          persistGcHandler(persist_store),
          tc_timer(new FdsTimer()),
          tc_timer_task(new CompactorTimerTask(*tc_timer, this))
{
    state = ATOMIC_VAR_INIT(TCSTATE_IDLE);
    objs_done = ATOMIC_VAR_INIT(0);

    // set fields that will not change during compactor life
    snap_req.io_type = FDS_SM_SNAPSHOT_TOKEN;
    snap_req.smio_snap_resp_cb = std::bind(
        &TokenCompactor::snapDoneCb, this,
        std::placeholders::_1, std::placeholders::_2,
        std::placeholders::_3, std::placeholders::_4);
}

TokenCompactor::~TokenCompactor()
{
    tc_timer->destroy();
}

//
// Creates a new file for the token in tokenFileDb to which non-deleted
// objects will be copied and new puts will go.
// Decides on object id ranges to compact at a time, and creates
// work items to actually get list of objects that need copy /delete
//
Error TokenCompactor::startCompaction(fds_token_id tok_id,
                                      fds_uint16_t disk_id,
                                      diskio::DataTier tier,
                                      fds_bool_t verify,
                                      compaction_done_handler_t done_evt_hdlr)
{
    Error err(ERR_OK);
    tcStateType expect = TCSTATE_IDLE;
    if (!std::atomic_compare_exchange_strong(&state, &expect, TCSTATE_PREPARE_WORK)) {
        LOGNOTIFY << "startCompaction called in non-idle state, ignoring";
        return Error(ERR_NOT_READY);
    }

    // TODO(anna) do not do compaction if sync is in progress, return 'not ready'

    LOGNORMAL << "started compaction of token:" << tok_id
              << " disk:" << disk_id
              << " tier:" << tier
              << " verify:" << verify;
    OBJECTSTOREMGR(data_store)->counters->compactorRunning.incr();

    // remember the token we are goint to work on and object id range for this
    // token -- to safeguard later that we are copying right objects
    token_id = tok_id;
    cur_disk_id = disk_id;
    cur_tier = tier;
    verifyData = verify;
    done_evt_handler = done_evt_hdlr;  // set cb to notify about completion

    // reset counters and other members that keep track of progress
    total_objs = 0;
    fds_uint32_t counter_zero = 0;
    std::atomic_store(&objs_done, counter_zero);

    // start garbage collection for this token -- tell persistent layer
    // to start routing requests to shadow (new) file to which we will
    // copy non-garbage objects
    persistGcHandler->notifyStartGc(cur_disk_id, token_id, cur_tier);

    /*
    // we may have writes currently in flight that are writing to old file.
    // If we take db snapshot before these in flight write finish and
    // metadata is updated, then we will miss data that needs to be copied
    // to new file (and lose it). We will wait on timer a few seconds so that
    // all IO currently outstanding are finished so that indexdb snapshot
    // will contain all object from the old file; we are ok to miss objects
    // in the new file, since we don't need to copy them
    if (!tc_timer->schedule(tc_timer_task, std::chrono::seconds(2))) {
        LOGNOTIFY << "Failed to schedule timer for db snapshot, completing now..";
        // We already created shadow file so, we are setting state
        // idle, and scavenger has to try again later
        // TODO(anna) need proper recovery in this case
        std::atomic_exchange(&state, TCSTATE_IDLE);
        return Error(ERR_NOT_READY);
    }
    */
    handleTimerEvent();

    return err;
}

void TokenCompactor::handleTimerEvent()
{
    tcStateType cur_state = std::atomic_load(&state);
    if (cur_state == TCSTATE_PREPARE_WORK) {
        enqSnapDbWork();
    } else {
        handleCompactionDone(ERR_OK);
    }
}

void TokenCompactor::enqSnapDbWork()
{
    Error err(ERR_OK);
    tcStateType expect = TCSTATE_PREPARE_WORK;
    if (!std::atomic_compare_exchange_strong(&state, &expect, TCSTATE_IN_PROGRESS)) {
        LOGERROR << "enqSnapDbWork is called in wrong state!";
        return;
    }

    // send request to do object db snapshot that we will work with
    snap_req.token_id = token_id;
    err = data_store->enqueueMsg(FdsSysTaskQueueId, &snap_req);
    if (!err.ok()) {
        LOGERROR << "Failed to enqueue take index db snapshot message ;" << err;
        // We already created shadow file
        // TODO(anna) should we just retry here? reschedule timer?
        handleCompactionDone(err);
    }
}

/**
 * Enqueue request for objects copy/delete
 */
Error TokenCompactor::enqCopyWork(std::vector<ObjectID>* obj_list, ContinueWorkFn nextWork)
{
    Error err(ERR_OK);
    SmIoCompactObjects* copy_req = new SmIoCompactObjects();
    copy_req->io_type = FDS_SM_COMPACT_OBJECTS;
    (copy_req->oid_list).swap(*obj_list);
    copy_req->tier = cur_tier;
    copy_req->verifyData = verifyData;
    copy_req->smio_compactobj_resp_cb = std::bind(
        &TokenCompactor::objsCompactedCb, this,
        std::placeholders::_1, std::placeholders::_2, nextWork);

    // enqueue to qos queue, copy_req will be delete after it's dequeued
    // and processed
    tcStateType cur_state = std::atomic_load(&state);
    if (cur_state != TCSTATE_IN_PROGRESS) {
        LOGERROR << "Failed to enqueue copy objs request. TC not in valid state.";
        err = ERR_SM_TC_INVALID_STATE;
        return err;
    }
    err = data_store->enqueueMsg(FdsSysTaskQueueId, copy_req);
    if (!err.ok()) {
        LOGERROR << "Failed to enqueue copy objs request, error " << err;
    }
    return err;
}

/**
 * Callback from object store that metadata snapshot is complete
 * We prepare work items with object id lists to copy/delete
 */
void TokenCompactor::snapDoneCb(const Error& error,
                                SmIoSnapshotObjectDB* snap_req,
                                leveldb::ReadOptions& options,
                                std::shared_ptr<leveldb::DB> db)
{
    Error err(ERR_OK);

    LOGDEBUG << "snapshot done for token:" << token_id
             << " received with result:" << error;
    fds_assert(total_objs == 0);  // smth went wrong, we set work only once

    // we must be in IN_PROGRESS state
    tcStateType cur_state = std::atomic_load(&state);
    fds_assert(cur_state == TCSTATE_IN_PROGRESS);

    if (!error.ok() || cur_state != TCSTATE_IN_PROGRESS || total_objs != 0) {
        LOGERROR << "Failed get snapshot for token " << token_id
                 << " with error " << error
                 << " current TC state " << cur_state;
        handleCompactionDone(error);
        return;
    }

    // iterate over snapshot of index db and create work items to work with
    leveldb::Iterator* it = db->NewIterator(options);

    ObjMetaData omd;
    std::shared_ptr<loc_oid_map_t> loc_oid_map = std::make_shared<loc_oid_map_t>();

    LOGDEBUG << "Building object list to compact... it valid? " << it->Valid();

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        ObjectID id(it->key().ToString());

        omd.deserializeFrom(it->value());
        // TOOD(Andrew): Remove this const cast when all of the loc
        // functions (e.g., is_shadow) handle a const variable
        obj_phy_loc_t* loc = const_cast<obj_phy_loc_t *>(omd.getObjPhyLoc(cur_tier));
        // we only care about the tier this compactor is working on
        fds_assert(loc != nullptr);
        if (loc == nullptr || loc->obj_tier != cur_tier) {
            continue;
        }

        // filter out objects that are already in shadow file --
        // this could happen between times we started writing objs
        // to shadow file and we took this db snapshot
        if (persistGcHandler->isShadowLocation(cur_disk_id, loc, token_id)) {
            LOGDEBUG << id << " already in shadow file (disk_id "
                << loc->obj_stor_loc_id << " file_id "
                << loc->obj_file_id << " tok " << token_id
                << " tier " << (fds_int16_t)loc->obj_tier << ")";
            continue;
        }

        // we group objects by their <phy loc, fileid> and order by offset
        // so that we compact objects that are close in the token file together
        fds_uint32_t loc_fid = loc->obj_stor_loc_id;
        loc_fid |= (loc->obj_file_id << 16);
        LOGDEBUG << "Object " << id << " loc_id " << loc->obj_stor_loc_id
            << " fileId " << loc->obj_file_id << "("
            << loc_fid << ") offset " << loc->obj_stor_offset
            << " tier " << (fds_int16_t)loc->obj_tier
            << " tok " << token_id;

        fds_assert(!(loc_oid_map->count(loc_fid)
            && (*loc_oid_map)[loc_fid].count(loc->obj_stor_offset)));
        if (loc_oid_map->count(loc_fid)
            && (*loc_oid_map)[loc_fid].count(loc->obj_stor_offset)) {
            LOGWARN << "Entry for object " << id << " already exists in the shadow file.";
        }

        ((*loc_oid_map)[loc_fid])[loc->obj_stor_offset] = id;
    }

    // calculate total_objs before enqueueing copy work into QoS queue,
    // to make sure that total_objs is correct when we get copy done cb
    for (loc_oid_map_t::const_iterator cit = loc_oid_map->cbegin();
         cit != loc_oid_map->cend();
         ++cit) {
        total_objs += (cit->second).size();
    }

    delete it;
    db->ReleaseSnapshot(options.snapshot);

    loc_oid_map_t::const_iterator cit = loc_oid_map->cbegin();
    offset_oid_map_t::const_iterator cit2 = (cit->second).cbegin();
    compactionWorker(loc_oid_map, cit, cit2, false);
}

void TokenCompactor::compactionWorker(std::shared_ptr<loc_oid_map_t> loc_oid_map,
                                      loc_oid_map_t::const_iterator cit,
                                      offset_oid_map_t::const_iterator cit2,
                                      bool last_run) {

    if (last_run) {
        loc_oid_map.reset();

        if (total_objs == 0) {
            handleCompactionDone(Error(ERR_OK));
        }
        return;
    }

    fds_uint32_t offset = 0;
    std::vector<ObjectID> obj_list;

    Error err(ERR_OK);

    // create copy work items
    for (cit; cit != loc_oid_map->cend(); ++cit) {
        for (cit2; cit2 != (cit->second).cend() && obj_list.size() <= GC_COPY_WORKLIST_SIZE; ++cit2) {

            LOGDEBUG << "Pushing " << cit2->second;
            obj_list.push_back(cit2->second);
        }
        // if we collected enough oids, create copy req and put it to qos queue
        if (obj_list.size() >= GC_COPY_WORKLIST_SIZE) {

            ContinueWorkFn nextWork = std::bind(&TokenCompactor::compactionWorker, this,
                                                loc_oid_map, cit, cit2, false);

            LOGDEBUG << "Enqueue copy work for " << obj_list.size() << " objects";
            err = enqCopyWork(&obj_list, nextWork);

            return;
        }
    }

    // send copy request for the remaining objects
    if (obj_list.size() > 0) {
        LOGDEBUG << "Enqueue copy work for " << obj_list.size() << " objects (this should be < "
                    << GC_COPY_WORKLIST_SIZE << " )";

        ContinueWorkFn nextWork = std::bind(&TokenCompactor::compactionWorker, this, loc_oid_map, cit, cit2, true);
        err = enqCopyWork(&obj_list, nextWork);
    } else {
        compactionWorker(loc_oid_map, cit, cit2, true);
    }
}


//
// Notification that set of objects were compacted
// Update progress counters and finish token compaction process
// if we finished compaction of all objects
//
void TokenCompactor::objsCompactedCb(const Error& error,
                                     SmIoCompactObjects* req,
                                     ContinueWorkFn nextWork)
{
    fds_assert(req != nullptr);
    fds_uint32_t done_before, total_done;

    // we must be in IN_PROGRESS state
    tcStateType cur_state = std::atomic_load(&state);

    if (!error.ok() ||
        cur_state != TCSTATE_IN_PROGRESS ||
        req == nullptr) {
        LOGERROR << "Failed TC for token " << token_id
                 << " tier " << (fds_uint16_t)cur_tier
                 << " disk_id " << cur_disk_id
                 << " with error " << error;
        delete req;
        handleCompactionDone(error);
        return;
    }

    fds_uint32_t work_objs_done = (req->oid_list).size();

    // account for progress and see if token compaction is done
    done_before = std::atomic_fetch_add(&objs_done, work_objs_done);
    total_done = done_before + work_objs_done;
    fds_assert(total_done <= total_objs);
    if (!(total_done <= total_objs)) {
        LOGERROR << "Failed TC for token " << token_id
                 << " tier " << (fds_uint16_t)cur_tier
                 << " disk_id " << cur_disk_id
                 << " with error " << ERR_INVALID_ARG
                 << " done " << total_done << " total " << total_objs;
        delete req;
        handleCompactionDone(ERR_INVALID_ARG);
        return;
    }

    LOGDEBUG << "Finished compaction of " << work_objs_done << " objects"
             << ", done so far " << total_done << " out of " << total_objs
             << " (tok " << token_id << " tier " << (fds_uint16_t)cur_tier
             << " disk_id " << cur_disk_id;

    if (total_done == total_objs) {
        // we are done!
        // NOTE: we are currently doing completion on timer only! when no error
        // otherwise need to make sure to remember the error
        /*
        if (!tc_timer->schedule(tc_timer_task, std::chrono::seconds(2))) {
            LOGNOTIFY << "Failed to schedule completion timer, completing now..";
            handleCompactionDone(error);
        }
        */
        handleTimerEvent();
        delete req;
    }

    nextWork();
}

Error TokenCompactor::handleCompactionDone(const Error& tc_error)
{
    Error err(ERR_OK);
    err = tc_error;
    fiu_do_on("sm.tc.fail.compaction",\
              LOGDEBUG << "sm.tc.fail.compaction fault point enabled";\
              if (err.ok()) { err = ERR_NOT_FOUND; }\
              fiu_disable("sm.tc.fail.compaction"));

    tcStateType expect = TCSTATE_IN_PROGRESS;
    tcStateType new_state = TCSTATE_IDLE;
    if (std::atomic_compare_exchange_strong(&state, &expect, new_state)) {
        LOGNORMAL << "Finished compaction for token:" << token_id
                  << " disk:" << cur_disk_id
                  << " tier:" << cur_tier
                  << " verify:" << verifyData
                  << " tc state:" << state
                  << " error:" << err;
        if (err.ok()) {
            // tell persistent layer we are done copying -- remove the old file
            persistGcHandler->notifyEndGc(cur_disk_id, token_id, cur_tier);
        }

        // notify the requester about the completion
        if (done_evt_handler) {
            done_evt_handler(token_id, err);
        }
    }
    return err;
}

//
// returns number from 0 to 100 (percent of progress, approx)
//
fds_uint32_t TokenCompactor::getProgress() const
{
    fds_uint32_t done = std::atomic_load(&objs_done);
    fds_assert(done <= total_objs);
    if (!(done <= total_objs)) {
        LOGERROR << "Failed to get TC progress for token " << token_id
                 << " tier " << (fds_uint16_t)cur_tier
                 << " disk_id " << cur_disk_id
                 << " with error " << ERR_INVALID_ARG
                 << " total done " << done << " total objects " << total_objs;
    }

    if (total_objs == 0) {
        return 100;
    }
    return done / total_objs;
}

fds_bool_t TokenCompactor::isIdle() const
{
    tcStateType cur_state = std::atomic_load(&state);
    return (cur_state == TCSTATE_IDLE);
}

//
// given object metadata, check if we can remove entry from index db
//
fds_bool_t TokenCompactor::isGarbage(const ObjMetaData& md)
{
    auto deleteThresh = fds::objDelCountThresh;
    if (md.getDeleteCount() >= deleteThresh) {
        return true;
    }

    return false;
}

//
// given object metadata, check if we need to garbage collect it
//
fds_bool_t TokenCompactor::isDataGarbage(const ObjMetaData& md,
                                         diskio::DataTier _tier)
{
    fds_bool_t do_gc = false;

    // obj is garbage if we don't need to keep it in system anymore
    if (isGarbage(md)) {
        return true;
    }

    if (md.isObjReconcileRequired()) {
        LOGCRITICAL << "SM Token Migration incomplete reconciliation:  "
                    << " This may not be a bug, but should be investigated:" << md;
        return true;
    }

    // obj is garbage if it does not exist on given tier
    if (!md.onTier(_tier)) {
        return true;
    }

    return do_gc;
}

void CompactorTimerTask::runTimerTask()
{
    fds_assert(tok_compactor);
    if (tok_compactor) { tok_compactor->handleTimerEvent(); }
}
}  // namespace fds
