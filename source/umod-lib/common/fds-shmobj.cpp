/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <algorithm>
#include <vector>
#include <util/Log.h>
#include <fds-shmobj.h>
#include <platform/fds_shmem.h>

#include "platform/node_shm_ctrl.h"

namespace fds {

/*
 * ------------------------------------------------------------------------------------
 * Shared Memory RO Obj Manager
 * ------------------------------------------------------------------------------------
 */
ShmObjRO::ShmObjRO(const FdsShmem *shm,
                   size_t          off,
                   size_t          key_siz,
                   size_t          key_off,
                   size_t          obj_siz,
                   int             obj_cnt) : shm_ctrl(shm)
{
    fds_verify((key_siz > 0) && (obj_siz >= key_siz) && (obj_cnt > 0));

    shm_area       = static_cast<const char *>(shm->shm_area()) + off;
    shm_area_bound = shm_area + (obj_siz * obj_cnt);
    shm_key_siz    = key_siz;
    shm_key_off    = key_off;
    shm_obj_siz    = obj_siz;
    shm_obj_cnt    = obj_cnt;
}

// shm_lookup_rec
// --------------
//
int
ShmObjRO::shm_lookup_rec(const void *key, void *rec, size_t rec_sz) const
{
    int         idx;
    size_t      off;
    const char *cur, *k;

    fds_assert(rec_sz == shm_obj_siz);
    k = static_cast<const char *>(key);

    off = 0;
    for (idx = 0; idx < shm_obj_cnt; idx++, off += shm_obj_siz) {
        cur = shm_area + off;
        if (obj_key_cmp(k, cur + shm_key_off, shm_key_siz) == 0) {
            /* Found the match */
            memcpy(rec, cur, shm_obj_siz);
            break;
        }
    }
    if (idx == shm_obj_cnt) {
        memset(rec, 0, shm_obj_siz);
        idx = -1;
    }
    return idx;
}

int
ShmObjRO::shm_lookup_rec(int idx, const void *key, void *rec, size_t rec_sz) const
{
    const char *cur, *k;

    fds_verify((0 <= idx) && (idx < shm_obj_cnt));
    cur = shm_area + (idx * shm_obj_siz);
    if (key != NULL) {
        /* Verify the key if the index is wrong. */
        if (obj_key_cmp(static_cast<const char *>(key),
                        cur + shm_key_off, shm_key_siz) != 0) {
            return -1;
        }
    }
    memcpy(rec, cur, shm_obj_siz);
    return idx;
}

// shm_iter_objs
// -------------
//
int
ShmObjRO::shm_iter_objs(ShmObjIter *iter) const
{
    int         idx;
    size_t      off;
    const char *cur, *key;

    off = 0;
    for (idx = 0; idx < shm_obj_cnt; idx++, off += shm_obj_siz) {
        cur = shm_area + off;
        key = cur + shm_key_off;

        iter->iter_cnt++;
        if (iter->shm_obj_iter_fn(idx, key, cur, shm_key_siz, shm_obj_siz) == false) {
            break;
        }
    }
    return iter->iter_cnt;
}

// obj_key_cmp
// -----------
//
int
ShmObjRO::obj_key_cmp(const char *a1, const char *a2, size_t size) const
{
    return memcmp(a1, a2, size);
}

// obj_key_invalid
// ---------------
//
bool
ShmObjRO::obj_key_invalid(const char *key, size_t size) const
{
    const fds_uint32_t *k;

    k = reinterpret_cast<const fds_uint32_t *>(key);
    if (size >= (2 * sizeof(*k))) {
        return (k[0] == 0) && (k[1] == 0);
    }
    return k[0] == 0;
}

int
ShmObjROKeyUint64::obj_key_cmp(const char *a1, const char *a2, size_t size) const
{
    const fds_uint64_t *k1, *k2;

    k1 = reinterpret_cast<const fds_uint64_t *>(a1);
    k2 = reinterpret_cast<const fds_uint64_t *>(a2);
    return (*k1 - *k2);
}

int
ShmObjRWKeyUint64::obj_key_cmp(const char *a1, const char *a2, size_t size) const
{
    const fds_uint64_t *k1, *k2;

    k1 = reinterpret_cast<const fds_uint64_t *>(a1);
    k2 = reinterpret_cast<const fds_uint64_t *>(a2);
    return (*k1 - *k2);
}

/*
 * ------------------------------------------------------------------------------------
 * Shared Memory RW Obj Manager
 * ------------------------------------------------------------------------------------
 */
ShmObjRW::ShmObjRW(const FdsShmem *shm,
                   size_t          off,
                   size_t          key_siz,
                   size_t          key_off,
                   size_t          obj_siz,
                   int             obj_cnt)
    : ShmObjRO(shm, off, key_siz, key_off, obj_siz, obj_cnt), rw_mtx()
{
    shm_rw_area = static_cast<char *>(shm_ctrl->shm_area()) + off;
    fds_assert(shm_rw_area == shm_area);
}

// shm_insert_rec
// --------------
//
int
ShmObjRW::shm_insert_rec(int idx, const void *data, size_t rec_sz)
{
    return 0;
}

// shm_insert_rec
// --------------
//
int
ShmObjRW::shm_insert_rec(const void *key, const void *data, size_t dat_siz)
{
    int         idx, empty;
    size_t      off;
    char       *cur;
    const char *k;

    fds_verify(dat_siz == shm_obj_siz);
    k     = static_cast<const char *>(key);
    off   = 0;
    empty = -1;

    rw_mtx.lock();
    for (idx = 0; idx < shm_obj_cnt; idx++, off += shm_obj_siz) {
        cur = shm_rw_area + off;
        if (obj_key_cmp(k, cur + shm_key_off, shm_key_siz) == 0) {
            /* Fond existing slot, overwrite it with the new record. */
            memcpy(cur, data, shm_obj_siz);
            break;
        }
        if (obj_key_invalid(cur + shm_key_off, shm_key_siz) == true) {
            if (empty == -1) {
                empty = idx;
            }
        }
    }
    if (idx == shm_obj_cnt) {
        if (empty != -1) {
            idx = empty;
            cur = shm_rw_area + (empty * shm_obj_siz);
            memcpy(cur, data, shm_obj_siz);
        } else {
            idx = -1;
        }
    }
    rw_mtx.unlock();
    return idx;
}

// shm_remove_rec
// --------------
//
int
ShmObjRW::shm_remove_rec(const void *key, void *data, size_t rec_sz)
{
    int         idx;
    size_t      off;
    char       *cur;
    const char *k;

    k   = static_cast<const char *>(key);
    off = 0;

    rw_mtx.lock();
    for (idx = 0; idx < shm_obj_cnt; idx++, off += shm_obj_siz) {
        cur = shm_rw_area + off;
        if (obj_key_cmp(k, cur + shm_key_off, shm_key_siz) == 0) {
            /* Found the matching record, remove it. */
            if (data != NULL) {
                fds_verify(rec_sz == shm_obj_siz);
                memcpy(data, cur, shm_obj_siz);
            }
            memset(cur, 0, shm_obj_siz);
            break;
        }
    }
    rw_mtx.unlock();
    return idx;
}

// shm_remove_rec
// --------------
//
int
ShmObjRW::shm_remove_rec(int idx, const void *key, void *data, size_t rec_sz)
{
    char       *cur;
    const char *k;

    fds_verify((0 <= idx) && (idx < shm_obj_cnt));
    k = static_cast<const char *>(key);

    rw_mtx.lock();
    cur = shm_rw_area + (idx * shm_obj_siz);
    if (key != NULL) {
        fds_verify(obj_key_cmp(k, cur + shm_key_off, shm_key_siz) == 0);
    }
    if (data != NULL) {
        fds_verify(rec_sz == shm_obj_siz);
        memcpy(data, cur, shm_obj_siz);
    }
    rw_mtx.unlock();
    return idx;
}

// Queue stuff
// -----------
//
ShmConPrdQueue::ShmConPrdQueue(shm_con_prd_sync_t *sync, ShmObjRW *data)
        : fdsio::RequestQueue(1, -1), smq_sync(sync), smq_data(data),
          smq_size(NodeShmCtrl::shm_q_item_count),
          smq_itm_size(NodeShmCtrl::shm_q_item_size)
{
}


// Initialize reqID
fds_uint32_t ShmConPrdQueue::smq_reqID = 0;

void
ShmConPrdQueue::shm_track_request(ShmqReqOut *out, shmq_req_t *hdr, int caller)
{
    // Assign the unique tracking id and submit the 'out' request to this queue.
    // Map this id to the 'out' object so that we can retrieve it later.
    //
    hdr->smq_code |= SHMQ_TRACK_MASK;
    hdr->smq_idx = caller;
    hdr->smq_payload_size = 0;

    // Mutex down so we don't duplicate IDs by accident
    pthread_mutex_lock(&smq_sync->shm_mtx);
    hdr->smq_id = ShmConPrdQueue::smq_reqID++;
    smq_reqOutMap[hdr->smq_id] = out;
    // Mutex up
    pthread_mutex_unlock(&smq_sync->shm_mtx);

    out->req_out = *hdr;
    this->rq_enqueue(out, 0);
}

/* static */ void
ShmConPrdQueue::shm_swap_req_header(shmq_req_t *x, shmq_req_t *y)
{
    fds_assert((x != nullptr) && (y != nullptr));
    shmq_req_t temp;
    memcpy(static_cast<void *>(&temp), static_cast<void *>(x), sizeof(shmq_req_t));
    memcpy(static_cast<void *>(x), static_cast<void *>(y), sizeof(shmq_req_t));
    memcpy(static_cast<void *>(y), static_cast<void *>(&temp), sizeof(shmq_req_t));
}

void
ShmConPrdQueue::shm_register_handler(int smq_code, ShmqReqIn *cb)
{
    // XXX: Threadsafe? Don't think this one matters.
    // A new callback shouldn't be registered with an existing code?
    smq_cb_map[smq_code] = cb;
}

void
ShmConPrdQueue::shm_consume_loop(int consumer_idx)
{
    int          size = 128;     // take from parameter/constructor
    char         data[size];
    ShmqReqIn   *cb;
    shmq_req_t  *inp;

    inp = reinterpret_cast<shmq_req_t *>(data);
    while (1) {
        /* Block if we don't have anything to consume. */
        cb = NULL;
        shm_consumer(data, size, consumer_idx);

        if (inp->smq_code & SHMQ_TRACK_MASK) {
            /* Lookup the saved request in shm_track_request() */
            if (inp->smq_idx == consumer_idx) {
                ShmqReqOut *orig = shm_get_saved_req(inp->smq_id);
                if (orig != NULL) {
                    /* Notify the sender, this also takes it out of the queue. */
                    fds_verify(orig->req_size <= (size_t)size);
                    memcpy(orig->req_in, data, orig->req_size);
                    orig->req_complete();
                    continue;
                }
            }
            /* Clear the mask and lookup registered handler. */
            cb = shm_get_callback(inp->smq_code & ~SHMQ_TRACK_MASK);
        } else {
            cb = shm_get_callback(inp->smq_code);
        }
        LOGDEBUG << "Recv shm: " << inp->smq_code << ", " << inp->smq_idx <<
            ", id " << inp->smq_id;

        if (cb != NULL) {
            cb->shmq_handler(inp, size);
        }
    }
}

ShmqReqOut *
ShmConPrdQueue::shm_get_saved_req(fds_uint32_t id)
{
    // Try to find the key in the map
    try {
        std::unordered_map<fds_uint32_t, ShmqReqOut*>::const_iterator found =
                smq_reqOutMap.find(id);
        // If the key is not found return nullptr
        if ( found == smq_reqOutMap.end() ) {
            return nullptr;
        }
        // Else return the pointer to the correct ShmqReqOut
        return found->second;
    } catch(int e) {
        return nullptr;
    }
}

ShmqReqIn *
ShmConPrdQueue::shm_get_callback(fds_uint32_t code)
{
    try {
        std::unordered_map<fds_uint32_t, ShmqReqIn*>::const_iterator found =
                smq_cb_map.find(code);
        if ( found == smq_cb_map.end() ) {
            return nullptr;
        }
        return found->second;
    } catch(int e) {
        return nullptr;
    }
}

Shm_1Prd_nCon::Shm_1Prd_nCon(shm_con_prd_sync_t *sync,
                             shm_1prd_ncon_q_t *ctrl, ShmObjRW *data)
    : ShmConPrdQueue(sync, data), smq_ctrl(ctrl) {}

void
Shm_1Prd_nCon::shm_init_consumer_queue()
{
    // XXX: Threadsafe? This probably shouldn't be called
    // if consumers are already initialized, and likely does not
    // need to be threadsafe
    // -------

    // For each index, set it to -1
    for (int i = 0; i < smq_ctrl->shm_ncon_cnt; ++i) {
        smq_ctrl->shm_ncon_idx[i] = -1;
    }
}

void
Shm_1Prd_nCon::shm_activate_consumer(int consumer)
{
    // Consumer index should be -1 to perform this operation
    // (smq_ctrl->shm_ncon_idx[consumer] == -1)
    // Mutex down
    pthread_mutex_lock(&smq_sync->shm_mtx);
    // Set consumer idx to prod idx
    smq_ctrl->shm_ncon_idx[consumer] = smq_ctrl->shm_1prd_idx;
    // Mutex up
    pthread_mutex_unlock(&smq_sync->shm_mtx);
}

int
Shm_1Prd_nCon::shm_calc_delta(int consumer)
{
    // Note: This should only be called while holding the mutex
    int delta = 0;
    // If cns < prd: prd - cns
    if (smq_ctrl->shm_ncon_idx[consumer] < smq_ctrl->shm_1prd_idx) {
        delta = smq_ctrl->shm_1prd_idx - smq_ctrl->shm_ncon_idx[consumer];
    // If prd < cns: (queue_size - cns_index) + prd_index
    } else if (smq_ctrl->shm_ncon_idx[consumer] > smq_ctrl->shm_1prd_idx) {
        delta = (smq_size - smq_ctrl->shm_ncon_idx[consumer]) +
                smq_ctrl->shm_1prd_idx;
    }
    return delta;
}

int
Shm_1Prd_nCon::shm_calc_max_delta()
{
    // Get highest value delta
    int delta, high_delta = -1;
    // For each consumer
    for (int i = 0; i < smq_ctrl->shm_ncon_cnt; ++i) {
        // If the index is -1 cns not active, pass
        if (smq_ctrl->shm_ncon_idx[i] == -1) {
            continue;
        }
        // Calc the delta
        delta = shm_calc_delta(i);
        // Calc the max delta
        if (delta > high_delta) {
            high_delta = delta;
        }
    }
    return high_delta;
}

void
Shm_1Prd_nCon::shm_consumer(void *data, size_t size, int consumer /* = 0 */)
{
    fds_assert(consumer >= 0);
    fds_assert(consumer < smq_ctrl->shm_ncon_cnt);
    fds_assert(data != nullptr);
    fds_assert(size > 0);

    int ret_code = -1;
    // Mutex down
    ret_code = pthread_mutex_lock(&smq_sync->shm_mtx);
    fds_assert(0 == ret_code);

    // Calculate delta
    int delta = shm_calc_delta(consumer);

    // We know the queue is empty if our delta == 0
    while (delta == 0) {
        // Wait on the consumer condition variable
        pthread_cond_wait(&smq_sync->shm_con_cv, &smq_sync->shm_mtx);
        // Update delta -- we should still be holding the mutex
        delta = shm_calc_delta(consumer);
    }
    // Take a request from the queue
    void *from_ptr = static_cast<char *>(smq_data->shm_rw_base()) +
            (smq_ctrl->shm_ncon_idx[consumer] * smq_itm_size);
    fds_assert(from_ptr <=
               (static_cast<const char *>(smq_data->shm_bound()) - size));

    memcpy(data, from_ptr, size);

    // Increase this queue's index counter
    smq_ctrl->shm_ncon_idx[consumer] =
            (smq_ctrl->shm_ncon_idx[consumer] + 1) % smq_size;

    // Signal to producer that we've consumed
    ret_code = pthread_cond_signal(&smq_sync->shm_prd_cv);
    fds_assert(0 == ret_code);
    // Mutex up
    ret_code = pthread_mutex_unlock(&smq_sync->shm_mtx);
    fds_assert(0 == ret_code);
}

void
Shm_1Prd_nCon::shm_producer(const void *data, size_t size, int producer /* = 0 */)
{
    fds_assert(data != nullptr);
    fds_assert(size > 0);

    // In this case we need to be careful that all of the queues
    // have progressed before we can write a new entry
    // -----------
    // Mutex down
    pthread_mutex_lock(&smq_sync->shm_mtx);

    // Get highest value delta
    int high_delta = -1;
    high_delta = shm_calc_max_delta();

    // If high_delta is still -1 then no active consumers
    if (high_delta > -1) {
        // We know the producer cannot continue if the greatest delta is == smq_size
        while (high_delta == (static_cast<int>(smq_size) - 1)) {
            // Wait on the producer condition variable
            pthread_cond_wait(&smq_sync->shm_prd_cv, &smq_sync->shm_mtx);
            // Recalculate the high_delta -- we should still have the mutex
            high_delta = shm_calc_max_delta();
        }
    }
    // Add new data to the queue
    void *out_ptr = static_cast<char *>(smq_data->shm_rw_base()) +
        (smq_ctrl->shm_1prd_idx * smq_itm_size);
    fds_assert(out_ptr >= smq_data->shm_rw_base());
    fds_assert(out_ptr <=
               (static_cast<const char *>(smq_data->shm_bound()) - smq_itm_size));

    memcpy(out_ptr, data, size);
    // Increase the prd_index
    smq_ctrl->shm_1prd_idx = (smq_ctrl->shm_1prd_idx + 1) % smq_size;

    // Signal to consumers that there is more stuff
    pthread_cond_broadcast(&smq_sync->shm_con_cv);
    pthread_mutex_unlock(&smq_sync->shm_mtx);
}


Shm_nPrd_1Con::Shm_nPrd_1Con(shm_con_prd_sync_t *sync,
                             shm_nprd_1con_q_t *ctrl, ShmObjRW *data)
    : ShmConPrdQueue(sync, data), smq_ctrl(ctrl) {}

void
Shm_nPrd_1Con::shm_init_consumer_queue()
{
    // XXX: Threadsafe? This probably shouldn't be called
    // if consumers are already initialized, and likely does not
    // need to be threadsafe
    // -------
    smq_ctrl->shm_1con_idx = -1;
}

void
Shm_nPrd_1Con::shm_activate_consumer(int cons)
{
    // con idx should be -1 to perform this operation
    // fds_assert(smq_ctrl->shm_1con_idx == -1);

    int ret_code = -1;
    // Mutex down
    ret_code = pthread_mutex_lock(&smq_sync->shm_mtx);
    fds_assert(0 == ret_code);
    // Set consumer idx to prod idx
    smq_ctrl->shm_1con_idx = smq_ctrl->shm_nprd_idx;
    // Mutex up
    ret_code = pthread_mutex_unlock(&smq_sync->shm_mtx);
    fds_assert(0 == ret_code);
}

void
Shm_nPrd_1Con::shm_consumer(void *data, size_t size, int consumer /* = 0 */)
{
    fds_assert(data != nullptr);
    fds_assert(size > 0);

    int ret_code = -1;
    // Mutex down
    ret_code = pthread_mutex_lock(&smq_sync->shm_mtx);
    fds_assert(0 == ret_code);
    // We know the queue is empty if the consumer index is equal to
    // the producer index
    while ( smq_ctrl->shm_nprd_idx == smq_ctrl->shm_1con_idx ) {
        // Wait on the consumer condition variable
        pthread_cond_wait(&smq_sync->shm_con_cv, &smq_sync->shm_mtx);
    }
    // Take a request from the queue
    void *from_ptr = static_cast<char *>(smq_data->shm_rw_base()) +
            (smq_ctrl->shm_1con_idx * smq_itm_size);
    fds_assert(from_ptr <=
               (static_cast<const char *>(smq_data->shm_bound()) -
                smq_itm_size));

    memcpy(data, from_ptr, size);
    // Increase this queue's index counter
    smq_ctrl->shm_1con_idx =
            (smq_ctrl->shm_1con_idx + 1) % smq_size;

    // Signal to producer that we've consumed
    ret_code = pthread_cond_broadcast(&smq_sync->shm_prd_cv);
    fds_assert(0 == ret_code);
    // Mutex up
    ret_code = pthread_mutex_unlock(&smq_sync->shm_mtx);
    fds_assert(0 == ret_code);
}
void
Shm_nPrd_1Con::shm_producer(const void *data, size_t size, int producer /* = 0 */)
{
    int prod;

    fds_assert(data != nullptr);
    fds_assert(size > 0);

    // Mutex down
    pthread_mutex_lock(&smq_sync->shm_mtx);
    prod = (smq_ctrl->shm_nprd_idx + 1) % smq_size;
    while (prod == smq_ctrl->shm_1con_idx) {
        // Wait on the producer condition variable
        pthread_cond_wait(&smq_sync->shm_prd_cv, &smq_sync->shm_mtx);
        prod = (smq_ctrl->shm_nprd_idx + 1) % smq_size;
    }
    // Add new data to the queue
    void *out_ptr = static_cast<char *>(smq_data->shm_rw_base()) +
        (smq_ctrl->shm_nprd_idx * smq_itm_size);
    fds_assert(out_ptr <=
               (static_cast<const char *>(smq_data->shm_bound()) - smq_itm_size));

    memcpy(out_ptr, data, size);
    // Increase the prd_index
    smq_ctrl->shm_nprd_idx = prod;

    // Signal to consumers that there is more stuff
    pthread_cond_broadcast(&smq_sync->shm_con_cv);
    pthread_mutex_unlock(&smq_sync->shm_mtx);
}

}  // namespace fds
