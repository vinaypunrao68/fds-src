/*                                                                                                                                          
 * Copyright 2014 Formation Data Systems, Inc.                                                                                              
 */

#include <vector>
#include <map>
#include <TokenCompactor.h>

namespace fds {

typedef std::map<fds_uint32_t, ObjectID> offset_oid_map_t;
typedef std::map<fds_uint32_t, offset_oid_map_t> loc_oid_map_t;

TokenCompactor::TokenCompactor(SmIoReqHandler *_data_store)
        : token_id(0),
          done_evt_handler(NULL),
          data_store(_data_store)
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
}

//
// Creates a new file for the token in tokenFileDb to which non-deleted
// objects will be copied and new puts will go.
// Decides on object id ranges to compact at a time, and creates
// work items to actually get list of objects that need copy /delete
//
Error TokenCompactor::startCompaction(fds_token_id tok_id,
                                      fds_uint32_t bits_for_token,
                                      compaction_done_handler_t done_evt_hdlr)
{
    Error err(ERR_OK);
    diskio::DataIO& dio_mgr = diskio::DataIO::disk_singleton();
    tcStateType expect = TCSTATE_IDLE;
    if (!std::atomic_compare_exchange_strong(&state, &expect, TCSTATE_IN_PROGRESS)) {
        LOGNOTIFY << "startCompaction called in non-idle state, ignoring";
        return Error(ERR_NOT_READY);
    }

    // TODO(anna) do not do compaction if sync is in progress, return 'not ready'

    LOGNORMAL << "Start Compaction of token " << tok_id;

    // remember the token we are goint to work on and object id range for this
    // token -- to safeguard later that we are copying right objects
    token_id = tok_id;
    ObjectID::getTokenRange(token_id, bits_for_token, tok_start_oid, tok_end_oid);
    LOGDEBUG << "token range: " << tok_start_oid << " ... " << tok_end_oid;
    done_evt_handler = done_evt_hdlr;  // set cb to notify about completion

    // reset counters and other members that keep track of progress
    total_objs = 0;
    fds_uint32_t counter_zero = 0;
    std::atomic_store(&objs_done, counter_zero);

    // start garbage collection for this token -- tell persistent layer
    // to start routing requests to shadow (new) file to which we will
    // copy non-garbage objects
    // TODO(anna) initial implemetation compacts disk only file, need to
    // think if we need to do something different about SSD files
    dio_mgr.notify_start_gc(token_id, diskTier);

    // send request to do object db snapshot that we will work with
    snap_req.token_id = token_id;
    err = data_store->enqueueMsg(FdsSysTaskQueueId, &snap_req);
    if (!err.ok()) {
        LOGERROR << "Failed to enqueue take index db snapshot message"
                 << " error " << err.GetErrstr() << "; try again later";
        // We already created shadow file so, we are setting state
        // idle, and scavenger has to try again later
        // TODO(anna) need proper recovery in this case
        std::atomic_exchange(&state, TCSTATE_IDLE);
        return Error(ERR_NOT_READY);
    }

    return err;
}

/**
 * Enqueue request for objects copy/delete
 */
Error TokenCompactor::enqCopyWork(std::vector<ObjectID>* obj_list)
{
    Error err(ERR_OK);
    SmIoCompactObjects* copy_req = new SmIoCompactObjects();
    copy_req->io_type = FDS_SM_COMPACT_OBJECTS;
    (copy_req->oid_list).swap(*obj_list);
    copy_req->smio_compactobj_resp_cb = std::bind(
        &TokenCompactor::objsCompactedCb, this,
        std::placeholders::_1, std::placeholders::_2);

    // enqueue to qos queue, copy_req will be delete after it's dequeued
    // and processed
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
                                leveldb::DB *db)
{
    Error err(ERR_OK);
    ObjMetaData omd;
    fds_uint32_t offset = 0;
    diskio::DataIO& dio_mgr = diskio::DataIO::disk_singleton();

    LOGNORMAL << "Index DB snapshot received with result " << error;
    fds_verify(total_objs == 0);  // smth went wrong, we set work only once

    // we must be in IN_PROGRESS state
    tcStateType cur_state = std::atomic_load(&state);
    fds_verify(cur_state == TCSTATE_IN_PROGRESS);

    if (!error.ok()) {
        LOGERROR << "Failed to get snapshot of index db, cannot continue"
                 << " with token GC copy, completing with error";
        handleCompactionDone(error);
        return;
    }

    // iterate over snapshot of index db and create work items to work with
    std::vector<ObjectID> obj_list;
    loc_oid_map_t loc_oid_map;
    leveldb::Iterator* it = db->NewIterator(options);
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        ObjectID id(it->key().ToString());
        // this should not happen, but just in case
        if (id < tok_start_oid || id > tok_end_oid) {
            continue;
        }

        omd.deserializeFrom(it->value());
        obj_phy_loc_t* loc = omd.getObjPhyLoc(diskTier);
        fds_verify(loc != NULL);

        // TODO(anna) we must filter out objects that are already in shadow
        // file -- that could happen between times we started writing objs
        // to shadow file and we took this db snapshot

        // we group objects by their <phy loc, fileid> and order by offset
        // so that we compact objects that are close in the token file together
        fds_uint32_t loc_fid = loc->obj_stor_loc_id;
        loc_fid |= (loc->obj_file_id << 16);
        LOGDEBUG << "Object " << id << " loc_id " << loc->obj_stor_loc_id
                 << " fileId " << loc->obj_file_id << "("
                 << loc_fid << ") offset " << loc->obj_stor_offset;

        // verify may be broken if we also do ssd tier
        fds_verify(!(loc_oid_map.count(loc_fid)
                     && loc_oid_map[loc_fid].count(loc->obj_stor_offset)));

        (loc_oid_map[loc_fid])[loc->obj_stor_offset] = id;
    }

    // create copy work items
    for (loc_oid_map_t::const_iterator cit = loc_oid_map.cbegin();
         cit != loc_oid_map.cend();
         ++cit) {
        for (offset_oid_map_t::const_iterator cit2 = (cit->second).cbegin();
             cit2 != (cit->second).cend();
             ++cit2) {
            obj_list.push_back(cit2->second);
            ++total_objs;

            // if we collected enough oids, create copy req and put it to qos queue
            if (obj_list.size() >= GC_COPY_WORKLIST_SIZE) {
                LOGDEBUG << "Enqueue copy work for " << obj_list.size() << " objects";
                err = enqCopyWork(&obj_list);
                if (!err.ok()) {
                    // TODO(anna): most likely queue is full, we need to save the
                    // work and try again later
                    fds_verify(false);  // IMPLEMENT THIS
                }
                obj_list.clear();
            }
        }
    }

    // send copy request for the remaining objects
    if (obj_list.size() > 0) {
        LOGDEBUG << "Enqueue copy work for " << obj_list.size() << " objects";
        err = enqCopyWork(&obj_list);
        if (!err.ok()) {
            // TODO(anna): most likely queue is full, we need to save the
            // work and try again later
            fds_verify(false);  // IMPLEMENT THIS
        }
    }

    // cleanup
    delete it;
    db->ReleaseSnapshot(options.snapshot);

    if (total_objs == 0) {
        // there are no objects in index db, we are done
        handleCompactionDone(Error(ERR_OK));
    }
}

//
// Notification that set of objects were compacted
// Update progress counters and finish token compaction process
// if we finished compaction of all objects
//
void TokenCompactor::objsCompactedCb(const Error& error,
                                     SmIoCompactObjects* req)
{
    fds_verify(req != NULL);
    fds_uint32_t done_before, total_done;
    fds_uint32_t work_objs_done = (req->oid_list).size();

    // we must be in IN_PROGRESS state
    tcStateType cur_state = std::atomic_load(&state);
    fds_verify(cur_state == TCSTATE_IN_PROGRESS);

    if (!error.ok()) {
        LOGERROR << "Failed to compact a set of objects, cannot continue"
                 << " with token GC copy, completing with error " << error;
        handleCompactionDone(error);
        return;
    }

    // account for progress and see if token compaction is done
    done_before = std::atomic_fetch_add(&objs_done, work_objs_done);
    total_done = done_before + work_objs_done;
    fds_verify(total_done <= total_objs);

    LOGNORMAL << "Finished compaction of " << work_objs_done << " objects"
              << ", done so far " << total_done << " out of " << total_objs;

    if (total_done == total_objs) {
        // we are done!
        handleCompactionDone(error);
    }
}

Error TokenCompactor::handleCompactionDone(const Error& tc_error)
{
    Error err(ERR_OK);
    diskio::DataIO& dio_mgr = diskio::DataIO::disk_singleton();

    tcStateType expect = TCSTATE_IN_PROGRESS;
    tcStateType new_state = tc_error.ok() ? TCSTATE_DONE : TCSTATE_ERROR;
    if (!std::atomic_compare_exchange_strong(&state, &expect, new_state)) {
        // must not happen
        fds_verify(false);
    }

    LOGNORMAL << "Compaction finished for token " << token_id
              << ", result " << tc_error;

    // check error happened in the middle of compaction
    if (!tc_error.ok()) {
        // TODO(anna) need to recover from error because we may have already wrote
        // new objects to the shadow file!
        fds_verify(false);  // IMPLEMENT THIS
    }

    // tell persistent layer we are done copying -- remove the old file
    dio_mgr.notify_end_gc(token_id, diskTier);

    // set token compactor state to idle -- scavenger can use this TokenCompactor
    // for a new compactino job
    std::atomic_exchange(&state, TCSTATE_IDLE);

    // notify the requester about the completion
    if (done_evt_handler) {
        done_evt_handler(token_id, Error(ERR_OK));
    }

    return err;
}

//
// returns number from 0 to 100 (percent of progress, approx)
//
fds_uint32_t TokenCompactor::getProgress() const
{
    fds_uint32_t done = std::atomic_load(&objs_done);
    fds_verify(done <= total_objs);

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
// given object metadata, check if we need to garbage collect it
//
fds_bool_t TokenCompactor::isGarbage(const ObjMetaData& md)
{
    fds_bool_t do_gc = false;

    // TODO(anna or Vinay) add other policies for checking GC
    // The first version of this method just decides based on
    // refcount -- if < 1 then garbage collect
    if (md.getRefCnt() < 1) {
        do_gc = true;
    }

    return do_gc;
}
}  // namespace fds
