/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <fds-shmobj.h>
#include <platform/fds-shmem.h>

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
        /* Verify the key. */
        fds_verify(obj_key_cmp(static_cast<const char *>(key),
                               cur + shm_key_off, shm_key_siz) == 0);
    }
    memcpy(rec, cur, shm_obj_siz);
    return idx;
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
            shm_obj_cnt++;
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
                shm_obj_cnt--;
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
    shm_obj_cnt--;
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
ShmConPrdQueue::ShmConPrdQueue(shm_con_prd_sync_t *sync, shm_1prd_ncon_q_t *ctrl, ShmObjRW *data)
    : fdsio::RequestQueue(1, -1)
{
}

ShmConPrdQueue::ShmConPrdQueue(shm_con_prd_sync_t *sync, shm_nprd_1con_q_t *ctrl, ShmObjRW *data)
    : fdsio::RequestQueue(1, -1)
{
}

void
ShmConPrdQueue::shm_track_request(ShmqReqOut *out, shmq_req_t *hdr)
{
    // Assign the unique tracking id and submit the 'out' request to this queue.
    // Map this id to the 'out' object so that we can retrieve it later.
    //
    hdr->smq_id    = 0;
    hdr->smq_code |= SHMQ_TRACK_MASK;

    out->req_out = *hdr;
    this->rq_enqueue(out, 0);
}

/* static */ void
ShmConPrdQueue::shm_swap_req_header(shmq_req_t *x, shmq_req_t *y)
{
}

void
ShmConPrdQueue::shm_register_handler(int smq_code, ShmqReqIn *cb)
{
}

void
ShmConPrdQueue::shm_consume_loop(int consumer_idx)
{
    int          size = 64;     // take from parameter/constructor
    char         data[size];
    shmq_req_t  *inp;

    inp = reinterpret_cast<shmq_req_t *>(data);
    while (1) {
        /* Block if we don't have anything to consume. */
        shm_consumer(data, size, consumer_idx);

        if (inp->smq_code & SHMQ_TRACK_MASK) {
            /* Lookup the saved request in shm_track_request() */
            ShmqReqOut *orig = shm_get_saved_req(inp->smq_id);
            if (orig != NULL) {
                /* Notify the sender, this also takes it out of the queue. */
                orig->req_complete();
                continue;
            }
            /* Clear the mask and lookup registered handler. */
            inp->smq_code &= ~SHMQ_TRACK_MASK;
        }
        ShmqReqIn *cb = shm_get_callback(inp->smq_code);
        if (cb != NULL) {
            cb->shmq_handler(inp, size);
        }
        // Remove me!
        sleep(2);
    }
}

ShmqReqOut *
ShmConPrdQueue::shm_get_saved_req(fds_uint32_t id)
{
    return NULL;
}

ShmqReqIn *
ShmConPrdQueue::shm_get_callback(fds_uint32_t code)
{
    return NULL;
}

Shm_1Prd_nCon::Shm_1Prd_nCon(shm_con_prd_sync_t *sync, shm_1prd_ncon_q_t *ctrl, ShmObjRW *data)
        : ShmConPrdQueue(sync, ctrl, data)
{
}

void
Shm_1Prd_nCon::shm_consumer(void *data, size_t size, int consumer /* = 0 */)
{
}

void
Shm_1Prd_nCon::shm_producer(const void *data, size_t size, int producer /* = 0 */)
{
}


Shm_nPrd_1Con::Shm_nPrd_1Con(shm_con_prd_sync_t *sync, shm_nprd_1con_q_t *ctrl, ShmObjRW *data)
        : ShmConPrdQueue(sync, ctrl, data)
{
}

void
Shm_nPrd_1Con::shm_consumer(void *data, size_t size, int consumer /* = 0 */)
{
}
void
Shm_nPrd_1Con::shm_producer(const void *data, size_t size, int producer /* = 0 */)
{
}

}  // namespace fds
